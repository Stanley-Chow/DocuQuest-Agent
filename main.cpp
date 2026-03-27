#include "provided.h"
#include <iostream>
#include <string>
#include <cassert>
#include <fstream>

const std::string DOCUMENT_DIRECTORY = "C:\\Users\\18510\\Desktop\\docs";

void testMultimapEx() {
    std::cout << "--- Starting Multimap Tests Provided by Specification---" << std::endl;
	// This is a testing program provided by specificiation of Project 4.
    MultimapBase* mm = create_multimap();
    mm->put("carey", "silly");
    mm->put("carey", "boring");
    mm->put("carey", "silly");
    // duplicate value: ignored
    mm->put("carey", "awesome");
    MultimapBase::IteratorBase* it = mm->get("carey");
    std::string value;
    while (it->next(value))
        std::cout << value << "\n"; // prints: boring, awesome, silly
    // not necessarily in that order
    delete it;
    delete mm;
}

void testMultimap() {
    std::cout << "--- Starting Multimap Tests ---" << std::endl;

    // 1. Create Multimap via Factory
    MultimapBase* mm = create_multimap();
    assert(mm->empty());
    assert(mm->size() == 0);

    // 2. Test Basic Put
    mm->put("ucla", "bruins");
    mm->put("ucla", "westwood");
    mm->put("usc", "trojans");

    // Size should be 3 (total number of values stored)
    assert(mm->size() == 3);
    assert(!mm->empty());

    // 3. Test Duplicate Prevention
    // Adding the exact same key-value pair should do nothing
    mm->put("ucla", "bruins");
    assert(mm->size() == 3);

    // 4. Test Retrieval (Iterator)
    std::cout << "Testing retrieval for 'ucla'..." << std::endl;
    MultimapBase::IteratorBase* it = mm->get("ucla");
    std::string val;
    int count = 0;
    while (it->next(val)) {
        std::cout << "  Found: " << val << std::endl;
        count++;
    }
    assert(count == 2);
    delete it; // Crucial: Always delete iterators!

    // 5. Test Non-existent Key
    it = mm->get("stanford");
    assert(!it->next(val)); // Should return false immediately
    delete it;

    // 6. Test Alphabetical BST Ordering (Implicit test)
    // Adding keys in an order that forces a specific tree structure
    mm->put("apple", "fruit");
    mm->put("zebra", "animal");
    mm->put("orange", "color");

    it = mm->get("zebra");
    assert(it->next(val) && val == "animal");
    delete it;
    
    // 7. Large scale test for memory/leaks
    for (int i = 0; i < 1000; i++) {
        mm->put("key" + std::to_string(i), "value" + std::to_string(i));
    }
    assert(mm->size() == 1006);

    std::cout << "All Multimap tests passed!" << std::endl;
    delete mm; // Test destructor cleanup
}

void testTokenizer() {
    std::cout << "--- Starting Tokenizer Tests ---" << std::endl;

    // 1. Create Tokenizer via Factory
    TokenizerBase* t = create_tokenizer();

    // 2. Test Basic Tokenization
    // Should result in: "this", "is", "a", "test"
    t->tokenize("This is a test.");
    std::string token;
    std::vector<std::string> results;
    while (t->next(token)) {
        results.push_back(token);
    }
    assert(results.size() == 4);
    assert(results[0] == "this"); // Test lowercase conversion
    assert(results[3] == "test"); // Test punctuation stripping

    // 3. Test Complex Delimiters and Numbers
    // Alphanumeric means A-Z, a-z, and 0-9.
    // Should result in: "cs32", "is", "fun", "2026"
    results.clear();
    t->tokenize("CS32 is... FUN! (2026)");
    while (t->next(token)) {
        results.push_back(token);
    }
    assert(results.size() == 4);
    assert(results[0] == "cs32");
    assert(results[3] == "2026");



    // 4. Test Consecutive Delimiters
    // Should skip multiple spaces/symbols
    results.clear();
    t->tokenize("word1,,,   word2@@@word3");
    while (t->next(token)) {
        results.push_back(token);
    }
    assert(results.size() == 3);
    assert(results[1] == "word2");

    // 5. Test Empty or Only-Delimiter Strings
    results.clear();
    t->tokenize("   !!! ???   ");
    assert(!t->next(token)); // Should immediately return false

    // 6. Test Tokenizing same object multiple times
    // Ensure m_pos resets correctly
    t->tokenize("First1");
    t->next(token);
    t->tokenize("Second");
    t->next(token);
    assert(token == "second");

    std::cout << "All Tokenizer tests passed!" << std::endl;
    delete t;
}

// Helper to create a quick test file
void createTestFile(const std::string& name, const std::string& content) {
    std::ofstream outfile(name);
    outfile << content;
    outfile.close();
}

