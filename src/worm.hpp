#include "../include/sista/sista.hpp"
#include "direction.hpp"
#include "entity.hpp"
#include <random>
#include <memory>
#pragma once

class Worm; // Forward implicit declaration
class WormBody : public Entity {
public:
    static sista::ANSISettings wormBodyStyle;
    static std::vector<std::shared_ptr<WormBody>> wormBodies;
    std::weak_ptr<Worm> head;

    WormBody(sista::Coordinates, Direction);
    void remove() override;

    void die();
};

/* Worm - represents a Worm but as a `sista::Pawn` it corresponds to the head */
class Worm : public Entity, public std::enable_shared_from_this<Worm> {
public:
    static sista::ANSISettings wormHeadStyle;
    static std::vector<std::shared_ptr<Worm>> worms;
    static std::bernoulli_distribution turning;
    static std::bernoulli_distribution moving;
    static std::bernoulli_distribution spawning;
    static std::bernoulli_distribution eatingArcher;
    static std::bernoulli_distribution eatingTail;
    static std::bernoulli_distribution clayRelease;
    static Direction options[2];
    std::vector<std::shared_ptr<WormBody>> body;
    Direction direction;
    bool collided;
    short int hp;

    Worm(sista::Coordinates);
    Worm(sista::Coordinates, Direction);
    void remove() override;

    void move();
    void turn();
    void turn(Direction);
    void getHit();
    void die();
};
