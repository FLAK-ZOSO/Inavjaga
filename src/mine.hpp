#include "entity.hpp"
#include <random>
#include <memory>
#pragma once

class Mine : public Entity {
public:
    static sista::ANSISettings mineStyle;
    static sista::ANSISettings triggeredMineStyle;
    static std::vector<std::shared_ptr<Mine>> mines;
    static std::bernoulli_distribution explosion;
    static std::uniform_int_distribution<int> mineDamage;
    bool triggered;

    Mine(sista::Coordinates);
    void remove() override;

    void trigger();
    void explode();
};