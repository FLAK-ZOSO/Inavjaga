#include "include/sista/sista.hpp"
#include "src/constants.hpp"
#include "src/direction.hpp"
#include "src/inventory.hpp"
#include "src/entity.hpp"
#include "src/portal.hpp"
#include "src/player.hpp"
#include "src/chest.hpp"
#include "src/bullet.hpp"
#include "src/worm.hpp"
#include "src/mine.hpp"
#include "src/wall.hpp"
#include "src/enemyBullet.hpp"
#include "src/archer.hpp"
#include <unordered_map>
#include <vector>
#include <random>
#include <map>
#include <set>

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