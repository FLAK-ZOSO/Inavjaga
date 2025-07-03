#define VERSION "0.0.1"
#define DATE "2025-07-03"

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
#define INITIAL_MEAT 3

// Costs
#define COST_OF_MINE {1,3,0}

// Colors
#define RGB_BLACK ANSI::RGBColor(0,0,0)
#define RGB_ROCKS_FOREGROUND ANSI::RGBColor(10,10,10)
#define RGB_ROCKS_BACKGROUND ANSI::RGBColor(100,100,100)

// Damage
#define MINE_MINIMUM_DAMAGE 1
#define MINE_MAXIMUM_DAMAGE 3
#define MINE_EXPLOSION_IN_FRAME_PROBABILITY 0.10
