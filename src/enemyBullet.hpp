#include "../include/sista/sista.hpp"
#include "direction.hpp"
#include "entity.hpp"
#include <memory>
#pragma once

class EnemyBullet : public Entity {
public:
    static sista::ANSISettings enemyBulletStyle;
    static std::vector<std::shared_ptr<EnemyBullet>> enemyBullets;
    Direction direction;
    bool collided = false;

    EnemyBullet(sista::Coordinates, Direction);
    void remove() override;

    void move();
};