#include "../include/sista/sista.hpp"
#pragma once

enum Type {
    PLAYER,
    BULLET,
    WALL, // #
    PORTAL, // =
    CHEST, // C, can be collected by the player
    TRAP, // T, will act when stepped on
    MINE, // *, will be triggered when passing by

    // Inspired from https://github.com/Lioydiano/Dune
    WORM_HEAD, // H
    WORM_BODY, // <^v>

    ARCHER, // A
    ENEMY_BULLET,
};

class Entity : public sista::Pawn {
public:
    Type type;

    Entity(char, sista::Coordinates, ANSI::Settings&, Type);
    virtual ~Entity() {}
    virtual void remove() = 0;
};