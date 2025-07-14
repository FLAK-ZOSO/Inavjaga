#include "include/sista/sista.hpp"
#include "direction.hpp"
#include "entity.hpp"
#pragma once

class EnemyBullet : public Entity {
public:
    static ANSI::Settings enemyBulletStyle;
    static std::vector<EnemyBullet*> enemyBullets;
    Direction direction;
    bool collided = false;

    EnemyBullet(sista::Coordinates, Direction);
    void remove() override;

    void move();
};