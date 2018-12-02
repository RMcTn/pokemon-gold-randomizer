#include "rom.h"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <sstream>

Rom::Rom() {
}

void Rom::run() {
	populate_character_mapping();
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
		//TODO: Maybe print pokemon's type here like it does in the game
		write_string(STARTER_TEXT_POSITIONS[i], poke.get_name() += "?", true);
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
		if (curr_char == GB_STRING_TERMINATOR)
			break;
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

void Rom::write_string(unsigned int offset, std::string text, bool add_terminator /*=false*/) {
	//Seems to be a problem with max length strings, that is doesn't stop them
	//TODO: This crashes if it writes sandshrew's name
	std::string translated = translate_string_to_game(text);
	int i = 0;
	for (uint8_t ch : translated) {
		rom[offset + i] = ch;
		i++;
	}
	if (add_terminator) {
		rom[offset + i] = GB_END_OF_TEXT;
	}
}

void Rom::populate_character_mapping() {
	std::ifstream map_file("mappings/gen2_english", std::ios::in);
	if (!map_file.is_open()) {
		std::cout << "Could not open gen2_english file\n";
		exit(EXIT_FAILURE);
	}
	uint8_t character;
	std::string hex_encoding;
	int line = 1;
	while (map_file >> character >> hex_encoding) {
		try {
			int hex_value = std::stoi(hex_encoding, nullptr, 16);
			if (hex_value > UINT8_MAX) {
				std::ostringstream err_msg;
				err_msg << "Hex value in gen2_english at line " << line << " is too large. Max is " << UINT8_MAX;
				throw std::out_of_range(err_msg.str());
			}
			character_mapping.insert(std::pair<uint8_t, int>(character, hex_value));
		} catch (std::invalid_argument& e) {
			std::cout << "Invalid hex value in gen2_english file at line " << line << "\n";
			std::cout << "No randomization will take place\n";
			exit(EXIT_FAILURE);
		} catch (std::out_of_range& e) {
			std::cout << e.what() << "\n";
			exit(EXIT_FAILURE);
		}
	}
}

std::string Rom::translate_string_to_game(const std::string text) {
	std::string translated;		//Maybe this should be a vector of chars or something instead
	for (uint8_t ch : text) {
		auto element = character_mapping.find(ch);
		if (element == character_mapping.end()) {
			//No conversion, just add the character
			translated.push_back(ch);
			continue;
		}

		translated.push_back(element->second);
	}
	return translated;
}
