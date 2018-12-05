#include "rom.h"

#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <sstream>
#include <ctime>

Rom::Rom() {
	seed = std::time(nullptr);	
	std::srand(seed);		
}

Rom::Rom(int seed) {
	this->seed = seed;
	std::srand(seed);		
}

void Rom::run() {
	populate_character_mapping();
	populate_pokemon();
	randomize_intro_pokemon();
	randomize_starters();

	randomize_land_encounters(land_offset_johto);
	randomize_land_encounters(land_offset_kanto);
	randomize_water_encounters(water_offset_johto);
	randomize_water_encounters(water_offset_kanto);
	randomize_fishing_encounters();
	return;
}

void Rom::randomize_starters() {
	//Starter positons in memory for pokemon Gold
	unsigned int const STARTER_POSITIONS[3][4] = {
		{0x1800D2, 0x1800D4, 0x1800EB, 0x1800F6},		//Cyndaquil
		{0x180114, 0x180116, 0x18012D, 0x180138},		//Totodile
		{0x180150, 0x180152, 0x180169, 0x180174}};		//Chikorita
	unsigned int const STARTER_TEXT_POSITIONS[] = {0x1805F4, 0x180620, 0x18064D};
	for (int i = 0; i < 3; i++) {
		uint8_t pokemonID = std::rand() % number_of_pokemon;
		for (unsigned int position : STARTER_POSITIONS[i]) {
			//TODO: Noticed a weird sprite when unown was picked. Look into it
			rom[position] = pokemonID;
		}	
		Pokemon poke = pokemon[pokemonID - 1];
		//TODO: Maybe print pokemon's type here like it does in the game
		write_string(STARTER_TEXT_POSITIONS[i], poke.get_name() += "?", true);
	}

}

void Rom::randomize_intro_pokemon() {
	
		uint8_t pokemonID = std::rand() % number_of_pokemon;
		const int INTRO_POKEMON_POSITION = 0x5FDE;
		const int INTRO_POKEMON_CRY_POSITION = 0X6061;
		rom[INTRO_POKEMON_POSITION] = pokemonID;
		rom[INTRO_POKEMON_CRY_POSITION] = pokemonID;

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
		names.push_back(read_string(NAMES_OFFSET + (i * NAMES_LENGTH), NAMES_LENGTH));
	}
	return names;	
}

void Rom::populate_pokemon() {
	std::vector<std::string> names = load_pokemon_names();
	for (unsigned int i = 0; i < number_of_pokemon; i++) {
		Pokemon new_pokemon = Pokemon(i + 1, names[i]);		
		pokemon.push_back(new_pokemon);
	}
}

void Rom::write_string(unsigned int offset, std::string text, bool add_terminator /*=false*/) {
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

void Rom::randomize_land_encounters(int offset) {
	std::srand(seed);
	//TODO: Add option to keep randomization across time cycles
	//Gen 2 has morning, day and night cycles, need to randomize all 3 for each area
	//Map sections seem to end at 0xFF
	while (rom[offset] != 0xFF) {
		//First 5 bytes of each area seems to just be information we don't need to use. These bytes can be skipped for each area, unless they are logged for randomization output file
		const int info_offset = 5;
		//Each entry for the pokemon in an area is two bytes: 1st byte is the level of the pokemon, 2nd byte is the pokemon ID
		const int level_offset = 1;
		const int pokemon_bytes = 2;
		const int land_encounters_number = 7;
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < land_encounters_number; j++) {
				int time_of_day_offset = i * land_encounters_number * 2;
				//Multiply by two here since each pokemon entry is two bytes (see above)
				int pokemon_offset = j * 2;
				uint8_t pokemonID = std::rand() % number_of_pokemon;
				rom[offset + info_offset + time_of_day_offset + pokemon_offset + level_offset] = pokemonID;

			}
		}
		//pokemon_bytes * 3 is the size of each pokemon entry, multiplied by the number of time states (morning, day, night)
		//Then multiplied further by the amount of encounters there can be
		const int area_bytes = info_offset + ((pokemon_bytes * 3) * land_encounters_number);
		offset += area_bytes;
	}
}

void Rom::randomize_water_encounters(int offset) {
	std::srand(seed);
	//TODO: Refactor this with other area encounter randomizes, a lot of similar code
	while (rom[offset] != 0xFF) {
		const int water_encounters_number = 3;
		const int info_offset = 3;
		//Each entry for the pokemon in an area is two bytes: 1st byte is the level of the pokemon, 2nd byte is the pokemon ID
		const int level_offset = 1;
		const int pokemon_bytes = 2;
		for (int j = 0; j < water_encounters_number; j++) {
			//Multiply by two here since each pokemon entry is two bytes (see above)
			int pokemon_offset = j * 2;
			uint8_t pokemonID = std::rand() % number_of_pokemon;
			rom[offset + info_offset + pokemon_offset + level_offset] = pokemonID;

		}

		const int area_bytes = info_offset + (pokemon_bytes * water_encounters_number);
		offset += area_bytes;
	}
}

void Rom::randomize_fishing_encounters() {
	//TODO: Need to test this in Kanto
	//TODO: Check if fishing has time cycle differences
	std::srand(seed);
	const int fishing_offset = 0x92A52;
	//First pokemon id first after this offset
	//Byte format goes:
	//pokemon id, level, something(?)
	int offset = fishing_offset + 1;
	const int fishing_group_count = 12;
	for (int i = 0; i < fishing_group_count; i++) {
		const int pokemon_per_fishing_group = 11;
		for (int j = 0; j < pokemon_per_fishing_group; j++) {
			uint8_t pokemonID = std::rand() % number_of_pokemon;
			rom[offset] = pokemonID;
			//+2 to pass over level and extra byte
			offset += 2;
		}
	}
}

std::string Rom::read_string(int offset, int length) {
	std::string line;
	uint8_t ch;
	for (int i = 0; i < length; i++) {
		ch = rom[offset + i];
		if (ch == GB_STRING_TERMINATOR) {
			break;
		}
		line.push_back(ch);
	}
	return line;
}
