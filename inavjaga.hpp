#include "include/sista/sista.hpp"
#include "constants.hpp"
#include "direction.hpp"
#include "inventory.hpp"
#include "entity.hpp"
#include "portal.hpp"
#include "player.hpp"
#include "chest.hpp"
#include "bullet.hpp"
#include "worm.hpp"
#include "mine.hpp"
#include "wall.hpp"
#include "enemyBullet.hpp"
#include "archer.hpp"
#include <unordered_map>
#include <vector>
#include <random>
#include <map>
#include <set>


extern std::unordered_map<Direction, sista::Coordinates> directionMap;
extern std::unordered_map<Direction, char> directionSymbol;
extern std::set<char> movementKeys;
extern std::mt19937 rng;
extern std::map<int, std::vector<int>> passages; // {y, {x1, x2, x3...}}
extern std::map<int, std::vector<int>> breaches; // {y, {x1, x2, x3...}}
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