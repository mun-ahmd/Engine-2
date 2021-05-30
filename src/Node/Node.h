#pragma once
#include <vector>

template <typename T>
class Node
{
public:
	Node(Node* parentIN, T* objectIN)
	{
		parent = parentIN;
		object = objectIN;
	}
	~Node()
	{
		//this aint good cause dont need to remove parents for anyone other than the first
		parent->removeChild(this);
		for (std::vector<Node*>::iterator ptr = children.begin(); ptr < children.end(); ptr++)
			ptr->~Node();
	}
	removeChild(Node* child)
	{
		std::remove(children.begin(), children.end(), child);
	}
private:
	std::vector<Node*> children;
	Node* parent;
	T* object;
};