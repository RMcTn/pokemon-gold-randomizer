#ifndef POKEMON_RANDOMIZER_MOVE_H
#define POKEMON_RANDOMIZER_MOVE_H

struct Move {
    unsigned int move_id;
    unsigned int level_to_learn;
    // TODO: move type?
    // TODO: name of the move? would need to load move names before populating
};

#endif //POKEMON_RANDOMIZER_MOVE_H
