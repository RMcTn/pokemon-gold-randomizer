#include "pokemon.h"

#include <fstream>
#include <vector>
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
	unsigned int number_of_pokemon = 251;

	std::vector<std::string> load_pokemon_names();
	std::string read_pokemon_name(unsigned int offset, unsigned int length);
	void write_string(unsigned int offset, unsigned int length, std::string text);
	std::vector<Pokemon> pokemon;
	void populate_pokemon();

};
