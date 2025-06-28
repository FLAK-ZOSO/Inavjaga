#include "cross_platform.hpp"
#include "inavjaga.hpp"
#include <algorithm>
#include <thread>
#include <chrono>
#include <mutex>

#define VERSION "0.0.1"
#define DATE "2025-06-28"

#define WIDTH 70
#define HEIGHT 30

#define TUNNEL_UNIT 2
#define FRAME_DURATION 100

Player* Player::player;
std::vector<Wall*> Wall::walls;

sista::SwappableField* field;
sista::Cursor cursor;
sista::Border border(
    '@', {
        ANSI::RGBColor(10, 10, 10),
        ANSI::RGBColor(100, 100, 100),
        ANSI::Attribute::BRIGHT
    }
);
std::mutex streamMutex;
bool speedup = false;
bool pause_ = false;
bool end = false;


int main(int argc, char* argv[]) {
    #ifdef __APPLE__
        term_echooff();
        // system("stty raw -echo");
    #endif
    std::ios_base::sync_with_stdio(false);
    ANSI::reset(); // Reset the settings
    srand(time(0)); // Seed the random number generator

    sista::SwappableField field_(WIDTH, HEIGHT);
    field = &field_;
    generateTunnels();
    Player::player = new Player({0, 0});
    Player::player->mode = Player::Mode::BULLET;
    field->addPawn(Player::player);
    field->print(border);
    std::thread th(input);
    for (int i=0; !end; i++) {
        while (pause_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        printSideInstructions(i);
        std::this_thread::sleep_for(std::chrono::milliseconds(
            (int)(FRAME_DURATION / (std::pow(1 + (int)speedup, 2)))
        )); // If there is speedup, the waiting time is reduced by a factor of 4
        std::flush(std::cout);
    }

    end = true;
    deallocateAll();
    th.join();
    field->clear();
    cursor.set(72, 0); // Move the cursor to the bottom of the screen, so the terminal is not left in a weird state
    std::this_thread::sleep_for(std::chrono::seconds(2));
    flushInput();
    #if __linux__
        getch();
    #elif __APPLE__
        getchar();
    #elif _WIN32
        getch();
    #endif
    #ifdef __APPLE__
        // system("stty -raw echo");
        tcsetattr(0, TCSANOW, &orig_termios);
    #endif
}

void printSideInstructions(int i) {
    // Print the inventory
    ANSI::reset();
    cursor.set(3, 80);
    ANSI::setAttribute(ANSI::Attribute::BRIGHT);
    std::cout << "Inventory\n";
    ANSI::resetAttribute(ANSI::Attribute::BRIGHT);
    cursor.set(4, 80);
    std::cout << "Walls: " << Player::player->inventory.walls << "   \n";
    cursor.set(5, 80);
    std::cout << "Eggs: " << Player::player->inventory.eggs << "   \n";
    cursor.set(6, 80);
    std::cout << "Meat: " << Player::player->inventory.meat << "   \n";
    cursor.set(7, 80);
    std::cout << "Mode: ";
    switch (Player::player->mode) {
        case Player::Mode::COLLECT:
            std::cout << "Collect";
            break;
        case Player::Mode::BULLET:
            std::cout << "Bullet";
            break;
        case Player::Mode::DUMPCHEST:
            std::cout << "Dump chest";
            break;
        case Player::Mode::TRAP:
            std::cout << "Trap";
            break;
        case Player::Mode::MINE:
            std::cout << "Mine";
            break;
    }
    std::cout << "      ";
    cursor.set(10, 80);
    ANSI::setAttribute(ANSI::Attribute::BRIGHT);
    std::cout << "Time survived: " << i << "    \n";
    ANSI::resetAttribute(ANSI::Attribute::BRIGHT);
    cursor.set(11, 80);
    // Be aware not to overwrite the inventory and the time survived which use {3, 80} to ~{11, 80}
    #if __linux__
    if (i % 10 == 9) {
    #elif __APPLE__ or _WIN32
    if (i % 100 == 99 || i == 0) {
    #endif
        cursor.set(15, 80);
        ANSI::setAttribute(ANSI::Attribute::BRIGHT);
        std::cout << "Instructions\n";
        ANSI::resetAttribute(ANSI::Attribute::BRIGHT);
        cursor.set(16, 80);
        std::cout << "Move: w | a | s | d\n";
        cursor.set(17, 80);
        std::cout << "Act: i | j | k | l\n";
        cursor.set(18, 80);
        std::cout << "Collect mode: \x1b[35mc\x1b[0m\n";
        cursor.set(19, 80);
        std::cout << "Bullet mode: \x1b[35mb\x1b[0m\n";
        cursor.set(20, 80);
        std::cout << "Dump Chest mode: \x1b[35me\x1b[0m\n";
        cursor.set(21, 80);
        std::cout << "Build Wall mode: \x1b[35m=\x1b[0m | \x1b[35m0\x1b[0m | \x1b[35m#\x1b[0m\n";
        cursor.set(22, 80);
        std::cout << "Build Gate mode: \x1b[35mg\x1b[0m\n";
        cursor.set(23, 80);
        std::cout << "Place Trap mode: \x1b[35mt\x1b[0m\n";
        cursor.set(24, 80);
        std::cout << "Place Mine mode: \x1b[35mm\x1b[0m | \x1b[35m*\x1b[0m\n";
        cursor.set(25, 80);
        std::cout << "Egg-hatching mode: \x1b[35mh\x1b[0m\n";
        cursor.set(27, 80);
        std::cout << "Speedup mode: \x1b[35m+\x1b[0m | \x1b[35m-\x1b[0m\n";
        cursor.set(28, 80);
        std::cout << "Pause or resume: \x1b[35m.\x1b[0m | \x1b[35mp\x1b[0m\n";
        cursor.set(29, 80);
        std::cout << "Q: Quit\n";
    }
}

void generateTunnels() {
    for (int row=0; row<HEIGHT; row++) {
        if (row % (TUNNEL_UNIT * 3) >= TUNNEL_UNIT * 2) {
            for (int column=0; column<WIDTH; column++) {
                if (column < TUNNEL_UNIT * 2
                    && (row / TUNNEL_UNIT / 3) % 2 == 0) {
                    // On "even" horizontal tunnels we leave tunnel space on the left
                    column = TUNNEL_UNIT * 2;
                } else if (column >= WIDTH-(TUNNEL_UNIT * 2)
                            && (row / TUNNEL_UNIT / 3) % 2 == 1) {
                    break; // On "odd" horizontal tunnels we leave tunnel space on the right
                }
                field->addPawn(new Wall({row, column}, row % (TUNNEL_UNIT * 3)));
            }
        }
    }
}

void input() {
    char input_ = '_';
    while (input_ != 'Q' /*&& input_ != 'q'*/) {
        if (end) return;
        #if defined(_WIN32) or defined(__linux__)
            input_ = getch();
        #elif __APPLE__
            input_ = getchar();
        #endif
        if (end) return;
        act(input_);
    }
}

void act(char input_) {
    switch (input_) {
        case 'w': case 'W': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->move(Direction::UP);
            break;
        }
        case 'a': case 'A': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->move(Direction::LEFT);
            break;
        }
        case 's': case 'S': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->move(Direction::DOWN);
            break;
        }
        case 'd': case 'D': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->move(Direction::RIGHT);
            break;
        }

        // case 'j': case 'J': {
        //     std::lock_guard<std::mutex> lock(streamMutex);
        //     Player::player->shoot(Direction::LEFT);
        //     break;
        // }
        // case 'k': case 'K': {
        //     std::lock_guard<std::mutex> lock(streamMutex);
        //     Player::player->shoot(Direction::DOWN);
        //     break;
        // }
        // case 'l': case 'L': {
        //     std::lock_guard<std::mutex> lock(streamMutex);
        //     Player::player->shoot(Direction::RIGHT);
        //     break;
        // }
        // case 'i': case 'I': {
        //     std::lock_guard<std::mutex> lock(streamMutex);
        //     Player::player->shoot(Direction::UP);
        //     break;
        // }

        case 'c': case 'C':
            Player::player->mode = Player::Mode::COLLECT;
            break;
        case 'b': case 'B':
            Player::player->mode = Player::Mode::BULLET;
            break;
        case 'e': case 'E': case 'q':
            Player::player->mode = Player::Mode::DUMPCHEST;
            break;
        case 't': case 'T':
            Player::player->mode = Player::Mode::TRAP;
            break;
        case 'm': case 'M': case '*':
            Player::player->mode = Player::Mode::MINE;
            break;

        case '+': case '-':
            speedup = !speedup;
            break;
        case '.': case 'p': case 'P':
            pause_ = !pause_;
            break;
        case 'Q': /* case 'q': */
            end = true;
            return;
        default:
            break;
    }
}

