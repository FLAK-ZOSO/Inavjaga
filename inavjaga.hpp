#include "include/sista/sista.hpp"
#include "constants.hpp"
#include <unordered_map>
#include <vector>
#include <random>
#include <map>

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

enum Direction {UP, RIGHT, DOWN, LEFT};
extern std::unordered_map<Direction, sista::Coordinates> directionMap;
extern std::unordered_map<Direction, char> directionSymbol;
extern std::mt19937 rng;
extern std::map<int, std::vector<int>> passages; // {y, {x1, x2, x3...}}
extern std::map<int, std::vector<int>> breaches; // {y, {x1, x2, x3...}}
extern std::bernoulli_distribution dumbMoveDistribution;

void generateTunnels();
void spawnInitialEnemies();
void spawnEnemies();
bool endConditions();
void printSideInstructions(int);
void printKeys();
void input();
void act(char);
void deallocateAll();


struct Inventory {
    short clay;
    short bullets;
    short meat;

    void operator+=(const Inventory&);
    void operator-=(const Inventory&);

    inline bool containsAtLeast(const Inventory) const;
}; // The idea is that the inventory can be dropped (as CHEST) and picked up by the player


class Entity : public sista::Pawn {
public:
    Type type;

    Entity();
    Entity(char, sista::Coordinates, ANSI::Settings&, Type);
    virtual ~Entity() {}
    virtual void remove() = 0;
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
    void remove() override;

    void move(Direction);
    void shoot(Direction);
};

class Wall : public Entity {
public:
    static ANSI::Settings wallStyle;
    static std::vector<Wall*> walls;
    static std::bernoulli_distribution wearing;
    short int strength;

    Wall();
    Wall(sista::Coordinates, short int);
    void remove() override;

    bool getHit();
};

class Bullet : public Entity {
public:
    static ANSI::Settings bulletStyle;
    static std::vector<Bullet*> bullets;
    Direction direction;
    bool collided = false;

    Bullet();
    Bullet(sista::Coordinates, Direction);
    void remove() override;

    void move();
};

class EnemyBullet : public Entity {
public:
    static ANSI::Settings enemyBulletStyle;
    static std::vector<EnemyBullet*> enemyBullets;
    Direction direction;
    bool collided = false;

    EnemyBullet();
    EnemyBullet(sista::Coordinates, Direction);
    void remove() override;

    void move();
};

class Chest : public Entity {
public:
    static ANSI::Settings chestStyle;
    static std::vector<Chest*> chests;
    Inventory inventory;

    Chest();
    Chest(sista::Coordinates, Inventory);
    void remove() override;
};

class Portal : public Entity {
public:
    static ANSI::Settings portalStyle;
    static std::vector<Portal*> portals;
    Portal* exit; // The matching portal

    Portal();
    Portal(sista::Coordinates);
    Portal(sista::Coordinates, Portal*);
    void remove() override;
};

class Mine : public Entity {
public:
    static ANSI::Settings mineStyle;
    static ANSI::Settings triggeredMineStyle;
    static std::vector<Mine*> mines;
    static std::bernoulli_distribution explosion;
    static std::uniform_int_distribution<int> mineDamage;
    bool triggered;

    Mine();
    Mine(sista::Coordinates);
    void remove() override;

    void trigger();
    void explode();
};

class Archer : public Entity {
public:
    static ANSI::Settings archerStyle;
    static std::vector<Archer*> archers;
    static std::bernoulli_distribution shooting;
    static std::bernoulli_distribution moving;
    static std::bernoulli_distribution spawning;

    Archer();
    Archer(sista::Coordinates);
    void remove() override;

    void move();
    bool move(Direction);
    void shoot();
    bool shoot(Direction);
    void die();
};
