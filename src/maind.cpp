#include "rom.h"

// Filename is not main since it was conflicting with using it in the switch console version. Should really make this project a library
int main() {
    Rom rom = Rom();
    if (!rom.load("../gold.gbc")) {
        printf("Could not load gold.gbc\n");
        return -1;
    }
    rom.run();
    rom.save();
}
