#include "rom.h"

int main() {
	Rom rom = Rom();
	rom.load();
	rom.run();
	rom.save();
}
