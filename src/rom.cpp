#include "rom.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>

Rom::Rom() {
}

void Rom::randomize_starters(int seed /*=0*/) {
	std::srand(seed);		
	//Starter positons in memory for pokemon Gold
	unsigned int STARTER_POSITIONS[]= {0x1800D2, 0x1800D4, 0x1800EB, 0x1800F6};
	uint8_t pokemonID = std::rand() % UINT8_MAX;
	for (unsigned int position : STARTER_POSITIONS) {
		rom[position] = pokemonID;
	}
	/*
 	 * TODO: Need to change game text to match randomized pokemon
	 * Will need a way to store each pokemon's id, name etc
 	 */	

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
	return true;
}

bool Rom::save() {
	file.open("gold.gbc", std::ios::out | std::ios::binary);	
	if (!file.is_open()) return false;
	//Standard says vector is contiguous memory, get the start address of the vector and write it to the file
	file.write((char*) &rom[0], sizeof(uint8_t) * rom.size());
	file.close();
	return true;
}
