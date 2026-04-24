#include "entity.hpp"
#include <random>
#include <memory>
#pragma once

class Wall : public Entity {
public:
    static sista::ANSISettings wallStyle;
    static std::vector<std::shared_ptr<Wall>> walls;
    static std::bernoulli_distribution wearing;
    short int strength;

    Wall(sista::Coordinates, short int);
    void remove() override;

    bool takeHit();
};