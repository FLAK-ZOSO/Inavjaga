#include "cross_platform.hpp"
#include "constants.hpp"
#include "inavjaga.hpp"
#include <algorithm>
#include <thread>
#include <chrono>
#include <mutex>


Player* Player::player;
std::vector<Wall*> Wall::walls;
std::vector<Bullet*> Bullet::bullets;
std::vector<Chest*> Chest::chests;
std::vector<Portal*> Portal::portals;

sista::SwappableField* field;
sista::Cursor cursor;
sista::Border border(
    '@', {
        RGB_GAME_BACKGROUND,
        ANSI::RGBColor(100, 100, 100),
        ANSI::Attribute::BRIGHT
    }
);
const Inventory INITIAL_INVENTORY {
    INITIAL_CLAY,
    INITIAL_BULLETS,
    INITIAL_MEAT
};
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
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (end) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(
            (int)(FRAME_DURATION / (std::pow(1 + (int)speedup, 2)))
        )); // If there is speedup, the waiting time is reduced by a factor of 4
        std::lock_guard<std::mutex> lock(streamMutex); // Lock stays until scope ends
        for (unsigned j = 0; j < Bullet::bullets.size(); j++) {
            Bullet* bullet = Bullet::bullets[j];
            if (bullet == nullptr) continue;
            if (bullet->collided) continue;
            bullet->move();
        }
        for (Bullet* bullet : Bullet::bullets) {
            if (bullet->collided) {
                bullet->remove();
            }
        }
        printSideInstructions(i);
        std::flush(std::cout);
    }

    end = true;
    deallocateAll();
    th.join();
    field->clear();
    cursor.set(72, 0); // Move the cursor to the bottom of the screen, so the terminal is not left in a weird state
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
    cursor.set(3, WIDTH + 10);
    ANSI::setAttribute(ANSI::Attribute::BRIGHT);
    std::cout << "Inventory\n";
    ANSI::resetAttribute(ANSI::Attribute::BRIGHT);
    cursor.set(4, WIDTH + 10);
    std::cout << "Clay: " << Player::player->inventory.clay << "   \n";
    cursor.set(5, WIDTH + 10);
    std::cout << "Bullets: " << Player::player->inventory.bullets << "   \n";
    cursor.set(6, WIDTH + 10);
    std::cout << "Meat: " << Player::player->inventory.meat << "   \n";
    cursor.set(7, WIDTH + 10);
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
    cursor.set(10, WIDTH + 10);
    ANSI::setAttribute(ANSI::Attribute::BRIGHT);
    std::cout << "Time survived: " << i << "    \n";
    ANSI::resetAttribute(ANSI::Attribute::BRIGHT);
    cursor.set(11, WIDTH + 10);

    if (i % 100 == 99 || i == 0) {
        cursor.set(12, WIDTH + 10);
        ANSI::setAttribute(ANSI::Attribute::BRIGHT);
        std::cout << "Instructions\n";
        ANSI::resetAttribute(ANSI::Attribute::BRIGHT);
        cursor.set(13, WIDTH + 10);
        std::cout << "Move: \x1b[35mw\x1b[37m | \x1b[35ma\x1b[37m | \x1b[35ms\x1b[37m | \x1b[35md\x1b[37m\n";
        cursor.set(14, WIDTH+10);
        std::cout << "Act: \x1b[35mi\x1b[37m | \x1b[35mj\x1b[37m | \x1b[35mk\x1b[37m | \x1b[35ml\x1b[37m\n";
        cursor.set(16, WIDTH + 10);
        std::cout << "Collect mode: \x1b[35mc\x1b[37m\n";
        cursor.set(17, WIDTH + 10);
        std::cout << "Bullet mode: \x1b[35mb\x1b[37m\n";
        cursor.set(18, WIDTH + 10);
        std::cout << "Dump Chest mode: \x1b[35me\x1b[37m\n";
        cursor.set(19, WIDTH + 10);
        std::cout << "Place Trap mode: \x1b[35mt\x1b[37m\n";
        cursor.set(20, WIDTH + 10);
        std::cout << "Place Mine mode: \x1b[35mm\x1b[37m | \x1b[35m*\x1b[37m\n";
        cursor.set(22, WIDTH + 10);
        std::cout << "Speedup mode: \x1b[35m+\x1b[37m | \x1b[35m-\x1b[37m\n";
        cursor.set(23, WIDTH + 10);
        std::cout << "Pause or resume: \x1b[35m.\x1b[37m | \x1b[35mp\x1b[37m\n";
        cursor.set(24, WIDTH + 10);
        std::cout << "Quit: \x1b[35mQ\x1b[37m\n";
    }
}

