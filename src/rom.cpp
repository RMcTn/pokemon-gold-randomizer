#include "rom.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

Rom::Rom() {
}

void Rom::run() {
	populate_pokemon();
	return;
}

void Rom::randomize_starters(int seed /*=0*/) {
	std::srand(seed);		
	//Starter positons in memory for pokemon Gold
	unsigned int const STARTER_POSITIONS[3][4] = {
		{0x1800D2, 0x1800D4, 0x1800EB, 0x1800F6},		//Cyndaquil
		{0x180114, 0x180116, 0x18012D, 0x180138},		//Totodile
		{0x180150, 0x180152, 0x180169, 0x180174}};		//Chikorita
	unsigned int const STARTER_TEXT_POSITIONS[] = {0x1805F4, 0x180620, 0x18064D};
	for (int i = 0; i < 3; i++) {
		uint8_t pokemonID = std::rand() % UINT8_MAX;
		for (unsigned int position : STARTER_POSITIONS[i]) {
			rom[position] = pokemonID;
		}	
		Pokemon poke = pokemon[pokemonID - 1];
		write_string(STARTER_TEXT_POSITIONS[i], 10, poke.get_name());
	}

}

bool Rom::load() {
	file.open("gold.gbc", std::fstream::in | std::ios::binary);
	if (!file.is_open()) return false;

	file.seekg(0, std::ios::end);
	unsigned int length = file.tellg();
	file.seekg(0, std::ios::beg);
	//TODO: Ensure file length is correct
	rom.insert(rom.begin(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
	file.close();
	load_pokemon_names();
	return true;
}

bool Rom::save() {
	file.open("gold randomized.gbc", std::ios::out | std::ios::binary);	
	if (!file.is_open()) return false;
	//Standard says vector is contiguous memory, get the start address of the vector and write it to the file
	file.write((char*) &rom[0], sizeof(uint8_t) * rom.size());
	file.close();
	return true;
}

std::vector<std::string> Rom::load_pokemon_names() {
	unsigned int NAMES_OFFSET = 0x1B0B74;
	unsigned int NAMES_LENGTH = 10;
	std::vector<std::string> names;
	std::vector<uint8_t> characters;
	for (unsigned int i = 0; i < number_of_pokemon; i++) {
		names.push_back(read_pokemon_name(NAMES_OFFSET + (i * NAMES_LENGTH), NAMES_LENGTH));
	}
	return names;	
}

std::string Rom::read_pokemon_name(unsigned int offset, unsigned int length) {
	std::string name;
	for (unsigned int i = 0; i < length; i++) {
		uint8_t curr_char = rom[offset + i];
		name.push_back(curr_char);
	}	
	return name;
}

void Rom::populate_pokemon() {
	std::vector<std::string> names = load_pokemon_names();
	for (unsigned int i = 0; i < number_of_pokemon; i++) {
		Pokemon new_pokemon = Pokemon(i + 1, names[i]);		
		pokemon.push_back(new_pokemon);
	}
}

void Rom::write_string(unsigned int offset, unsigned int length, std::string text) {
	//Seems to be a problem with max length strings, that is doesn't stop them
	//TODO: This crashes if it writes sandshrew's name
	for (unsigned int i = 0; i < length; i++) {
		rom[offset + i] = text[i];
	}
}
