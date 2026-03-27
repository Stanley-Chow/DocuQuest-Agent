#include "tokenizer.h"
#include <string>

Tokenizer::Tokenizer()
{
    m_index = 0;
	m_tokens = "";
}

Tokenizer::~Tokenizer()
{
    // No dynamic memory to clean up
    // I mean we do not allocate any memory for this class's implementation
	//, so we do not need to do anything in the destructor
}

void Tokenizer::tokenize(const std::string& input)
{
    // reset the tokenizer state
	m_tokens = input;
	m_index = 0;
}

bool Tokenizer::next(std::string& token)
{
	// FIX: To prevent negative index crash
	while (m_index < m_tokens.size() && !std::isalnum(static_cast<unsigned char>(m_tokens[m_index]))) {
		m_index++;
	}

	if (m_index >= m_tokens.size()) {
		return false;
	}

	std::string result;
	// FIX: Add static_cast<unsigned char> as well
	while (m_index < m_tokens.size() && std::isalnum(static_cast<unsigned char>(m_tokens[m_index]))) {
		result += std::tolower(static_cast<unsigned char>(m_tokens[m_index]));
		m_index++;
	}
	token = result;
	return true;
}