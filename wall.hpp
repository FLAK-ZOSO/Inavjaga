#include "include/sista/sista.hpp"
#include "entity.hpp"
#include <random>
#pragma once

class Wall : public Entity {
public:
    static ANSI::Settings wallStyle;
    static std::vector<Wall*> walls;
    static std::bernoulli_distribution wearing;
    short int strength;

    Wall(sista::Coordinates, short int);
    void remove() override;

    bool getHit();
};