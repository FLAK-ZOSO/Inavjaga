#include "include/sista/sista.hpp"
#include <unordered_map>
#include <random>

enum Type {
    PLAYER,
    BULLET,
    WALL, // #
    GATE, // =
    CHEST, // C, can be collected by the player
    TRAP, // T, will act when stepped on
    MINE, // *, will be triggered when passing by

    // Inspired from https://github.com/Lioydiano/Dune
    WORM_HEAD, // H
    WORM_BODY, // <^v>

    ARCHER, // A
    ENEMY_BULLET,
};

enum Direction {UP, RIGHT, DOWN, LEFT};
extern std::unordered_map<Direction, sista::Coordinates> directionMap;
extern std::unordered_map<Direction, char> directionSymbol;
extern std::mt19937 rng;
