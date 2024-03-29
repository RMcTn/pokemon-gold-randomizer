#include "rom.h"
#include "constants.h"
#include "randomization_options.h"

#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <ctime>
#include <algorithm>
#include <random>
#include <cassert>

// TODO: Add logging

Rom::Rom() : items(number_of_items) {
    seed = std::time(nullptr);
    rng = std::mt19937(seed);
}

Rom::Rom(int seed) : items(number_of_items) {
    this->seed = seed;
    rng = std::mt19937(seed);
}

void Rom::run(std::vector<RandomizationOptions> &randomization_options) {
    populate_character_mapping_no_load();
    populate_pokemon();
    populate_items();
    items.populate_allowed_items(load_banned_items());

    // TODO: Override trade evolving pokemon
    // TODO: Add flags for each option


    if (randomization_options.size() == 0) {
	    randomize_intro_pokemon();
	    randomize_starters(); // TODO: Allow choosing of starters
	    randomize_land_encounters(land_offset_johto);
	    randomize_land_encounters(land_offset_kanto);
	    randomize_water_encounters(water_offset_johto);
	    randomize_water_encounters(water_offset_kanto);
	    randomize_fishing_encounters();
	    randomize_trainers();
	    randomize_gift_pokemon();
	    randomize_static_pokemon();
	    randomize_game_corner_pokemon();
	    randomize_evolutions();
	    randomize_static_item_locations();
    } else {
    	    // Not the most efficient, but the sun will still rise
    	    for (auto option: randomization_options) {
		if (option == RANDOMIZE_INTRO_POKEMON) {
			randomize_intro_pokemon();
		}
		if (option == RANDOMIZE_STARTER_POKEMON) {
			randomize_starters();
		}
		if (option == RANDOMIZE_WILD_POKEMON) {
			randomize_land_encounters(land_offset_johto);
			randomize_land_encounters(land_offset_kanto);
			randomize_water_encounters(water_offset_johto);
			randomize_water_encounters(water_offset_kanto);
			randomize_fishing_encounters();
		}
		if (option == RANDOMIZE_TRAINERS) {
			randomize_trainers();
		}
		if (option == RANDOMIZE_GIFT_POKEMON) {
			randomize_gift_pokemon();
		}
		if (option == RANDOMIZE_STATIC_POKEMON) {
			randomize_static_pokemon();
		}
		if (option == RANDOMIZE_GAME_CORNER_POKEMON) {
			randomize_game_corner_pokemon();
		}
		if (option == RANDOMIZE_EVOLUTIONS) {
			randomize_evolutions();
		}
		if (option == RANDOMIZE_STATIC_ITEMS) {
			randomize_static_item_locations();
		}
		if (option == ENABLE_SHINY_MODE) {
			enable_shiny_mode();
		}
		if (option == RANDOMIZE_POKEMON_PALLETES) {
			randomize_pokemon_palettes();
		}
    	    }
    }
    // TODO: Randomize trade pokemon
//    shuffle_stats(); // TODO: Add flag for this
}

void Rom::randomize_starters() {
    //Starter positons in memory for pokemon Gold
    //First location is the sprite that shows when selecting the ball
    //Second location is the noise the pokemon will make
    //Third location is pokemon name shown in "player recieved ..." text
    //Fourth location is last mention of pokemon in text, and also what will appear in your party
    unsigned int const STARTER_POSITIONS[3][4] = {
            {0x1800D2, 0x1800D4, 0x1800EB, 0x1800F6},        //Cyndaquil
            {0x180114, 0x180116, 0x18012D, 0x180138},        //Totodile
            {0x180150, 0x180152, 0x180169, 0x180174}};        //Chikorita
    unsigned int const STARTER_TEXT_POSITIONS[] = {0x1805F4, 0x180620, 0x18064D};
    for (int i = 0; i < 3; i++) {
        std::uniform_int_distribution<unsigned int> distribution(1, number_of_pokemon);
        uint8_t pokemonID = distribution(rng);
        for (unsigned int position : STARTER_POSITIONS[i]) {
            //TODO: Noticed a weird sprite when unown was picked. Look into it
            rom[position] = pokemonID;
        }
        Pokemon poke = pokemon[pokemonID - 1];
        //TODO: Maybe print pokemon's type here like it does in the game
        write_string(STARTER_TEXT_POSITIONS[i], poke.get_name() += "?", true);
    }
    //TODO: Maybe move randomizing starters items to a seperate function
    unsigned int const starter_item_positions[3] = {
            0x1800F8,        //Cyndaquil
            0x18013A,        //Totodile
            0x180176};        //Chikorita
    //Randomize starter's items
    for (unsigned int starter_item_position : starter_item_positions) {
        Item item = items.random_allowed_item();
        rom[starter_item_position] = item.get_id();
    }

}

void Rom::randomize_intro_pokemon() {
    std::uniform_int_distribution<unsigned int> distribution(1, number_of_pokemon);
    uint8_t pokemonID = distribution(rng);
    const int INTRO_POKEMON_POSITION = 0x5FDE;
    const int INTRO_POKEMON_CRY_POSITION = 0X6061;
    rom[INTRO_POKEMON_POSITION] = pokemonID;
    rom[INTRO_POKEMON_CRY_POSITION] = pokemonID;
}

