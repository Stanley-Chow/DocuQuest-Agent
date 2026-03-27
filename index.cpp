#include "index.h"
#include "provided.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <iterator>

// Constructor: This is where we prevent the 0xc0000005 crash!
Index::Index() {
    // We MUST use the factory function to create our multimap
    m_map = create_multimap();
}

// Destructor: Clean up the memory we allocated in the constructor
Index::~Index() {
    delete m_map;
}

int Index::build_index(const std::string& path) {
    std::vector<std::string> filenames = get_filenames(path);
    if (filenames.empty()) {
        return 0;
    }

    int docsIndexed = 0;
    for (const std::string& name : filenames) {
        // Construct the full path
        std::string fullPath = name;
        add_doc(fullPath);
        docsIndexed++;
    }
    return docsIndexed;
}

void Index::add_doc(const std::string& doc_file) {
    std::ifstream infile(doc_file);
    if (!infile) {
        return; // Skip files that won't open
    }

    // Read the entire file content into a string
    std::stringstream buffer;
    buffer << infile.rdbuf();
    std::string content = buffer.str();

    // Use the factory to create a tokenizer
    TokenizerBase* t = create_tokenizer();
    t->tokenize(content);

    std::string token;
    while (t->next(token)) {
        // Logic: Put the (token, document_path) pair into our BST
        m_map->put(token, doc_file);
    }

    delete t; // Clean up the tokenizer
}

std::vector<std::string> Index::query(const std::vector<std::string>& terms) const {
    if (terms.empty()) return {};

    // 1. Get documents for the first term to start our "matches" set
    std::set<std::string> current_matches;
    MultimapBase::IteratorBase* it0 = m_map->get(terms[0]);
    std::string doc;
    while (it0->next(doc)) {
        current_matches.insert(doc);
    }
    delete it0;

    // 2. Intersect with documents for the remaining terms
    for (size_t i = 1; i < terms.size(); i++) {
        // If we already have no matches, we can't possibly find an intersection
        if (current_matches.empty()) break;

        std::set<std::string> term_docs;
        MultimapBase::IteratorBase* itN = m_map->get(terms[i]);
        while (itN->next(doc)) {
            term_docs.insert(doc);
        }
        delete itN;

        // Perform the intersection: Keep only docs found in BOTH sets
        std::set<std::string> intersection;
        std::set_intersection(
            current_matches.begin(), current_matches.end(),
            term_docs.begin(), term_docs.end(),
            std::inserter(intersection, intersection.begin())
        );

        current_matches = intersection; // Update our running list of matches
    }

    // 3. Convert final set back to vector
    return std::vector<std::string>(current_matches.begin(), current_matches.end());
}

/*
#include "index.h"
#include "provided.h"
#include <string>
#include <vector>
#include <set>
#include <algorithm>
#include <iostream>
#include <fstream>

Index::Index()
{
	m_map = create_multimap();
}

Index::~Index()
{
	delete m_map;
}

int Index::build_index(const std::string& path)
{
	std::vector<std::string> doc_files = get_filenames(path);
	if (doc_files.empty()) {
		return 0; // No documents found, return 0
	}
	for (const std::string& doc_file : doc_files) {
		add_doc(doc_file); // not sure if this should be doc_file 
							//or path + "/" + doc_file
	}
	return (int)doc_files.size();
}

void Index::add_doc(const std::string& doc_file)
{
	std::ifstream file(doc_file);
	std::string line;
	TokenizerBase* tokenizer = create_tokenizer();
	while (std::getline(file, line)) {
		tokenizer->tokenize(line);
		std::string token;
		while (tokenizer->next(token)) {
			m_map->put(token, doc_file);
		}
	}
	delete tokenizer;
	file.close();
	return;
}

std::vector<std::string> Index::query(const std::vector<std::string>& terms) const
{
	if (terms.empty()) {
		return {};
	}
	// Access the first term's documents
	std::set<std::string> result_set;
	MultimapBase::IteratorBase* iter;

	iter = m_map->get(terms[0]);
	std::string value;
	while (iter->next(value)) {
		result_set.insert(value);
	}

	for (const std::string& term : terms) {
		std::set<std::string> tmp_set;
		std::set<std::string> m_intersection;
		iter = m_map->get(term);
		while (iter->next(value)) {
			tmp_set.insert(value);
		}
		std::set_intersection(result_set.begin(), result_set.end(), tmp_set.begin(), tmp_set.end(),
			std::inserter(m_intersection, m_intersection.begin()));
		result_set = m_intersection;
	}

	return std::vector<std::string>(result_set.begin(), 
		result_set.end());
}
*/