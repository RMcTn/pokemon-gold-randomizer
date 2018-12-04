#include "pokemon.h"

#include <fstream>
#include <vector>
#include <map>

class Rom {

public:
	Rom();
	Rom(int seed);
	void randomize_starters();
	void randomize_intro_pokemon();
	void randomize_land_encounters();
	void randomize_water_encounters();
	bool load();
	bool save();
	void run();	//Just a driver function whilst functionality is being built

private:
	int seed;
	std::fstream file;
	std::vector<uint8_t> rom;
	//Mapping of english characters to the characters used in gen2 (no unordinary characters for now)
	std::map<uint8_t, int> character_mapping;
	unsigned int number_of_pokemon = 251;
	const uint8_t GB_END_OF_TEXT = 0x57;
	const uint8_t GB_STRING_TERMINATOR = 0x50;
	std::vector<std::string> load_pokemon_names();
	std::string read_pokemon_name(unsigned int offset, unsigned int length);
	void write_string(unsigned int offset, std::string text, bool add_terminator = false);
	std::vector<Pokemon> pokemon;
	void populate_pokemon();
	void populate_character_mapping();
	std::string translate_string_from_game(const std::string text);
	std::string translate_string_to_game(const std::string text);

};
