#include "rom.h"

int main() {
	Rom rom = Rom();
	rom.load();
	rom.run();
	rom.randomize_starters();
	rom.save();
}
