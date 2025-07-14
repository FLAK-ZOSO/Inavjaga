#include "../include/sista/sista.hpp"
#include "direction.hpp"
#include "entity.hpp"
#pragma once

class Bullet : public Entity {
public:
    static ANSI::Settings bulletStyle;
    static std::vector<Bullet*> bullets;
    Direction direction;
    bool collided = false;

    Bullet(sista::Coordinates, Direction);
    void remove() override;

    void move();
};