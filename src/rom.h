#include "pokemon.h"

#include <fstream>
#include <vector>
#include <map>

class Rom {

public:
	Rom();
	void randomize_starters(int seed = 0);
	bool load();
	bool save();
	void run();	//Just a driver function whilst functionality is being built

private:
	std::fstream file;
	std::vector<uint8_t> rom;
	//Mapping of english characters to the characters used in gen2 (no unordinary characters for now)
	std::map<uint8_t, int> character_mapping;
	unsigned int number_of_pokemon = 251;

	std::vector<std::string> load_pokemon_names();
	std::string read_pokemon_name(unsigned int offset, unsigned int length);
	void write_string(unsigned int offset, unsigned int length, std::string text);
	std::vector<Pokemon> pokemon;
	void populate_pokemon();
	void populate_character_mapping();

};
