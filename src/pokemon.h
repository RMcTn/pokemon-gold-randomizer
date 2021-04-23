#pragma once

#include <string>
#include "pokemon_stats.h"

class Pokemon {
public:
    // TODO: just make struct at some point
    Pokemon(unsigned int id, std::string name);

    void set_id(unsigned int id) { this->id = id; }

    unsigned int get_id() const { return id; }

    void set_name(std::string name) { this->name = name; }

    std::string get_name() const { return name; }


    PokemonStats stats;

private:
    unsigned int id;
    std::string name;
};
