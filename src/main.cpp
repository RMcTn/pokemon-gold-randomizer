#include "rom.h"

#include <ctime>

int main() {
	Rom rom = Rom();
	rom.load();
	rom.randomize_starters(std::time(nullptr));
	rom.save();
}
