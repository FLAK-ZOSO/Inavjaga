#define VERSION "0.1.0"
#define DATE "2025-07-05"

#define DEBUG 0

// Field size and shape
#define WIDTH 100
#define HEIGHT 30
#define TUNNEL_UNIT 3

// Game speed
#define FRAME_DURATION 100
#define BULLET_SPEED 1

// Inventory
#define INITIAL_CLAY 5
#define INITIAL_BULLETS 10
#define INITIAL_MEAT 10

// Loot
#define LOOT_ARCHER_CLAY 0
#define LOOT_ARCHER_BULLETS 3
#define LOOT_ARCHER_MEAT 1
#define LOOT_WORM_HEAD_CLAY 3
#define LOOT_WORM_HEAD_BULLETS 0
#define LOOT_WORM_HEAD_MEAT 3

// Costs
#define COST_OF_MINE {1,3,0} // The cost of building a mine in the format {clay, bullets, meat}
#define MEAT_DURATION_PERIOD 200 // Every how many frames the player eats a unit of meat

// Colors
#define RGB_BLACK ANSI::RGBColor(0,0,0)
#define RGB_ROCKS_FOREGROUND ANSI::RGBColor(10,10,10)
#define RGB_ROCKS_BACKGROUND ANSI::RGBColor(100,100,100)

// Damage
#define MINE_MINIMUM_DAMAGE 1
#define MINE_MAXIMUM_DAMAGE 3
#define INITIAL_WALL_STRENGTH 7
#define WORM_HEALTH_POINTS 3

// Probabilistic settings
#define WALL_WEARING_PROBABILITY 0.1
#define DAMAGED_WALLS_COUNT 5
#define MINE_EXPLOSION_IN_FRAME_PROBABILITY 0.10
#define DUMB_MOVE_PROBABILITY 0.25
#define ARCHER_SPAWNING_PROBABILITY 0.01
#define ARCHER_MOVING_PROBABILITY 0.33
#define ARCHER_SHOOTING_PROBABILITY 0.05
#define WORM_TURNING_PROBABILITY 0.10
#define WORM_SPAWNING_PROBABILITY 0.001
#define WORM_EATING_ARCHER_PROBABILITY 0.1
#define CLAY_RELEASE_PROBABILITY 0.01

// Spawn settings
#define INITIAL_ARCHERS 6
#define INITIAL_WORMS 2

// Worm
#define WORM_LENGTH 7
