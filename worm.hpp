#include "include/sista/sista.hpp"
#include "direction.hpp"
#include "entity.hpp"
#include <random>
#pragma once

class Worm; // Forward implicit declaration
class WormBody : public Entity {
public:
    static ANSI::Settings wormBodyStyle;
    static std::vector<WormBody*> wormBodies;
    Worm* head;

    WormBody(sista::Coordinates, Direction);
    void remove() override;

    void die();
};

/* Worm - represents a Worm but as a `sista::Pawn` it corresponds to the head */
class Worm : public Entity {
public:
    static ANSI::Settings wormHeadStyle;
    static std::vector<Worm*> worms;
    static std::bernoulli_distribution turning;
    static std::bernoulli_distribution moving;
    static std::bernoulli_distribution spawning;
    static std::bernoulli_distribution eatingArcher;
    static std::bernoulli_distribution eatingTail;
    static std::bernoulli_distribution clayRelease;
    static Direction options[2];
    std::vector<WormBody*> body;
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
