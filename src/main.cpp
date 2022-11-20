#include "randomization_options.h"
#include "rom.h"
#include <vector>

int main() {
    Rom rom = Rom();
    if (!rom.load("../gold.gbc")) {
        printf("Could not load gold.gbc\n");
        return -1;
    }
    std::vector<RandomizationOptions> options;
    rom.run(options);
    rom.save();
}
