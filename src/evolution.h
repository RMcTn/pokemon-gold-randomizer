#ifndef POKEMON_RANDOMIZER_EVOLUTION_H
#define POKEMON_RANDOMIZER_EVOLUTION_H

enum EvolutionType {
    EVOLVE_LEVEL = 1,
    EVOLVE_ITEM,
    EVOLVE_TRADE,
    // TODO: Do we need one for trade with item?
    EVOLVE_HAPPINESS,
    EVOLVE_STAT,
};

enum EvolutionHappinessCondition {
    ANYTIME = 1,
    DAYTIME,
    NIGHTTIME,
};

enum EvolutionStatCondition {
    ATTACK_GREATER_THAN_DEFENCE = 1,
    ATTACK_LESS_THAN_DEFENCE,
    ATTACK_EQUAL_DEFENCE,
};


struct Evolution {
    unsigned int pokemon;
    unsigned int pokemon_to_evolve_to;
    EvolutionType evolution_type;
    unsigned int level_to_evolve;
    unsigned int item_id_to_evolve;
    EvolutionHappinessCondition happiness_condition;
    EvolutionStatCondition stat_condition;
};


#endif //POKEMON_RANDOMIZER_EVOLUTION_H
