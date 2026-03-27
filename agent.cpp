#include "agent.h"
#include "provided.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <set>

Agent::Agent(const IndexBase& index)
 : m_index(index)
{
	m_termsTemplate = "";
	m_summarizeTemplate = "";
	m_loadedPrompts = false;
}

Agent::~Agent()
{
	// No dynamic memory to clean up in this implementation
}

void Agent::replace_all(std::string& s, const std::string& placeholder, const std::string& replacement)
{
	// at most replace the first occurrence of placeholder in s with replacement
	size_t pos = 0;
	while ((pos = s.find(placeholder, pos)) != std::string::npos) {
		s.replace(pos, placeholder.length(), replacement);
		pos += replacement.length(); // Move past the replacement
	}
	return;
}

bool Agent::load_prompts(const std::string& terms_file, const std::string& summarize_file)
{
	std::ifstream termsStream(terms_file);
	
	if (!termsStream) {
		// Failed to open terms file
		return false;
	}
	std::stringstream termsBuffer;
	termsBuffer << termsStream.rdbuf();
	m_termsTemplate = termsBuffer.str();

	std::ifstream summarizeStream(summarize_file);
	if (!summarizeStream) {
		// Failed to open summarize file
		return false;
	}
	std::stringstream summarizeBuffer;
	summarizeBuffer << summarizeStream.rdbuf();
	m_summarizeTemplate = summarizeBuffer.str();

	m_loadedPrompts = true;
	return true;
}


bool Agent::query(const std::string& question, std::string& answer)
{
	if (!m_loadedPrompts) {
		return false;
	}

	std::string copy_termsPrompt = m_termsTemplate;
	replace_all(copy_termsPrompt, "{query}", question);

	std::string termsResponse;
	if (!query_llm("terms", copy_termsPrompt, termsResponse)) {
		return false;
	}

	std::stringstream ss(termsResponse);
	std::string line;
	std::set<std::string> docMatch;

	TokenizerBase* tokenizer = create_tokenizer();

	while (std::getline(ss, line)) {
		tokenizer->tokenize(line);
		std::string token;
		std::vector<std::string> m_term_tokens;

		while (tokenizer->next(token))
		{
			m_term_tokens.push_back(token);
		}

		if (!m_term_tokens.empty()) {
			std::vector<std::string> tmp = m_index.query(m_term_tokens);
			for (const std::string& doc : tmp) {
				docMatch.insert(doc);
			}
		}
	}

	// Clean up the tokenizer once we are done with the loop
	delete tokenizer;

	// If we went through all lines and still found nothing, fail.
	if (docMatch.empty()) {
		return false;
	}

	// Read the content of up to the first 10 matching documents
	std::string combinedDocs;
	int count = 0;
	for (const std::string& doc : docMatch) {
		if (count >= 10) {
			break;
		}

		std::ifstream ifs(doc);
		if (ifs) {
			std::stringstream docBuffer;
			docBuffer << ifs.rdbuf();
			combinedDocs += docBuffer.str() + "\n";
			count++;
		}
	}

	// Prepare the summarization prompt and get the final answer
	std::string copy_summarizePrompt = m_summarizeTemplate;
	replace_all(copy_summarizePrompt, "{query}", question);
	replace_all(copy_summarizePrompt, "{documents}", combinedDocs);

	return query_llm("summarize", copy_summarizePrompt, answer);
}