void Rom::randomize_evolutions() {
    // TODO: Should it be possible for 2 pokemon to be able to evolve into the same thing?
    std::uniform_int_distribution<unsigned int> distribution(1, number_of_pokemon);
    for (Evolution &evolution : pokemon_evolutions) {
        uint8_t pokemonID = distribution(rng);
        evolution.pokemon_to_evolve_to = pokemonID;
        printf("%s now evolves to %s\n", translate_string_from_game(pokemon[evolution.pokemon - 1].get_name()).c_str(),
               translate_string_from_game(pokemon[evolution.pokemon_to_evolve_to - 1].get_name()).c_str());
    }

    const unsigned int base_evolutions_offset = 0x429B3;
    unsigned int evo_offset = base_evolutions_offset;
    // @NOTE: Look at populate_pokemon_evolutions for info on the memory for evolutions
    unsigned int current_evolution_index = 0;
    for (int i = 1; i <= number_of_pokemon; i++) {
        while (rom[evo_offset] != 0) {
            const Evolution &current_evolution = pokemon_evolutions[current_evolution_index];
            switch (current_evolution.evolution_type) {
                case EvolutionType::EVOLVE_LEVEL: {
                    // EVOLVE_LEVEL, level, species
                    rom[evo_offset + 2] = current_evolution.pokemon_to_evolve_to;
                    evo_offset += 3;
                    break;
                }
                case EvolutionType::EVOLVE_ITEM: {
                    // EVOLVE_ITEM, used item, species
                    rom[evo_offset + 2] = current_evolution.pokemon_to_evolve_to;
                    evo_offset += 3;
                    break;
                }
                case EvolutionType::EVOLVE_TRADE: {
                    // EVOLVE_TRADE, held item (or -1 for none), species
                    rom[evo_offset + 2] = current_evolution.pokemon_to_evolve_to;
                    evo_offset += 3;
                    break;
                }
                case EvolutionType::EVOLVE_HAPPINESS: {
                    // EVOLVE_HAPPINESS, TR_* constant (ANYTIME, MORNDAY, NITE), species
                    rom[evo_offset + 2] = current_evolution.pokemon_to_evolve_to;
                    evo_offset += 3;
                    break;
                }
                case EvolutionType::EVOLVE_STAT: {
                    // EVOLVE_STAT, level, ATK_*_DEF constant (LT, GT, EQ), species
                    rom[evo_offset + 3] = current_evolution.pokemon_to_evolve_to;
                    evo_offset += 4;
                    break;
                }

            }
        }
        evo_offset++; // move by 1 to skip the 0
        // skip over move learnset by finding next 0
        while (rom[evo_offset] != 0) {
            evo_offset++;
        }
        evo_offset++;
        current_evolution_index++;
        if (current_evolution_index >= pokemon_evolutions.size()) {
            break;
        }
        // do evo stuff again
    }
}

void Rom::randomize_map_items(unsigned int map_offset) {
    /*
     // See https://datacrystal.romhacking.net/wiki/Pok%C3%A9mon_Gold_and_Silver:Notes
        SECONDARY MAP HEADER
        1 byte  - ID of block displayed "outside" of map
        1 byte  - Map height
        1 byte  - Map width
        1 byte  - Bank of block data
        2 bytes - Pointer to block data (little-endian)
        1 byte  - Bank of scripts and events headers
        2 bytes - Pointer to scripts header
        2 bytes - Pointer to events header
        1 byte  - Bitfield of which map connections are enabled
        Bits : 0 - 0 - 0 - 0 - NORTH - SOUTH - WEST - EAST

        // See https://hax.iimarckus.org/files/scriptingcodes_eng.htm for event structure
     */
    std::uniform_int_distribution<unsigned int> distribution(0, items.allowed_items_size() - 1);
    // TODO: Keep note of what maps we're processing/have processed
    unsigned int map_id = rom[map_offset + 5];
    unsigned int secondary_header_bank = rom[map_offset];
    unsigned int secondary_header_pointer =
            rom[map_offset + 3] |
            (rom[map_offset + 4] << 8);
    unsigned int secondary_header_offset =
            (secondary_header_pointer % GAMEBOY_BANK_SIZE) +
            secondary_header_bank * GAMEBOY_BANK_SIZE;
    // Use secondary header offset to get event header offset (see above for memory layout)

    unsigned int event_header_bank = rom[secondary_header_offset + 6];
    unsigned int event_header_pointer =
            rom[secondary_header_offset + 9] |
            (rom[secondary_header_offset + 10] << 8);
    unsigned int event_header_offset =
            (event_header_pointer % GAMEBOY_BANK_SIZE) +
            event_header_bank * GAMEBOY_BANK_SIZE;
    // Skip filler
    event_header_offset += 2;
    const int warp_count = rom[event_header_offset];
    event_header_offset++;
    const int warp_size = 5;
    event_header_offset += warp_count * warp_size;

    const int trigger_count = rom[event_header_offset];
    event_header_offset++;
    const int trigger_size = 8;
    event_header_offset += trigger_count * trigger_size;

    const int signpost_count = rom[event_header_offset];
    event_header_offset++;
    const int hidden_item_type = 7;
    const int signpost_size = 5;
    // Hidden items are treated as signposts
    for (int signpost = 0; signpost < signpost_count; signpost++) {
        int signpost_type = rom[event_header_offset + (signpost * signpost_size) + 2];
        // See https://hax.iimarckus.org/files/scriptingcodes_eng.htm Events signpost section
        if (signpost_type == hidden_item_type) {
            unsigned int signpost_pointer =
                    rom[event_header_offset + signpost * signpost_size + 3] |
                    (rom[event_header_offset + signpost * signpost_size + 4] << 8);
            unsigned int signpost_offset =
                    (signpost_pointer % GAMEBOY_BANK_SIZE) +
                    event_header_bank * GAMEBOY_BANK_SIZE;

            rom[signpost_offset + 2] = items.allowed_items[distribution(rng)].get_id();

        }
    }

    event_header_offset += signpost_count * signpost_size;

    const int people_count = rom[event_header_offset];
    event_header_offset++;
    const int people_size = 13;
    // Visible pokeballs on the ground are treated as people events;
    for (int person = 0; person < people_count; person++) {
        const int person_color_and_function_info = rom[event_header_offset + person * people_size + 7];
        // bottom bit of color and function info determines if an item is given or not
        // See https://hax.iimarckus.org/files/scriptingcodes_eng.htm People events section
        if ((person_color_and_function_info & 1) == 1) {
            unsigned int person_pointer =
                    rom[event_header_offset + person * people_size + 9] |
                    (rom[event_header_offset + person * people_size + 10] << 8);
            unsigned int person_offset = (person_pointer % GAMEBOY_BANK_SIZE) +
                                         event_header_bank * GAMEBOY_BANK_SIZE;
            rom[person_offset] = items.allowed_items[distribution(rng)].get_id();
        }
    }
}

