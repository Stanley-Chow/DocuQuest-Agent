#ifndef PROVIDED_H
#define PROVIDED_H

#include <string>
#include <vector>

// Base class for multimaps
class MultimapBase
{
public:
    // Nested base class for iterators
    class IteratorBase {
    public:
        virtual ~IteratorBase() {}
        virtual bool next(std::string& value) = 0;
    };

    virtual ~MultimapBase() {}
    virtual void put(const std::string& key, const std::string& value) = 0;
    virtual IteratorBase* get(const std::string& key) const = 0;
    virtual bool empty() const = 0;
    virtual int size() const = 0;
};

// Base class for tokenizers
class TokenizerBase
{
public:
    virtual ~TokenizerBase() {}
    virtual void tokenize(const std::string& input) = 0;
    virtual bool next(std::string& token) = 0;
};

// Base class for indexes
class IndexBase
{
public:
    virtual ~IndexBase() {}
    virtual void add_doc(const std::string& doc_file) = 0;
    virtual int build_index(const std::string& path) = 0;
    virtual std::vector<std::string> query(const std::vector<std::string>& terms) const = 0;
};

// Base class for agents
class AgentBase
{
public:
    virtual ~AgentBase() {}
    virtual bool load_prompts(const std::string& terms_file, const std::string& summarize_file) = 0;
    virtual bool query(const std::string& question, std::string& answer) = 0;
};

// Factories for derived implementations (implemented in provided.cpp)
MultimapBase* create_multimap();
TokenizerBase* create_tokenizer();
IndexBase* create_index();
AgentBase* create_agent(const IndexBase& index);

// Get all filenames in a directory (excluding "." and "..")
std::vector<std::string> get_filenames(const std::string& directory);

// Set the name of the API file key to be other than the default "./.orkey"
void set_api_key_filename(const std::string& filename);

// Query an LLM with the given prompt and return the result
// category: a string indicating the type of query (e.g., "terms", "summarize")
bool query_llm(const std::string& category, const std::string& prompt, std::string& response);

#endif // PROVIDED_H
