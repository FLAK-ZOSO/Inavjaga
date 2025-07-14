#include "include/sista/sista.hpp"
#include "entity.hpp"
#include <random>
#pragma once

class Mine : public Entity {
public:
    static ANSI::Settings mineStyle;
    static ANSI::Settings triggeredMineStyle;
    static std::vector<Mine*> mines;
    static std::bernoulli_distribution explosion;
    static std::uniform_int_distribution<int> mineDamage;
    bool triggered;

    Mine(sista::Coordinates);
    void remove() override;

    void trigger();
    void explode();
};