void testIndex() {
    std::cout << "--- Starting Index Tests ---" << std::endl;

    // 1. Setup dummy files for indexing
    createTestFile("test_doc1.txt", "UCLA Bruins basketball is fun.");
    createTestFile("test_doc2.txt", "UCLA academics are great, but basketball is better.");

    // 2. Create Index via Factory
    IndexBase* idx = create_index();

    // 3. Test add_doc
    idx->add_doc("test_doc1.txt");
    idx->add_doc("test_doc2.txt");

    // 4. Test Single Word Query
    // "basketball" appears in both files
    std::vector<std::string> result = idx->query({ "basketball" });
    assert(result.size() == 2);

    // "academics" appears only in doc2
    result = idx->query({ "academics" });
    assert(result.size() == 1);
    assert(result[0] == "test_doc2.txt");

    // 5. Test Multi-word Intersection Query
    // Both words "ucla" and "fun" only appear together in doc1
    result = idx->query({"ucla", "fun" });
    assert(result.size() == 1);
    assert(result[0] == "test_doc1.txt");

    // 6. Test Query with No Matches
    // "bruins" is in doc1, "academics" is in doc2. Intersection is empty.
    result = idx->query({ "bruins", "academics" });
    assert(result.empty());

    // 7. Test Case Insensitivity (handled by Tokenizer)
    // We don't offer query to lower case check
    result = idx->query({ "BASKETBALL" });
    assert(result.size() == 0);

    // 8. Test Non-existent word
    result = idx->query({ "stanford" });
    assert(result.empty());

    std::cout << "All Index tests passed!" << std::endl;
    delete idx;
}

// Helper to create prompt files
void createPromptFile(const std::string& name, const std::string& content) {
    std::ofstream outfile(name);
    outfile << content;
    outfile.close();
}

void testAgent() {
    std::cout << "--- Starting Agent Tests ---" << std::endl;

    // 1. Setup: Agent needs an Index to function
    IndexBase* idx = create_index();
    // Index some dummy data so the agent has documents to summarize
    std::ofstream out("test_doc.txt");
    out << "The UCLA Bruins won the game yesterday in Westwood.";
    out.close();
    idx->add_doc("test_doc.txt");

    // 2. Create Agent
    AgentBase* agent = create_agent(*idx);

    // 3. Setup Prompt Templates
    // Note: These must contain the exact {query} and {documents} placeholders
    createPromptFile("test_terms.txt", "Extract search terms for: {query}");
    createPromptFile("test_summarize.txt", "Answer the question: {query} using these docs: {documents}");

    // 4. Test Load Prompts
    assert(agent->load_prompts("test_terms.txt", "test_summarize.txt"));
    assert(!agent->load_prompts("non_existent.txt", "summarize.txt")); // Should return false

    // 5. Test Query (Requires .orkey to be valid)
    std::cout << "Attempting LLM Query (this may take a few seconds)..." << std::endl;
    std::string answer;
    bool load_result = agent->load_prompts("test_terms.txt", "test_summarize.txt");
    bool success = agent->query("Who won the game?", answer);

    if (success) {
        std::cout << "Query Succeeded!" << std::endl;
        std::cout << "LLM Answer: " << answer << std::endl;
    }
    else {
        // If this fails, it's likely an API key issue or no documents matched the LLM's terms
        std::cout << "Query failed. Check your .orkey file and internet connection." << std::endl;
    }

    std::cout << "Agent tests completed." << std::endl;
    delete agent;
    delete idx;
}

int main() {
	std::cout << "Running tests..." << std::endl;
	testMultimapEx();
	std::cout << std::endl;
	testMultimap();
	std::cout << std::endl;
	testTokenizer();
    std::cout << std::endl;
	testIndex();
	std::cout << std::endl;
	testAgent();
	std::cout << std::endl;
	std::cout << "testAgent() ignore! To save token!" << std::endl;
	std::cout << "All tests completed successfully!" << std::endl;
	std::cout << "Now you can run the main program to interact with the agent." << std::endl;
    std::cout << std::endl;

	// Main program logic
    // Build index from all files in the docs directory
    std::cout << "Loading documents..." << std::endl;
    IndexBase* index = create_index();
    int numDocs = index->build_index(DOCUMENT_DIRECTORY);
    std::cout << "Indexed " << numDocs << " documents." << std::endl;
    AgentBase* agent = create_agent(*index);
    agent->load_prompts("terms.txt", "summarize.txt");

    std::string input;
    for (;;) {
        std::cout << "Enter question (or 'quit' to exit): ";
        std::getline(std::cin, input);

        if (input.empty()) continue;
        if (input == "quit" || input == "exit" || input == "q") break;
        
		std::cout << "You entered: " << input << std::endl;
        std::cout << std::endl;
        std::string answer;
        if (!agent->query(input, answer)) {
            std::cerr << "Query failed." << std::endl;
        }
        else {
            std::cout << answer << std::endl;
        }
        std::cout << std::endl;
    }

    delete agent;
    delete index;

    std::cout << "So far so good!" << std::endl;
    std::cout << "Pass!" << std::endl;
	return 0;
}