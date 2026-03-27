#include "multimap.h"
#include <string>
#include <vector>

// TODO: implement at least one constructor


Multimap::Iterator::Iterator(const std::vector<std::string>* v) :
	m_val(v), m_pos(0) {}


bool Multimap::Iterator::next(std::string& value)
{
	if (m_val == nullptr || m_pos >= m_val->size()) {
		return false;
	}
	value = (*m_val)[m_pos++];
	return true;
}

Multimap::Iterator::~Iterator()
{
	// No dynamic memory to clean up 
	// (I mean we do not allocate any memory 
	// during the implementation of this class)
}

Multimap::Multimap()
{
	m_root = nullptr;
	m_size = 0;
}

void Multimap::cleanup(Node* n) {
	if (n == nullptr) {
		return;
	}
	cleanup(n->left);
	cleanup(n->right);
	m_size--;
	delete n;
}

Multimap::~Multimap()
{
	cleanup(m_root);
}

void Multimap::put(const std::string& key, const std::string& value)
{
	if (m_root == nullptr) {
		// Tree is empty, create root node
		Node* newNode = new Node;
		// Initialize the new node
		newNode->m_key = key;
		newNode->m_values.push_back(value);
		newNode->left = nullptr;
		newNode->right = nullptr;
		m_root = newNode;
		m_size++;
		return;
	}
	// Otherwise, find the correct position to insert the key-value pair
	Node* current = m_root;
	while (true) {
		if (key == current->m_key) {
			for (const auto& v : current->m_values) {
				if (v == value) return;
			}
			current->m_values.push_back(value); // Key exists, add value to vector
			m_size++;
			return; // Key already exists, do not insert duplicate
		}
		else if (key < current->m_key) {
			if (current->left == nullptr) {
				// Insert new node as left child
				Node* newNode = new Node;
				newNode->m_key = key;
				newNode->m_values.push_back(value);
				newNode->left = nullptr;
				newNode->right = nullptr;
				current->left = newNode;
				m_size++;
				return;
			}
			current = current->left; // Move left
		}
		else { // key > current->m_key
			if (current->right == nullptr) {
				// Insert new node as right child
				Node* newNode = new Node;
				newNode->m_key = key;
				newNode->m_values.push_back(value);
				newNode->left = nullptr;
				newNode->right = nullptr;
				current->right = newNode;
				m_size++;
				return;
			}
			current = current->right; // Move right
		}
	}
}

MultimapBase::IteratorBase* Multimap::get(const std::string& key) const
{
	if (m_root == nullptr) {
		return new Iterator(nullptr); // Tree is empty
	}
	Node* current = m_root;
	while (current != nullptr) {
		if (key == current->m_key) {
			return new Iterator(&(current->m_values));
			// Key found, return iterator
		}
		else if (key < current->m_key) {
			current = current->left; // Move left
		}
		else { // else if key > current->m_key
			current = current->right; // Move right
		}
	}
	return new Iterator(nullptr); // Key not found
}

bool Multimap::empty() const
{
	if (m_root == nullptr) {
		return true; // Tree is empty
	}
	return false; // Tree is not empty
}

int Multimap::size() const
{
	return m_size;
}