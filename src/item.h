#pragma once
#include <string>
class Item {
public:
	Item(unsigned int id, std::string name);
	void set_id(unsigned int id) { this->id = id; }
	unsigned int get_id() const { return id; }
	void set_name(std::string name) { this->name = name; }
	std::string get_name() const { return name; }
	

private:
	unsigned int id;
	std::string name;
};
