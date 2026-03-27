#ifndef MULTIMAP_H
#define MULTIMAP_H

#include "provided.h"
#include <string>
#include <vector>

class Multimap : public MultimapBase
{
public:
    // Nested Iterator class
    class Iterator : public MultimapBase::IteratorBase
    {
    public:
        Iterator(const std::vector<std::string>* v);
        virtual ~Iterator();

        virtual bool next(std::string& value);

    private:
		const std::vector<std::string>* m_val; 
        // Pointer to the vector of values for the current key
        size_t m_pos; 
        // The current position in the vector
    };


    // Constructor & Destructor
    Multimap();
    virtual ~Multimap();

    virtual void put(const std::string& key, const std::string& value);
    virtual MultimapBase::IteratorBase* get(const std::string& key) const;
    virtual bool empty() const;
    virtual int size() const;

private:
    struct Node{
        std::string m_key;
        std::vector<std::string> m_values;
        Node* left = nullptr;
        Node* right = nullptr;
	};

    void cleanup(Node* n);

    Node* m_root;
    int m_size;
};

#endif // MULTIMAP_H