void Rom::randomize_static_item_locations() {
    /*
    // https://datacrystal.romhacking.net/wiki/Pok%C3%A9mon_Gold_and_Silver:ROM_map#Map_Pointers
     // See https://datacrystal.romhacking.net/wiki/Pok%C3%A9mon_Gold_and_Silver:Notes
     // See https://hax.iimarckus.org/files/scriptingcodes_eng.htm

        PRIMARY MAP HEADER
        1 byte  - Bank of second part
        1 byte  - Tileset ID
        1 byte  - Permission (?)
        2 bytes - Pointer to secondary header (little-endian)
        1 byte  - Location ID
        1 byte  - Music ID
        1 byte  - Upper nibble = 1 if no phone signal
                  Lower nibble = Day/Night-type palette
        1 byte  - Fishing group


        // TODO: Could randomize music ID for each map as well
     */


    const unsigned int map_banks_offset = 0x940ED;
    const unsigned int map_group_count = 26;

    std::vector<unsigned int> map_primary_header_offsets;
    unsigned int map_primary_headers_bank = map_banks_offset / GAMEBOY_BANK_SIZE;
    for (int i = 0; i < map_group_count; i++) {
        unsigned int current_map_primary_header_pointer = (rom[map_banks_offset + (i * 2)]) |
                                                          (rom[map_banks_offset + (i * 2) + 1] << 8);
        unsigned int current_map_primary_header_offset = (current_map_primary_header_pointer % GAMEBOY_BANK_SIZE) +
                                                         map_primary_headers_bank * GAMEBOY_BANK_SIZE;
        map_primary_header_offsets.push_back(current_map_primary_header_offset);
    }

    std::vector<unsigned int> map_ids;
    for (int map_group = 0; map_group < map_primary_header_offsets.size(); map_group++) {
        unsigned int map_primary_header_offset = map_primary_header_offsets[map_group];
        unsigned int max_map_offset;
        if (map_group + 1 >= map_primary_header_offsets.size()) {
            const int map_header_bank = map_banks_offset / GAMEBOY_BANK_SIZE;
            max_map_offset = (map_header_bank + 1) * GAMEBOY_BANK_SIZE;
        } else {
            max_map_offset = map_primary_header_offsets[map_group + 1];
        }

        const unsigned int maps_in_last_map_group = 11;
        unsigned int max_map = (map_group == map_group_count - 1) ? maps_in_last_map_group
                                                                  : std::numeric_limits<int>::max();
        unsigned int current_map = 0;

        while (map_primary_header_offset < max_map_offset && current_map < max_map) {

            // TODO: Populate a map of these map IDs to names so we can verify
            randomize_map_items(map_primary_header_offset);

            map_primary_header_offset += 9;
            current_map++;
        }


    }

}

void Rom::shuffle_stats() {
    std::shuffle(pokemon_stats.begin(), pokemon_stats.end(), rng);
    const unsigned int stats_offset = 0x51B0B;
    const unsigned int stats_step = 0x20;
    for (int i = 0; i < number_of_pokemon; i++) {
        unsigned int current_stats_offset = stats_offset + (i * stats_step);
        const PokemonStats &stats = pokemon_stats[i];
        pokemon[i].stats = stats;
        rom[current_stats_offset + 1] = stats.hp;
        rom[current_stats_offset + 2] = stats.attack;
        rom[current_stats_offset + 3] = stats.defence;
        rom[current_stats_offset + 4] = stats.speed;
        rom[current_stats_offset + 5] = stats.special_attack;
        rom[current_stats_offset + 6] = stats.special_defence;
    }
}

