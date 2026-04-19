#include "../include/sista/sista.hpp"
#include "direction.hpp"
#include "entity.hpp"
#pragma once

class Bullet : public Entity {
public:
    static sista::ANSISettings bulletStyle;
    static std::vector<Bullet*> bullets;
    Direction direction;
    bool collided = false;

    Bullet(sista::Coordinates, Direction);
    void remove() override;

    void move();
};