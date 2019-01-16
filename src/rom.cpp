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
	populate_items();

	randomize_intro_pokemon();
	randomize_starters();
	randomize_land_encounters(land_offset_johto);
	randomize_land_encounters(land_offset_kanto);
	randomize_water_encounters(water_offset_johto);
	randomize_water_encounters(water_offset_kanto);
	randomize_fishing_encounters();
	randomize_trainers();
	randomize_gift_pokemon();
	randomize_static_pokemon();
	randomize_game_corner_pokemon();
	return;
}

void Rom::randomize_starters() {
	//TODO: Randomize held items of starters
	//Starter positons in memory for pokemon Gold
	//First location is the sprite that shows when selecting the ball
	//Second location is the noise the pokemon will make
	//Third location is pokemon name shown in "player recieved ..." text
	//Fourth location is last mention of pokemon in text, and also what will appear in your party
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
	unsigned int const starter_item_positions[3] = {
		0x1800F8,		//Cyndaquil
		0x18013A,		//Totodile
		0x180176};		//Chikorita
	//Randomize starter's items
	for (int i = 0; i < 3; i++) {
		uint8_t itemID = std::rand() % 255;
		rom[starter_item_positions[i]] = itemID;	
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
	unsigned int MAX_NAMES_LENGTH = 10;
	std::vector<std::string> names;
	std::vector<uint8_t> characters;
	for (unsigned int i = 0; i < number_of_pokemon; i++) {
		names.push_back(read_string(NAMES_OFFSET + (i * MAX_NAMES_LENGTH), MAX_NAMES_LENGTH));
	}
	return names;	
}

std::vector<std::string> Rom::load_item_names() {
	//Maybe use a map for seperate item types like key items and normal items
	//Would be easier for allowed items
	const unsigned int item_names_offset = 0x1B0000;
	//12 characters and the terminator char
	const unsigned int max_item_name_length = 13;
	//This number includes "empty" items and filler items, may need to reword
	const int number_of_items = 255;

	std::vector<std::string> names;
	for (int i = 0; i < number_of_items; i++) {
		names.push_back(read_string(item_names_offset + i, max_item_name_length));
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

void Rom::populate_items() {
	std::vector<std::string> names = load_item_names();
	const int number_of_items = 255;
	for (unsigned int i = 0; i < number_of_items; i++) {
		Item new_item = Item(i + 1, names[i]);		
		items.push_back(new_item);
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

//read_string will stop when it hits a terminator character
std::string Rom::read_string(int offset, int max_length) {
	std::string line;
	uint8_t ch;
	for (int i = 0; i < max_length; i++) {
		ch = rom[offset + i];
		if (ch == GB_STRING_TERMINATOR) {
			break;
		}
		line.push_back(ch);
	}
	return line;
}

void Rom::randomize_trainers() {
	//TODO: Need to randomize rivals pokemon to match the right pokemon depending on what player chooses
	//TODO: May need to randomize moves as well to be appropriate to the pokemon randomized too, not sure yet

	const int trainer_class_number = 0x42;

	const int trainer_offset = 0x399C2;
	int offset = trainer_offset;
	//Number of trainers in each class
	const int trainer_class_amounts[] = {1, 1, 1, 1, 1, 1, 1, 1, 15, 0, 1, 3, 1, 1, 1, 1, 1, 1, 1, 5, 1, 12, 18, 19, 15, 1, 19, 20, 16, 13, 31, 5, 2, 3, 1, 14, 22, 21, 19, 12, 12, 6, 2, 20, 9, 1, 3, 8, 5, 9, 4, 12, 21, 19, 2, 9, 7, 3, 12, 6, 8, 5, 1, 1, 2, 5};
	for (int i = 0; i < trainer_class_number; i++) {
		int limit = trainer_class_amounts[i];
		for (int trainer_num = 0; trainer_num < limit; trainer_num++) {
			std::string name = read_string(offset, UINT8_MAX);	//Just using UINT8_MAX as a length isn't needed. Maybe have a read function that doesn't need a length?
			offset += name.length() + 1;	//Include terminating character in offset change
			//Need to check custom move marker. See https://bulbapedia.bulbagarden.net/wiki/Trainer_data_structure_in_Generation_II
			bool has_custom_moves = rom[offset] & 1;
			bool has_custom_held_item = rom[offset] & 2;
			offset++;
			//0xFF is end of the trainer
			while (rom[offset] != 0xFF) {
				//Level at rom[offset], pokemonID at rom[offset + 1]
				int new_id = std::rand() % number_of_pokemon;
				rom[offset + 1] = new_id;
				offset += 2;

				if (has_custom_held_item) {
					//TODO: Randomize held item
					offset++;
				}

				if (has_custom_moves) {
					//TODO: Randomize moves
					//1 byte per move
					offset += 4;

				}

			}
		}

	}
}

void Rom::randomize_gift_pokemon() {
	//https://www.serebii.net/gs/gift.shtml for a list of gift pokemon in gold
	std::vector<int> gift_pokemon_locations;
	const int spearow_mem_locations = 0x1599FC;	//This spearow is nicknamed 'Kenya', at goldenrod
	const int eevee_mem_locations = 0x15CC10;	//Eevee at goldenrod
	const int togepi_mem_locations = 0x15924F;	//Mystery egg togepi
	const int shuckle_mem_locations = 0x73E6;	//This shuckle is nicknamed 'Shuckie', at cianwood city
	const int tyrogue_mem_locations = 0x119F20;	//Tyrogue at mt mortar
	gift_pokemon_locations.push_back(spearow_mem_locations);
	gift_pokemon_locations.push_back(eevee_mem_locations);
	gift_pokemon_locations.push_back(togepi_mem_locations);
	gift_pokemon_locations.push_back(shuckle_mem_locations);
	gift_pokemon_locations.push_back(tyrogue_mem_locations);
	
	for (int location: gift_pokemon_locations) {
		uint8_t pokemonID = std::rand() % number_of_pokemon;
		rom[location] = pokemonID;
	}
}

void Rom::randomize_game_corner_pokemon() {
	//Pokemon rewards from the game corner
	//Goldenrod city prizes (Johto)
	const std::vector<int> abra_mem_locations {0x15E8B7, 0x15E8C8, 0x15E8CD, 0x15E93D}; 
	const std::vector<int> ekans_mem_locations {0x15E8E5, 0x15E8F6, 0x15E8FB, 0x15E94D}; 
	const std::vector<int> dratini_mem_locations {0x15E913, 0x15E924, 0x15E929, 0x15E95D}; 
	//Celadon city prizes (Kanto)
	const std::vector<int> mr_mime_mem_locations {0x179B9C, 0x179BAD, 0x179BB2, 0x179C22}; 
	const std::vector<int> eevee_mem_locations {0x179BCA, 0x179BDB, 0x179BE0, 0x179C32}; 
	const std::vector<int> porygon_mem_locations {0x179BF8, 0x179C09, 0x179C0E, 0x179C42}; 

	std::vector<std::vector<int>> game_corner_pokemon_locations {
		abra_mem_locations, 
		ekans_mem_locations, 
		dratini_mem_locations, 
		mr_mime_mem_locations, 
		eevee_mem_locations, 
		porygon_mem_locations}; 

	for (std::vector<int> pokemon_locations: game_corner_pokemon_locations) {
		uint8_t pokemonID = std::rand() % number_of_pokemon;
		Pokemon poke = pokemon[pokemonID - 1];
		int count = 0;
		for (int location: pokemon_locations) {
			if (count == static_cast<int>(pokemon_locations.size() - 1)) {
				//Write the name that appears in the game corner text
				//TODO: Short names wont overwrite the original pokemon's name, need to pad this some way
				write_string(location, poke.get_name(), false);
				break;
			}
			rom[location] = pokemonID;
			count++;
		}
	}
}






void Rom::randomize_static_pokemon() {
	//https://www.serebii.net/gs/interactable.shtml for a list of stationary pokemon in gold
	std::vector<std::vector<int>> static_pokemon_locations;

	const std::vector<int> sudowoodo_mem_locations = {0x12E1D6};
	static_pokemon_locations.push_back(sudowoodo_mem_locations);

	//Static pokemon that make noises
	//First location is sound made when activated, second location is what pokemon the encounter will be
	const std::vector<int> gyarados_mem_locations = {0x124F76, 0x124F7A};
	const std::vector<int> lapras_mem_locations = {0x111772, 0x111775};
	const std::vector<int> snorlax_mem_locations = {0x13D2A4, 0x13D2AB};
	const std::vector<int> hooh_mem_locations = {0x16E929, 0x16E919};
	//TODO: Test lugia's locations
	const std::vector<int> lugia_mem_locations = {0x11C1B6, 0x11C1A6};
	const std::vector<int> entei_mem_locations = {0x1093E3, 0x2A7DD};
	const std::vector<int> raikou_mem_locations = {0x1093D5, 0x2A7D8};
	const std::vector<int> suicine_mem_locations = {0x1093F1, 0x2A7E2};
	const std::vector<int> electrode1_mem_locations = {0x114DBA, 0x114DBD};
	const std::vector<int> electrode2_mem_locations = {0x114DE5, 0x114DE8};
	const std::vector<int> electrode3_mem_locations = {0x114E10, 0x114E13};
	static_pokemon_locations.push_back(gyarados_mem_locations);
	static_pokemon_locations.push_back(lapras_mem_locations);
	static_pokemon_locations.push_back(snorlax_mem_locations);
	static_pokemon_locations.push_back(hooh_mem_locations);
	static_pokemon_locations.push_back(lugia_mem_locations);
	static_pokemon_locations.push_back(entei_mem_locations);
	static_pokemon_locations.push_back(raikou_mem_locations);
	static_pokemon_locations.push_back(suicine_mem_locations);
	//Electrodes at power plant
	static_pokemon_locations.push_back(electrode1_mem_locations);
	static_pokemon_locations.push_back(electrode2_mem_locations);
	static_pokemon_locations.push_back(electrode3_mem_locations);

	for (std::vector<int> pokemon_locations: static_pokemon_locations) {
		uint8_t pokemonID = std::rand() % number_of_pokemon;
		for (int location: pokemon_locations) {
			rom[location] = pokemonID;
		}
	}
}