void deallocateAll() {
    for (auto wall : Wall::walls) {
        delete wall;
    }
}

// Entity::Entity() : sista::Pawn(' ', sista::Coordinates(0, 0), Player::playerStyle), type(Type::PLAYER) {}
Entity::Entity(char symbol, sista::Coordinates coordinates, ANSI::Settings& settings, Type type) :
    sista::Pawn(symbol, coordinates, settings), type(type) {}


Wall::Wall() : Entity('#', {0, 0}, wallStyle, Type::WALL) {}
Wall::Wall(sista::Coordinates coordinates, short int strength) :
    Entity('#', coordinates, wallStyle, Type::WALL), strength(strength) {
    Wall::walls.push_back(this);
}
void Wall::removeWall(Wall* wall) {
    Wall::walls.erase(std::find(Wall::walls.begin(), Wall::walls.end(), wall));
    field->erasePawn(wall);
    delete wall;
}
ANSI::Settings Wall::wallStyle = {
    ANSI::RGBColor(10, 10, 10),
    ANSI::RGBColor(100, 100, 100),
    ANSI::Attribute::BRIGHT
};

Player::Player(sista::Coordinates coordinates) : Entity('$', coordinates, playerStyle, Type::PLAYER), mode(Player::Mode::COLLECT), inventory({0, 0, 0}) {}
Player::Player() : Entity('$', {0, 0}, playerStyle, Type::PLAYER), mode(Player::Mode::COLLECT), inventory({0, 0, 0}) {}
void Player::move(Direction direction) {
    sista::Coordinates next = this->coordinates + directionMap[direction];
    if (field->isFree(next)) {
        field->movePawn(this, next);
    } else if (field->isOutOfBounds(next)) {
        return;
    } else if (field->isOccupied(next)) {
        Entity* entity = (Entity*)field->getPawn(next);
        if (entity->type == Type::WALL) {
            return;
        }
    }
}
void Player::shoot(Direction direction) {
    sista::Coordinates target = this->coordinates + directionMap[direction];
    if (!field->isFree(target)) return;

    switch (this->mode) {
        // TODO: add modes
        default:
            return;
    }
}
ANSI::Settings Player::playerStyle = {
    ANSI::ForegroundColor::F_RED,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};

std::unordered_map<Direction, sista::Coordinates> directionMap = {
    {Direction::UP, {(unsigned short)-1, 0}},
    {Direction::RIGHT, {0, 1}},
    {Direction::DOWN, {1, 0}},
    {Direction::LEFT, {0, (unsigned short)-1}}
};
std::unordered_map<Direction, char> directionSymbol = {
    {Direction::UP, '^'},
    {Direction::RIGHT, '>'},
    {Direction::DOWN, 'v'},
    {Direction::LEFT, '<'}
};
std::mt19937 rng(std::chrono::system_clock::now().time_since_epoch().count());

void Inventory::operator+=(const Inventory& other) {
    walls += other.walls;
    eggs += other.eggs;
    meat += other.meat;
}
