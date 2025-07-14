#include "../include/sista/sista.hpp"
#include "direction.hpp"
#include "entity.hpp"
#include <random>
#pragma once

class Archer : public Entity {
public:
    static ANSI::Settings archerStyle;
    static std::vector<Archer*> archers;
    static std::bernoulli_distribution shooting;
    static std::bernoulli_distribution moving;
    static std::bernoulli_distribution spawning;

    Archer(sista::Coordinates);
    void remove() override;

    void move();
    bool move(Direction);
    void shoot();
    bool shoot(Direction);
    void die();
};