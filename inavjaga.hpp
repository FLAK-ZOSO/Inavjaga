#include "include/sista/sista.hpp"
#include <unordered_map>
#include <vector>
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

void generateTunnels();
void printSideInstructions(int);
void input();
void act(char);
void deallocateAll();


struct Inventory {
    short walls = 0;
    short eggs = 0;
    short meat = 0;

    void operator+=(const Inventory&);
}; // The idea is that the inventory can be dropped (as CHEST) and picked up by the player


class Entity : public sista::Pawn {
public:
    Type type;

    Entity();
    Entity(char, sista::Coordinates, ANSI::Settings&, Type);
};


class Player : public Entity {
public:
    static ANSI::Settings playerStyle;
    static Player* player;
    enum Mode {
        COLLECT, BULLET,
        DUMPCHEST, TRAP, MINE
    } mode;
    Inventory inventory;

    Player();
    Player(sista::Coordinates);

    void move(Direction);
    void shoot(Direction);
};

class Wall : public Entity {
public:
    static ANSI::Settings wallStyle;
    static std::vector<Wall*> walls;
    short int strength;

    Wall();
    Wall(sista::Coordinates, short int);

    static void removeWall(Wall*);
};
