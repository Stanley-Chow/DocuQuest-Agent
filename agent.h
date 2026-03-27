#ifndef AGENT_H
#define AGENT_H

#include "provided.h"
#include <string>
#include <vector>
#include <set>

class Agent : public AgentBase
{
public:
    Agent(const IndexBase& index);
    virtual ~Agent();
    virtual bool load_prompts(const std::string& terms_file, const std::string& summarize_file);
    virtual bool query(const std::string& question, std::string& answer);

private:
    const IndexBase& m_index;
	std::string m_termsTemplate;
	std::string m_summarizeTemplate;
	bool m_loadedPrompts;

    void replace_all(std::string& s, const std::string& placeholder, 
        const std::string& replacement);
};

#endif // AGENT_H