#include "provided.h"
#include "multimap.h"
#include "tokenizer.h"
#include "index.h"
#include "agent.h"
#include <string>
#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstring>
#include <cassert>
#include <cctype>

// HTTP backend selection: Windows: use WinHTTP; non-Windows: use curl
// Define USE_CURL to force the curl backend on Windows.
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN  // fixes "byte" for pre-Windows SDK 10.0.20348.0
#include <windows.h>
#ifndef USE_CURL
#define USE_WINHTTP
#include <winhttp.h>
#pragma comment(lib, "winhttp.lib")
#endif
#endif

#ifndef USE_WINHTTP
#include <curl/curl.h>
#endif

MultimapBase* create_multimap() {
    return new Multimap();
}

TokenizerBase* create_tokenizer() {
    return new Tokenizer();
}

IndexBase* create_index() {
    return new Index();
}

AgentBase* create_agent(const IndexBase& index) {
    return new Agent(index);
}

static std::string apiKeyFilename = "C:\\Users\\18510\\Desktop\\.orkey";  // default if set_api_key_filename is not called

void set_api_key_filename(const std::string& filename)
{
	apiKeyFilename = filename;
}

std::vector<std::string> get_filenames(const std::string& directory_name) {
	std::vector<std::string> result;
	std::error_code ec;
	std::filesystem::path dirPath(directory_name.empty() ? "." : directory_name);
	if (! std::filesystem::is_directory(dirPath, ec)) {
		auto msg = (ec ? ec.message() : "not a directory");
		std::cerr << "Error: " << dirPath << ": " << msg << std::endl;
		return result;
	}
	std::filesystem::directory_iterator dirIter(dirPath, ec);
	if (ec) {
		std::cerr << "Error: it " << dirPath << ": " << ec.message() << std::endl;
		return result;
	}
	for (const auto& dir_entry : dirIter) {
		auto p = dir_entry.path();
		result.push_back((directory_name.empty() ? p.stem() : p).string());
	}
	return result;
}

// Helper function to read API key from file
static std::string readApiKey(const std::string& filename) {
    std::ifstream file(filename);
    if (!file) {
        return "";
    }
    std::string key;
    std::getline(file, key);
    return key;
}

// Helper function to escape JSON string
static std::string escapeJson(const std::string& str) {
    std::string escaped;
    for (char c : str) {
        switch (c) {
            case '"':  escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:   escaped += c; break;
        }
    }
    return escaped;
}

static bool buildJsonPayload(const std::string& prompt, std::string& jsonPrompt) {
    std::ostringstream jsonPayload;
    jsonPayload << "{"
                << "\"model\": \"qwen/qwen-2.5-7b-instruct\","
                << "\"messages\": ["
                << "{\"role\": \"user\", \"content\": \"" << escapeJson(prompt) << "\"}"
                << "],"
                << "\"thinking\": false"
                << "}";
    jsonPrompt = jsonPayload.str();
    return true;
}

// Helper function to extract result from JSON response
// OpenRouter response format: {"choices":[{"message":{"content":"..."}}]}
static std::string extractResult(const std::string& json) {
    // Look for "content" field in the response (should be in choices[0].message.content)
    size_t contentPos = json.find("\"content\"");
    if (contentPos == std::string::npos) {
        return "";
    }

    // Find the colon after "content"
    size_t colonPos = json.find(":", contentPos);
    if (colonPos == std::string::npos) {
        return "";
    }

    // Skip any whitespace after colon
    size_t valueStart = colonPos + 1;
    while (valueStart < json.length() &&
           std::isspace(static_cast<unsigned char>(json[valueStart]))) {
        valueStart++;
    }

    // Check if value is a string (starts with quote)
    if (valueStart >= json.length() || json[valueStart] != '"') {
        return "";
    }

    valueStart++; // Skip opening quote

    // Find closing quote, handling escaped quotes
    size_t valueEnd = valueStart;
    while (valueEnd < json.length()) {
        if (json[valueEnd] == '"' && (valueEnd == valueStart || json[valueEnd - 1] != '\\')) {
            break;
        }
        // Skip escaped characters
        if (json[valueEnd] == '\\' && valueEnd + 1 < json.length()) {
            valueEnd += 2;
        } else {
            valueEnd++;
        }
    }

    if (valueEnd >= json.length()) {
        return "";
    }

    std::string content = json.substr(valueStart, valueEnd - valueStart);

    // Unescape JSON string
    std::string unescaped;
    for (size_t i = 0; i < content.length(); i++) {
        if (content[i] == '\\' && i + 1 < content.length()) {
            switch (content[i + 1]) {
                case '"':  unescaped += '"';  i++; break;
                case '\\': unescaped += '\\'; i++; break;
                case '/':  unescaped += '/';  i++; break;
                case 'b':  unescaped += '\b'; i++; break;
                case 'f':  unescaped += '\f'; i++; break;
                case 'n':  unescaped += '\n'; i++; break;
                case 'r':  unescaped += '\r'; i++; break;
                case 't':  unescaped += '\t'; i++; break;
                default:
                    // Preserve unknown escape sequences as-is
                    unescaped += content[i];
                    unescaped += content[i + 1];
                    i++;
                    break;
            }
        } else {
            unescaped += content[i];
        }
    }

    return unescaped;
}