bool Rom::load(const std::string &romFilename) {
    file.open(romFilename, std::fstream::in | std::ios::binary);
    if (!file.is_open()) return false;

    file.seekg(0, std::ios::end);
    unsigned int length = file.tellg();
    file.seekg(0, std::ios::beg);
    //TODO: Ensure file length is correct
    rom.insert(rom.begin(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

    file.close();
    return true;
}

bool Rom::save(std::string filename_to_save) {
    if (filename_to_save.empty()) filename_to_save = "gold randomized.gbc";
    file.open(filename_to_save, std::ios::out | std::ios::binary);
    if (!file.is_open()) return false;
    file.write((char *) &rom[0], sizeof(uint8_t) * rom.size());
    file.close();
    return true;
}

std::vector<std::string> Rom::load_pokemon_names() {
    // TODO: Why return anything from this? Just set the pokemon's name from here?
    unsigned int NAMES_OFFSET = 0x1B0B74;
    unsigned int MAX_NAMES_LENGTH = 10;
    std::vector<std::string> names;
    std::vector<uint8_t> characters;
    for (unsigned int i = 0; i < number_of_pokemon; i++) {
        names.push_back(read_string(NAMES_OFFSET + (i * MAX_NAMES_LENGTH), MAX_NAMES_LENGTH));
    }
    return names;
}

void Rom::populate_pokemon_stats() {
    const unsigned int stats_offset = 0x51B0B;
    const unsigned int stats_step = 0x20;
    // pokedex id, hp, defence, speed, special attack,
    // special defence, then 14 bytes we don't really care about right now

    // TODO: Implement randomizing for stats
    for (Pokemon &current_pokemon : pokemon) {
        // TODO: @Cleanup for now we load into the pokemon directly and a stats array, may be
        // better overall for randomizing etc if we just keep each important thing in an array that maps
        // to the pokemon id
        const unsigned int current_stats_offset = stats_offset + ((current_pokemon.get_id() - 1) * stats_step);
        current_pokemon.stats.hp = rom[current_stats_offset + 1];
        current_pokemon.stats.attack = rom[current_stats_offset + 2];
        current_pokemon.stats.defence = rom[current_stats_offset + 3];
        current_pokemon.stats.speed = rom[current_stats_offset + 4];
        current_pokemon.stats.special_attack = rom[current_stats_offset + 5];
        current_pokemon.stats.special_defence = rom[current_stats_offset + 6];
        PokemonStats stats{};
        stats.hp = rom[current_stats_offset + 1];
        stats.attack = rom[current_stats_offset + 2];
        stats.defence = rom[current_stats_offset + 3];
        stats.speed = rom[current_stats_offset + 4];
        stats.special_attack = rom[current_stats_offset + 5];
        stats.special_defence = rom[current_stats_offset + 6];
        pokemon_stats.push_back(stats);

    }
}

std::vector<std::string> Rom::load_item_names() {
    const unsigned int item_names_offset = 0x1B0000;
    //12 characters and the terminator char
    const unsigned int max_item_name_length = 13;
    //This number includes "empty" items and filler items, may need to reword

    std::vector<std::string> names;
    int len;
    int offset = item_names_offset;
    for (int i = 0; i <= number_of_items; i++) {
        std::string name;
        len = read_string_and_length(offset, max_item_name_length, name);
        names.push_back(name);
        //+1 for terminator character
        offset += len + 1;
    }
    return names;
}

void Rom::populate_pokemon() {
    // TODO: Populate pokemon list first then fill in values for each populate function?
    std::vector<std::string> names = load_pokemon_names();
    for (unsigned int i = 0; i < number_of_pokemon; i++) {
        Pokemon new_pokemon = Pokemon(i + 1, names[i]);
        pokemon.push_back(new_pokemon);
    }
    populate_pokemon_stats();
    populate_pokemon_evolutions_and_moveslist();
}

void Rom::populate_pokemon_evolutions_and_moveslist() {
    const unsigned int base_evolutions_offset = 0x429B3;
    unsigned int evo_offset = base_evolutions_offset;
    // Evolutions and attacks are grouped together since they're both checked at level-up. from https://github.com/pret/pokegold/blob/master/data/pokemon/evos_attacks_pointers.asm;
    for (int i = 1; i <= number_of_pokemon; i++) {
        // evolution_type 0 means no more evolutions for that pokemon
        unsigned int evolution_type = rom[evo_offset];
        while (evolution_type != 0) {
            Evolution evolution{};
            evolution.pokemon = i;
            switch (evolution_type) {
                case EvolutionType::EVOLVE_LEVEL: {
                    // EVOLVE_LEVEL, level, species
                    unsigned int evolve_level = rom[evo_offset + 1];
                    unsigned int pokemon_to_evolve_to = rom[evo_offset + 2];
                    evolution.evolution_type = EvolutionType::EVOLVE_LEVEL;
                    evolution.level_to_evolve = evolve_level;
                    evolution.pokemon_to_evolve_to = pokemon_to_evolve_to;
                    evo_offset += 3;
                    break;
                }
                case EvolutionType::EVOLVE_ITEM: {
                    // EVOLVE_ITEM, used item, species
                    unsigned int item_id_to_evolve = rom[evo_offset + 1];
                    unsigned int pokemon_to_evolve_to = rom[evo_offset + 2];
                    evolution.evolution_type = EvolutionType::EVOLVE_ITEM;
                    evolution.item_id_to_evolve = item_id_to_evolve;
                    evolution.pokemon_to_evolve_to = pokemon_to_evolve_to;
                    evo_offset += 3;
                    break;
                }
                case EvolutionType::EVOLVE_TRADE: {
                    // EVOLVE_TRADE, held item (or -1 for none), species
                    unsigned int item_id_to_evolve = rom[evo_offset + 1];
                    unsigned int pokemon_to_evolve_to = rom[evo_offset + 2];
                    evolution.evolution_type = EvolutionType::EVOLVE_TRADE;
                    evolution.item_id_to_evolve = item_id_to_evolve;
                    evolution.pokemon_to_evolve_to = pokemon_to_evolve_to;
                    evo_offset += 3;
                    break;
                }
                case EvolutionType::EVOLVE_HAPPINESS: {
                    // EVOLVE_HAPPINESS, TR_* constant (ANYTIME, MORNDAY, NITE), species
                    unsigned int happiness_time = rom[evo_offset + 1];
                    unsigned int pokemon_to_evolve_to = rom[evo_offset + 2];
                    evolution.evolution_type = EvolutionType::EVOLVE_HAPPINESS;
                    evolution.happiness_condition = static_cast<EvolutionHappinessCondition>(happiness_time);
                    evolution.pokemon_to_evolve_to = pokemon_to_evolve_to;
                    evo_offset += 3;
                    break;
                }
                case EvolutionType::EVOLVE_STAT: {
                    // EVOLVE_STAT, level, ATK_*_DEF constant (LT, GT, EQ), species
                    unsigned int level_to_evolve = rom[evo_offset + 1];
                    unsigned int stat_condition = rom[evo_offset + 2];
                    unsigned int pokemon_to_evolve_to = rom[evo_offset + 3];
                    evolution.evolution_type = EvolutionType::EVOLVE_STAT;
                    evolution.level_to_evolve = level_to_evolve;
                    evolution.stat_condition = static_cast<EvolutionStatCondition>(stat_condition);
                    evolution.pokemon_to_evolve_to = pokemon_to_evolve_to;
                    evo_offset += 4;
                    break;
                }
                default:
                    std::cerr << "Something went wrong when loading in evolutions, evolution_type was "
                              << evolution_type << '\n';
                    break;
            }
            assert(evolution.pokemon_to_evolve_to != 0);
            pokemon_evolutions.push_back(evolution);
            evolution_type = rom[evo_offset];
        }
        evo_offset++; // move by 1 to skip the 0
        // populate pokemon's learnset
        std::vector<Move> current_pokemon_moveslist;
        while (rom[evo_offset] != 0) {
            // Move learnset (in increasing level order):
            // level, move
            Move move{};
            move.level_to_learn = rom[evo_offset];
            move.move_id = rom[evo_offset + 1];
            current_pokemon_moveslist.push_back(move);
            evo_offset += 2;
        }
        pokemon_movelists.push_back(current_pokemon_moveslist);
        evo_offset++;
        // do evo stuff again
    }
}

void Rom::populate_items() {
    std::vector<std::string> names = load_item_names();
    std::vector<Item> loaded_items;
    for (unsigned int i = 0; i <= number_of_items; i++) {
        Item new_item = Item(i, names[i]);
        loaded_items.push_back(new_item);
    }
    items.set_items(loaded_items);
}

void Rom::write_string(unsigned int offset, const std::string &text, bool add_terminator /*=false*/) {
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

void Rom::populate_character_mapping_no_load() {
	// Just hard code the mapping rather than loading it (Unless more games/languages were supported)
	// TODO check if this can be done at initializing rather than this monstrosity
	character_mapping.insert(std::pair<uint8_t, int>('A', 0x80));
	character_mapping.insert(std::pair<uint8_t, int>('A', 0x80));
	character_mapping.insert(std::pair<uint8_t, int>('B', 0x81));
	character_mapping.insert(std::pair<uint8_t, int>('C', 0x82));
	character_mapping.insert(std::pair<uint8_t, int>('D', 0x83));
	character_mapping.insert(std::pair<uint8_t, int>('E', 0x84));
	character_mapping.insert(std::pair<uint8_t, int>('F', 0x85));
	character_mapping.insert(std::pair<uint8_t, int>('G', 0x86));
	character_mapping.insert(std::pair<uint8_t, int>('H', 0x87));
	character_mapping.insert(std::pair<uint8_t, int>('I', 0x88));
	character_mapping.insert(std::pair<uint8_t, int>('J', 0x89));
	character_mapping.insert(std::pair<uint8_t, int>('K', 0x8A));
	character_mapping.insert(std::pair<uint8_t, int>('L', 0x8B));
	character_mapping.insert(std::pair<uint8_t, int>('M', 0x8C));
	character_mapping.insert(std::pair<uint8_t, int>('N', 0x8D));
	character_mapping.insert(std::pair<uint8_t, int>('O', 0x8E));
	character_mapping.insert(std::pair<uint8_t, int>('P', 0x8F));
	character_mapping.insert(std::pair<uint8_t, int>('Q', 0x90));
	character_mapping.insert(std::pair<uint8_t, int>('R', 0x91));
	character_mapping.insert(std::pair<uint8_t, int>('S', 0x92));
	character_mapping.insert(std::pair<uint8_t, int>('T', 0x93));
	character_mapping.insert(std::pair<uint8_t, int>('U', 0x94));
	character_mapping.insert(std::pair<uint8_t, int>('V', 0x95));
	character_mapping.insert(std::pair<uint8_t, int>('W', 0x96));
	character_mapping.insert(std::pair<uint8_t, int>('X', 0x97));
	character_mapping.insert(std::pair<uint8_t, int>('Y', 0x98));
	character_mapping.insert(std::pair<uint8_t, int>('Z', 0x99));
	character_mapping.insert(std::pair<uint8_t, int>('(', 0x9A));
	character_mapping.insert(std::pair<uint8_t, int>(')', 0x9B));
	character_mapping.insert(std::pair<uint8_t, int>(':', 0x9C));
	character_mapping.insert(std::pair<uint8_t, int>(';', 0x9D));
	character_mapping.insert(std::pair<uint8_t, int>('[', 0x9E));
	character_mapping.insert(std::pair<uint8_t, int>(']', 0x9F));
	character_mapping.insert(std::pair<uint8_t, int>('a', 0xA0));
	character_mapping.insert(std::pair<uint8_t, int>('b', 0xA1));
	character_mapping.insert(std::pair<uint8_t, int>('c', 0xA2));
	character_mapping.insert(std::pair<uint8_t, int>('d', 0xA3));
	character_mapping.insert(std::pair<uint8_t, int>('e', 0xA4));
	character_mapping.insert(std::pair<uint8_t, int>('f', 0xA5));
	character_mapping.insert(std::pair<uint8_t, int>('g', 0xA6));
	character_mapping.insert(std::pair<uint8_t, int>('h', 0xA7));
	character_mapping.insert(std::pair<uint8_t, int>('i', 0xA8));
	character_mapping.insert(std::pair<uint8_t, int>('j', 0xA9));
	character_mapping.insert(std::pair<uint8_t, int>('k', 0xAA));
	character_mapping.insert(std::pair<uint8_t, int>('l', 0xAB));
	character_mapping.insert(std::pair<uint8_t, int>('m', 0xAC));
	character_mapping.insert(std::pair<uint8_t, int>('n', 0xAD));
	character_mapping.insert(std::pair<uint8_t, int>('o', 0xAE));
	character_mapping.insert(std::pair<uint8_t, int>('p', 0xAF));
	character_mapping.insert(std::pair<uint8_t, int>('q', 0xB0));
	character_mapping.insert(std::pair<uint8_t, int>('r', 0xB1));
	character_mapping.insert(std::pair<uint8_t, int>('s', 0xB2));
	character_mapping.insert(std::pair<uint8_t, int>('t', 0xB3));
	character_mapping.insert(std::pair<uint8_t, int>('u', 0xB4));
	character_mapping.insert(std::pair<uint8_t, int>('v', 0xB5));
	character_mapping.insert(std::pair<uint8_t, int>('w', 0xB6));
	character_mapping.insert(std::pair<uint8_t, int>('x', 0xB7));
	character_mapping.insert(std::pair<uint8_t, int>('y', 0xB8));
	character_mapping.insert(std::pair<uint8_t, int>('z', 0xB9));
	character_mapping.insert(std::pair<uint8_t, int>('?', 0xE6));
}

void Rom::populate_character_mapping() {
// Really no reason to have this loaded from a file right now, pointless 'future proofing'
    std::ifstream map_file("../mappings/gen2_english", std::ios::in);
    if (!map_file.is_open()) {
        std::cout << "Could not open gen2_english file\n";
        exit(EXIT_FAILURE);
    }
    uint8_t character;
    std::string hex_encoding;
    int line = 1;
    while (map_file >> character >> hex_encoding) {
        // try {
            int hex_value = std::stoi(hex_encoding, nullptr, 16);
            if (hex_value > UINT8_MAX) {
            	    // TODO commented out since switch console can't handle exceptions
                // std::ostringstream err_msg;
                // err_msg << "Hex value in gen2_english at line " << line << " is too large. Max is " << UINT8_MAX;
                // throw std::out_of_range(err_msg.str());
            }
            character_mapping.insert(std::pair<uint8_t, int>(character, hex_value));
        // } catch (std::invalid_argument &e) {
        //     std::cout << "Invalid hex value in gen2_english file at line " << line << "\n";
        //     std::cout << "No randomization will take place\n";
        //     exit(EXIT_FAILURE);
        // } catch (std::out_of_range &e) {
        //     std::cout << e.what() << "\n";
        //     exit(EXIT_FAILURE);
        // }
    }
}

std::string Rom::translate_string_from_game(const std::string &text) {
    std::string translated;
    for (uint8_t ch : text) {
        bool found_mapping = false;
        for (const auto&[normal_char, game_mapped_char] : character_mapping) {
            if (game_mapped_char == ch) {
                translated.push_back(normal_char);
                found_mapping = true;
            }
        }
        if (!found_mapping) {
            // No conversion, just add the character
            translated.push_back(ch);
        }

    }
    return translated;
}

std::string Rom::translate_string_to_game(const std::string &text) {
    std::string translated;
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
                std::uniform_int_distribution<unsigned int> distribution(1, number_of_pokemon);
                uint8_t pokemonID = distribution(rng);
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
            std::uniform_int_distribution<unsigned int> distribution(1, number_of_pokemon);
            uint8_t pokemonID = distribution(rng);
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
            std::uniform_int_distribution<unsigned int> distribution(1, number_of_pokemon);
            uint8_t pokemonID = distribution(rng);
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
    const int trainer_class_amounts[] = {1, 1, 1, 1, 1, 1, 1, 1, 15, 0, 1, 3, 1, 1, 1, 1, 1, 1, 1, 5, 1, 12, 18, 19,
                                         15,
                                         1, 19, 20, 16, 13, 31, 5, 2, 3, 1, 14, 22, 21, 19, 12, 12, 6, 2, 20, 9, 1,
                                         3,
                                         8, 5, 9, 4, 12, 21, 19, 2, 9, 7, 3, 12, 6, 8, 5, 1, 1, 2, 5};
    for (int limit : trainer_class_amounts) {
        for (int trainer_num = 0; trainer_num < limit; trainer_num++) {
            std::string name = read_string(offset,
                                           UINT8_MAX);    //Just using UINT8_MAX as a length isn't needed. Maybe have a read function that doesn't need a length?
            offset += name.length() + 1;    //Include terminating character in offset change
            //Need to check custom move marker. See https://bulbapedia.bulbagarden.net/wiki/Trainer_data_structure_in_Generation_II
            bool has_custom_moves = rom[offset] & 1;
            bool has_custom_held_item = rom[offset] & 2;
            offset++;
            //0xFF is end of the trainer
            while (rom[offset] != 0xFF) {
                //Level at rom[offset], pokemonID at rom[offset + 1]
                unsigned int pokemon_level = rom[offset];
                std::uniform_int_distribution<unsigned int> distribution(1, number_of_pokemon);
                unsigned int new_pokemon_id = distribution(rng);
                rom[offset + 1] = new_pokemon_id;
                offset += 2;

                if (has_custom_held_item) {
                    //TODO: Randomize held item
                    offset++;
                }

                if (has_custom_moves) {
                    auto moveslist = pokemon_movelists[new_pokemon_id - 1];
                    // Populate with first learnable move as a default
                    std::vector<Move> new_moves{moveslist[0], moveslist[0], moveslist[0], moveslist[0]};
                    int move_count = 0;
                    for (int i = moveslist.size() - 1; i >= 0; i--) {
                        if (moveslist[i].level_to_learn <= pokemon_level) {
                            new_moves[move_count] = moveslist[i];
                            move_count++;
                            if (move_count >= 4) {
                                break;
                            }
                        }
                    }
                    //1 byte per move
                    rom[offset] = new_moves[0].move_id;
                    rom[offset + 1] = new_moves[1].move_id;
                    rom[offset + 2] = new_moves[2].move_id;
                    rom[offset + 3] = new_moves[3].move_id;

                    offset += 4;
                }

            }
        }

    }
}

void Rom::randomize_gift_pokemon() {
    //https://www.serebii.net/gs/gift.shtml for a list of gift pokemon in gold
    std::vector<int> gift_pokemon_locations;
    const int spearow_mem_locations = 0x1599FC;    //This spearow is nicknamed 'Kenya', at goldenrod
    const int eevee_mem_locations = 0x15CC10;    //Eevee at goldenrod
    const int togepi_mem_locations = 0x15924F;    //Mystery egg togepi
    const int shuckle_mem_locations = 0x73E6;    //This shuckle is nicknamed 'Shuckie', at cianwood city
    const int tyrogue_mem_locations = 0x119F20;    //Tyrogue at mt mortar
    gift_pokemon_locations.push_back(spearow_mem_locations);
    gift_pokemon_locations.push_back(eevee_mem_locations);
    gift_pokemon_locations.push_back(togepi_mem_locations);
    gift_pokemon_locations.push_back(shuckle_mem_locations);
    gift_pokemon_locations.push_back(tyrogue_mem_locations);

    for (int location: gift_pokemon_locations) {
        std::uniform_int_distribution<unsigned int> distribution(1, number_of_pokemon);
        uint8_t pokemonID = distribution(rng);
        rom[location] = pokemonID;
    }
}

void Rom::randomize_game_corner_pokemon() {
    //Pokemon rewards from the game corner
    //Goldenrod city prizes (Johto)
    const std::vector<int> abra_mem_locations{0x15E8B7, 0x15E8C8, 0x15E8CD, 0x15E93D};
    const std::vector<int> ekans_mem_locations{0x15E8E5, 0x15E8F6, 0x15E8FB, 0x15E94D};
    const std::vector<int> dratini_mem_locations{0x15E913, 0x15E924, 0x15E929, 0x15E95D};
    //Celadon city prizes (Kanto)
    const std::vector<int> mr_mime_mem_locations{0x179B9C, 0x179BAD, 0x179BB2, 0x179C22};
    const std::vector<int> eevee_mem_locations{0x179BCA, 0x179BDB, 0x179BE0, 0x179C32};
    const std::vector<int> porygon_mem_locations{0x179BF8, 0x179C09, 0x179C0E, 0x179C42};

    std::vector<std::vector<int>> game_corner_pokemon_locations{
            abra_mem_locations,
            ekans_mem_locations,
            dratini_mem_locations,
            mr_mime_mem_locations,
            eevee_mem_locations,
            porygon_mem_locations};

    for (const std::vector<int> &pokemon_locations: game_corner_pokemon_locations) {
        std::uniform_int_distribution<unsigned int> distribution(1, number_of_pokemon);
        uint8_t pokemonID = distribution(rng);
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

    for (const std::vector<int> &pokemon_locations: static_pokemon_locations) {
        std::uniform_int_distribution<unsigned int> distribution(1, number_of_pokemon);
        uint8_t pokemonID = distribution(rng);
        for (int location: pokemon_locations) {
            rom[location] = pokemonID;
        }
    }
}

/*
 * Reads a text file of banned item IDs for the randomization to avoid.
 * File should be only numerical IDs from 0-255, seperated by a space or a newline.
 * Will be able to read numbers from the start of words (1hello for example), but
 * this may not work as well
 */
std::vector<Item> Rom::load_banned_items() {
    //Default banned items are items that cannot be obtained in the game normally
    //https://bulbapedia.bulbagarden.net/wiki/List_of_items_by_index_number_(Generation_II)
    const std::vector<int> default_banned_item_ids = {
            0x06, 0x19, 0x2D, 0x32, 0x38,
            0x46, 0x5A, 0x64, 0x73, 0x74,
            0x78, 0x81, 0x87, 0x88, 0x89,
            0x8D, 0x8E, 0x91, 0x94, 0x95,
            0x99, 0x9A, 0x9B, 0xA2, 0xAB,
            0xB0, 0xB3, 0xBE, 0xC3, 0xDC,
            0xFA, 0xFB, 0xFC, 0xFD, 0xFE,
            0xFF
    };
    std::vector<Item> default_banned_items;
    default_banned_items.reserve(default_banned_item_ids.size());
    for (int id : default_banned_item_ids) {
        default_banned_items.push_back(items.get_item(id));
    }
    std::vector<Item> banned_items;
    const std::string filename = "items/gold_banned_items.txt";
    std::ifstream banned_item_file(filename, std::ios::in);
    //No banned items file found, revert to default banned items
    if (!banned_item_file.is_open()) {
        std::cerr << "Cannot open " << filename << ". Using default banned item list\n";
        return default_banned_items;
    }

    std::string line;
    int line_number = 1;
    while (banned_item_file >> line) {
        // try {
            int item_id = std::stoi(line, nullptr);
            if (item_id > UINT8_MAX) {
            	    // TODO commented out error stuff since switch console won't handle exceptions
                // std::ostringstream err_msg;
                // err_msg << "ID " << line << " at line number " << line_number << " in " << filename
                //         << " is too large. Max is " << UINT8_MAX;
		// std::cerr << err_msg << "\n";
		// std::cerr << "Falling back on default banned item list\n";
		printf("ID %s at line number %d in %s is too large. Max is %d\n", line.c_str(), line_number, filename.c_str(), UINT8_MAX);
		return default_banned_items;
            }
            banned_items.push_back(items.get_item(item_id));
            line_number++;
        // } catch (std::invalid_argument &e) {
        //     std::cerr << "Invalid value in " << filename << " at line " << line_number << "\n";
        //     std::cerr << "Falling back on default banned item list\n";
        //     return default_banned_items;
        // }
    }
    banned_item_file.close();
    return banned_items;
}

/*
 * Reads the string at offset until terminator character or max length is reached, into line.
 * Length does not include terminator character
 */
int Rom::read_string_and_length(int offset, int max_length, std::string &line) {
    uint8_t ch;
    int count = 0;
    for (int i = 0; i < max_length; i++) {
        ch = rom[offset + i];
        if (ch == GB_STRING_TERMINATOR) {
            break;
        }
        line.push_back(ch);
        count++;
    }
    return count;
}

void Rom::enable_shiny_mode() {
    //Replace the call in code to check if the pokemon should be shiny or not with
    //an opcode that does nothing
    const int shiny_check_location = 0x9C70;
    rom[shiny_check_location] = 0x00;
}

void Rom::randomize_pokemon_palettes() {
    //According to https://datacrystal.romhacking.net/wiki/Pok%C3%A9mon_Gold_and_Silver:ROM_map#Pokemon_Colors Pokemon palettes are between 0xAD45 and 0xB540
    //However, 0xB53D, 0xB53E and 0xB53F seem to be the player's sprite palette
    const int pokemon_palette_start = 0xAD45;
    const int pokemon_palette_end = 0xB53D;
    for (int i = pokemon_palette_start; i < pokemon_palette_end; i++) {
        std::uniform_int_distribution<unsigned int> distribution(0, UINT8_MAX);
        uint8_t val = distribution(rng);
        rom[i] = val;
    }
}
