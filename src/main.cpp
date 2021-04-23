#include "rom.h"

int main() {
    Rom rom = Rom();
    if (!rom.load("gold.gbc")) {
        printf("Could not load gold.gbc\n");
        return -1;
    }
    rom.run();
    rom.save();
}