#ifdef USE_WINHTTP

bool ask_llm(const std::string& jsonPrompt, std::string& jsonResponse, const std::string& apiKey) {
    HINTERNET hSession = WinHttpOpen(L"CS32Agent/1.0",
                                     WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                                     WINHTTP_NO_PROXY_NAME,
                                     WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hSession) {
        std::cerr << "Error: WinHttpOpen failed (" << GetLastError() << ")" << std::endl;
        return false;
    }

    HINTERNET hConnect = WinHttpConnect(hSession, L"openrouter.ai",
                                        INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!hConnect) {
        std::cerr << "Error: WinHttpConnect failed (" << GetLastError() << ")" << std::endl;
        WinHttpCloseHandle(hSession);
        return false;
    }

    HINTERNET hRequest = WinHttpOpenRequest(hConnect, L"POST",
                                            L"/api/v1/chat/completions",
                                            NULL, WINHTTP_NO_REFERER,
                                            WINHTTP_DEFAULT_ACCEPT_TYPES,
                                            WINHTTP_FLAG_SECURE);
    if (!hRequest) {
        std::cerr << "Error: WinHttpOpenRequest failed (" << GetLastError() << ")" << std::endl;
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    std::wstring wApiKey(apiKey.begin(), apiKey.end());
    std::wstring headers = L"Content-Type: application/json\r\nAuthorization: Bearer " + wApiKey;

    BOOL ok = WinHttpSendRequest(hRequest,
                                 headers.c_str(), (DWORD)headers.size(),
                                 (LPVOID)jsonPrompt.c_str(),
                                 (DWORD)jsonPrompt.size(),
                                 (DWORD)jsonPrompt.size(), 0);
    if (!ok) {
        std::cerr << "Error: WinHttpSendRequest failed (" << GetLastError() << ")" << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    ok = WinHttpReceiveResponse(hRequest, NULL);
    if (!ok) {
        std::cerr << "Error: WinHttpReceiveResponse failed (" << GetLastError() << ")" << std::endl;
        WinHttpCloseHandle(hRequest);
        WinHttpCloseHandle(hConnect);
        WinHttpCloseHandle(hSession);
        return false;
    }

    DWORD statusCode = 0;
    DWORD statusCodeSize = sizeof(statusCode);
    WinHttpQueryHeaders(hRequest,
                        WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX,
                        &statusCode, &statusCodeSize, WINHTTP_NO_HEADER_INDEX);

    DWORD bytesAvailable = 0;
    DWORD bytesRead = 0;
    do {
        bytesAvailable = 0;
        if (!WinHttpQueryDataAvailable(hRequest, &bytesAvailable))
            break;
        if (bytesAvailable == 0)
            break;
        std::vector<char> buffer(bytesAvailable);
        if (!WinHttpReadData(hRequest, buffer.data(), bytesAvailable, &bytesRead))
            break;
        jsonResponse.append(buffer.data(), bytesRead);
    } while (bytesRead > 0);

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    if (statusCode < 200 || statusCode >= 300) {
        std::cerr << "Error: HTTP request failed with status code " << statusCode << std::endl;
        std::cerr << "Response: " << jsonResponse << std::endl;
        return false;
    }

	return true;
}

#else // !USE_WINHTTP -- use libcurl

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

bool ask_llm(const std::string& jsonPrompt, std::string& jsonResponse, const std::string& apiKey) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Error: Failed to initialize curl" << std::endl;
        return false;
    }

    struct curl_slist *headers = nullptr;

    headers = curl_slist_append(headers, "Content-Type: application/json");
    std::string authHeader = "Authorization: Bearer " + apiKey;
    headers = curl_slist_append(headers, authHeader.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, "https://openrouter.ai/api/v1/chat/completions");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonPrompt.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &jsonResponse);

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "Error: curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code < 200 || http_code >= 300) {
        std::cerr << "Error: HTTP request failed with status code " << http_code << std::endl;
        std::cerr << "Response: " << jsonResponse << std::endl;
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        return false;
    }

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

	return true;
}

#endif

bool query_llm(const std::string& category, const std::string& prompt, std::string& response) {
    assert(category == "terms" || category == "summarize");
    (void)category;

    const size_t MAX_LLM_QUERY_CHARS = 50000;
    if (prompt.size() > MAX_LLM_QUERY_CHARS) {
        std::cerr << "Error: Prompt too long (" << prompt.size()
                  << " chars); max is " << MAX_LLM_QUERY_CHARS << std::endl;
        return false;
    }

    std::string apiKey = readApiKey(apiKeyFilename);
    if (apiKey.empty()) {
        std::cerr << "Error: Could not read API key from " << apiKeyFilename << std::endl;
        return false;
    }

    std::string jsonPrompt;
    buildJsonPayload(prompt, jsonPrompt);

	std::string jsonResponse;
	if (!ask_llm(jsonPrompt, jsonResponse, apiKey))
		return false;

    response = extractResult(jsonResponse);
    if (response.empty()) {
        std::cerr << "Error: Could not extract response from LLM's response" << std::endl;
        std::cerr << "LLM's Response: " << jsonResponse << std::endl;
        return false;
    }

    return true;
}
