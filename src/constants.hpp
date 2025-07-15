#pragma once

#define VERSION "1.0.0-beta.4"
#define DATE "2025-07-15"
#define AUTHOR "FLAK-ZOSO"


#define DEBUG 1
#define INTRO 1 // Set to 0 to disable the intro screen on startup
#define TUTORIAL 1 // Set to 0 to disable the tutorial on startup

// Field size and shape
#define WIDTH 70 // The width of the field in cells
#define HEIGHT 30 // The height of the field in cells
#define TUNNEL_UNIT 3 // The spacing unit for the tunnels (walls are TUNNEL_UNIT cells wide, the tunnels are twice as wide)
#define PORTALS_PER_LINE 2 // How many portals are there across each wall

// Game speed
#define FRAME_DURATION 100 // Expressed in milliseconds
#define BULLET_SPEED 1 // How many cells a bullet travels in one frame

// Inventory
#define DROP_INVENTORY_ON_DEATH 1 // Set to 0 to lose the player's inventory on death
#define INITIAL_CLAY 5 // The initial amount of clay the player has
#define INITIAL_BULLETS 10 // The initial amount of bullets the player has
#define INITIAL_MEAT 10 // The initial amount of meat the player has

// Loot
#define LOOT_ARCHER_CLAY 0 // The amount of clay the player gets from an archer
#define LOOT_ARCHER_BULLETS 3 // The amount of bullets the player gets from an archer
#define LOOT_ARCHER_MEAT 1 // The amount of meat the player gets from an archer
#define LOOT_WORM_HEAD_CLAY 3 // The amount of clay the player gets from a worm head
#define LOOT_WORM_HEAD_BULLETS 0 // The amount of bullets the player gets from a worm head
#define LOOT_WORM_HEAD_MEAT 3 // The amount of meat the player gets from a worm head

// Costs
#define COST_OF_MINE {1,3,0} // The cost of building a mine in the format {clay, bullets, meat}
#define MEAT_DURATION_PERIOD 50 // Every how many frames the player eats a unit of meat

// Coordinates
#define SPAWN_COORDINATES {TUNNEL_UNIT,WIDTH-TUNNEL_UNIT} // The coordinates where the player spawns
#define RESPAWN_COORDINATES {TUNNEL_UNIT,WIDTH-TUNNEL_UNIT} // The coordinates where the player respawns after death

// Colors
#define RGB_BLACK ANSI::RGBColor(0,0,0)
#define RGB_ROCKS_FOREGROUND ANSI::RGBColor(10,10,10)
#define RGB_ROCKS_BACKGROUND ANSI::RGBColor(100,100,100)

// Damage
#define MINE_MINIMUM_DAMAGE 1 // The minimum damage a mine can deal
#define MINE_MAXIMUM_DAMAGE 3 // The maximum damage a mine can deal
#define MINE_SENSITIVITY_RADIUS 1 // The radius around the mine that triggers it if a worm is present
#define MINE_DAMAGE_RADIUS 2 // The radius around the mine that deals damage
#define INITIAL_WALL_STRENGTH 7 // The initial strength of a wall
#define WORM_HEALTH_POINTS 3 // The initial health points of a worm

// Probabilistic settings (1 = 100%, 0.5 = 50%, etc.)
#define WALL_WEARING_PROBABILITY 0.3 // The probability of some walls being worn down (losing one strength point) in any given frame
#define DAMAGED_WALLS_COUNT 5 // The number of walls getting damaged at once if the wall wearing probability is met
#define MINE_EXPLOSION_IN_FRAME_PROBABILITY 0.10 // The probability of a mine exploding in any given frame given that it has been triggered
#define DUMB_MOVE_PROBABILITY 0.25 // The probability of an archer moving in a random direction instead of making a decision based on the environment
#define ARCHER_SPAWNING_PROBABILITY 0.02 // The probability of an archer spawning in a given frame
#define ARCHER_MOVING_PROBABILITY 0.33 // The probability of an archer moving in a given frame
#define ARCHER_SHOOTING_PROBABILITY 0.05 // The probability of an archer shooting in a given frame
#define WORM_TURNING_PROBABILITY 0.10 // The probability of a worm turning in a given frame
#define WORM_SPAWNING_PROBABILITY 0.004 // The probability of a worm spawning in a given frame
#define WORM_EATING_ARCHER_PROBABILITY 0.1 // The probability of a worm eating an archer when meeting it
#define WORM_EATING_TAIL_PROBABILITY 0.2 // The probability of a worm eating a piece of its own tail when stumbling upon it
#define WORM_MOVING_PROBABILITY 0.75 // The probability of a worm moving in any given frame
#define CLAY_RELEASE_PROBABILITY 0.01 // The probability of a worm releasing clay when it moves

// Spawn settings
#define INITIAL_ARCHERS 6 // The number of archers to spawn at the start of the game
#define INITIAL_WORMS 1 // The number of worms to spawn at the start of the game

// Worm
#define WORM_LENGTH 7 // The maximum length to which a worm can grow
