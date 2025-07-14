#include "include/sista/sista.hpp"
#include "constants.hpp"
#include "entity.hpp"
#include "portal.hpp"
#include "inventory.hpp"
#include <unordered_map>
#include <vector>
#include <random>
#include <map>
#include <set>


enum Direction {UP, RIGHT, DOWN, LEFT};
extern std::unordered_map<Direction, sista::Coordinates> directionMap;
extern std::unordered_map<Direction, char> directionSymbol;
extern std::set<char> movementKeys;
extern std::mt19937 rng;
extern std::map<int, std::vector<int>> passages; // {y, {x1, x2, x3...}}
extern std::map<int, std::vector<int>> breaches; // {y, {x1, x2, x3...}}
extern std::bernoulli_distribution dumbMoveDistribution;
enum EndReason {STARVED, SHOT, EATEN, STABBED, TOUCHDOWN, QUIT};

void generateTunnels();
void intro();
void tutorial();
void spawnInitialEnemies();
void spawnEnemies();
bool endConditions();
void printSideInstructions(int);
void printKeys();
void reprint();
void input();
void act(char);
void printEndInformation(EndReason);
void deallocateAll();


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

    EnemyBullet(sista::Coordinates, Direction);
    void remove() override;

    void move();
};

class Chest : public Entity {
public:
    static ANSI::Settings chestStyle;
    static std::vector<Chest*> chests;
    Inventory inventory;

    Chest(sista::Coordinates, Inventory);
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

    Archer(sista::Coordinates);
    void remove() override;

    void move();
    bool move(Direction);
    void shoot();
    bool shoot(Direction);
    void die();
};

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
