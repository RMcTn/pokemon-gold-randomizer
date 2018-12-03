#include "rom.h"

int main() {
	Rom rom = Rom();
	rom.load();
	rom.run();
	rom.randomize_intro_pokemon();
	rom.randomize_starters();
	rom.randomize_land_encounters();
	rom.save();
}