void generateTunnels() {
    std::uniform_int_distribution<int> distr(TUNNEL_UNIT * 2, WIDTH-(TUNNEL_UNIT * 2)); // Inclusive
    int portalCoordinate;
    for (int row=0; row<HEIGHT; row++) {
        if (row % (TUNNEL_UNIT * 3) == 0 && row + TUNNEL_UNIT * 3 < HEIGHT) {
            portalCoordinate = distr(rng);
            Portal* abovePortal = new Portal({row + TUNNEL_UNIT * 2, portalCoordinate});
            Portal* belowPortal = new Portal({row + TUNNEL_UNIT * 3 - 1, portalCoordinate});
            abovePortal->exit = belowPortal;
            belowPortal->exit = abovePortal;
            field->addPawn(abovePortal);
            field->addPawn(belowPortal);
        }

        if (row % (TUNNEL_UNIT * 3) >= TUNNEL_UNIT * 2) { // Every two units skipped, one is built
            for (int column=0; column<WIDTH; column++) {
                if (column < TUNNEL_UNIT * 2
                    && (row / TUNNEL_UNIT / 3) % 2 == 0) {
                    // On "even" horizontal tunnels we leave tunnel space on the left
                    column = TUNNEL_UNIT * 2;
                } else if (column >= WIDTH-(TUNNEL_UNIT * 2)
                            && (row / TUNNEL_UNIT / 3) % 2 == 1) {
                    break; // On "odd" horizontal tunnels we leave tunnel space on the right
                }
                if (field->isFree((unsigned short)row, (unsigned short)column)) {
                    field->addPawn(new Wall({row, column}, row % (TUNNEL_UNIT * 3)));
                }
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

        case 'j': case 'J': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->shoot(Direction::LEFT);
            break;
        }
        case 'k': case 'K': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->shoot(Direction::DOWN);
            break;
        }
        case 'l': case 'L': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->shoot(Direction::RIGHT);
            break;
        }
        case 'i': case 'I': {
            std::lock_guard<std::mutex> lock(streamMutex);
            Player::player->shoot(Direction::UP);
            break;
        }

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
    for (auto bullet : Bullet::bullets) {
        delete bullet;
    }
    for (auto chest : Chest::chests) {
        delete chest;
    }
    for (auto portal : Portal::portals) {
        delete portal;
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
void Wall::remove() {
    Wall::walls.erase(std::find(Wall::walls.begin(), Wall::walls.end(), this));
    field->erasePawn(this);
    delete this;
}
void Wall::getHit() {
    if (--strength <= 0) {
        this->remove();
    }
}
ANSI::Settings Wall::wallStyle = {
    RGB_GAME_BACKGROUND,
    ANSI::RGBColor(100, 100, 100),
    ANSI::Attribute::BRIGHT
};

Bullet::Bullet() : Entity('>', {0, 0}, bulletStyle, Type::BULLET), direction(Direction::RIGHT), collided(false) {}
Bullet::Bullet(sista::Coordinates coordinates, Direction direction) :
    Entity(directionSymbol[direction], coordinates, bulletStyle, Type::BULLET), direction(direction), collided(false) {
    Bullet::bullets.push_back(this);
}
void Bullet::remove() {
    Bullet::bullets.erase(std::find(Bullet::bullets.begin(), Bullet::bullets.end(), this));
    field->erasePawn(this);
    delete this;
}
void Bullet::move() {
    sista::Coordinates next = this->coordinates + directionMap[direction];
    if (field->isFree(next)) {
        field->movePawn(this, next);
    } else if (field->isOutOfBounds(next)) {
        this->remove();
    } else if (field->isOccupied(next)) {
        Entity* entity = (Entity*)field->getPawn(next);
        Type entityType = entity->type;
        switch (entityType) {
            case Type::WALL:
                ((Wall*)entity)->getHit();
                break;
        }
        this->remove(); // Hit something and the situation was not handled
    }
}
ANSI::Settings Bullet::bulletStyle = {
    ANSI::ForegroundColor::F_MAGENTA,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};

Chest::Chest() : Entity('C', {0, 0}, chestStyle, Type::CHEST), inventory(INITIAL_INVENTORY) {}
Chest::Chest(sista::Coordinates coordinates, Inventory inventory) : Entity('C', coordinates, chestStyle, Type::CHEST), inventory(inventory) {
    Chest::chests.push_back(this);
}
void Chest::remove() {
    Chest::chests.erase(std::find(Chest::chests.begin(), Chest::chests.end(), this));
    field->erasePawn(this);
    delete this;
}
ANSI::Settings Chest::chestStyle = {
    ANSI::RGBColor(193, 201, 104),
    RGB_BLACK,
    ANSI::Attribute::REVERSE
};

Portal::Portal() : Entity('#', {0, 0}, portalStyle, Type::PORTAL) {}
Portal::Portal(sista::Coordinates coordinates) : Entity('#', coordinates, portalStyle, Type::PORTAL) {
    Portal::portals.push_back(this);
}
void Portal::remove() {
    Portal::portals.erase(std::find(Portal::portals.begin(), Portal::portals.end(), this));
    field->erasePawn(this);
    delete this;
}
ANSI::Settings Portal::portalStyle = {
    ANSI::RGBColor(100, 0, 0xff),
    RGB_GAME_BACKGROUND,
    ANSI::Attribute::STRIKETHROUGH
};

Player::Player(sista::Coordinates coordinates) : Entity('$', coordinates, playerStyle, Type::PLAYER), mode(Player::Mode::COLLECT), inventory(INITIAL_INVENTORY) {}
Player::Player() : Entity('$', {0, 0}, playerStyle, Type::PLAYER), mode(Player::Mode::COLLECT), inventory(INITIAL_INVENTORY) {}
void Player::remove() {
    field->erasePawn(this);
    delete this;
}
void Player::move(Direction direction) {
    sista::Coordinates next = this->coordinates + directionMap[direction];
    if (field->isFree(next)) {
        field->movePawn(this, next);
    } else if (field->isOutOfBounds(next)) {
        return;
    } else if (field->isOccupied(next)) {
        Entity* entity = (Entity*)field->getPawn(next);
        switch (entity->type) {
            case Type::PORTAL: {
                Portal* portal = (Portal*)entity;
                sista::Coordinates landing = portal->exit->getCoordinates() + directionMap[direction];
                if (field->isFree(landing)) {
                    field->movePawn(this, landing);
                }
                break;
            }
            default:
                break;
        }
    }
}
void Player::shoot(Direction direction) {
    sista::Coordinates target = this->coordinates + directionMap[direction];
    if (!field->isFree(target)) {
        if (field->isOutOfBounds(target)) return;
        
        Entity* entity = (Entity*)field->getPawn(target);
        switch (this->mode) {
            // TODO: add modes
            case Mode::COLLECT: {
                if (entity->type == Type::CHEST) {
                    Chest* chest = (Chest*)entity;
                    this->inventory += chest->inventory;
                    chest->remove();
                }
                break;
            }
            default:
                return;
        }
        return;
    }

    switch (this->mode) {
        // TODO: add modes
        case Mode::BULLET:
            if (--inventory.bullets >= 0) {
                field->addPrintPawn(new Bullet(target, direction));
            }
            inventory.bullets = std::max(inventory.bullets, (short)0);
            break;
        case Mode::DUMPCHEST:
            // There is a check missing here... on purpose ;)
            field->addPrintPawn(new Chest(target, this->inventory));
            this->inventory = {0, 0, 0};
            break;
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
    clay += other.clay;
    bullets += other.bullets;
    meat += other.meat;
}
