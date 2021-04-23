#pragma once

#include <string>

class Pokemon {
public:
    // TODO: just make struct at some point
    Pokemon(unsigned int id, std::string name);

    void set_id(unsigned int id) { this->id = id; }

    unsigned int get_id() const { return id; }

    void set_name(std::string name) { this->name = name; }

    std::string get_name() const { return name; }

    unsigned int hp;
    unsigned int attack;
    unsigned int defence;
    unsigned int speed;
    unsigned int special_attack;
    unsigned int special_defence;

private:
    unsigned int id;
    std::string name;
};
