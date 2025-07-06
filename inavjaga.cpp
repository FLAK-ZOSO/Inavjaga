#include "cross_platform.hpp"
#include "inavjaga.hpp"
#include <algorithm>
#include <thread>
#include <chrono>
#include <mutex>
#include <stack>
#include <set>

Player* Player::player;
std::vector<Wall*> Wall::walls;
std::vector<Bullet*> Bullet::bullets;
std::vector<EnemyBullet*> EnemyBullet::enemyBullets;
std::vector<Chest*> Chest::chests;
std::vector<Portal*> Portal::portals;
std::vector<Mine*> Mine::mines;
std::vector<Archer*> Archer::archers;
std::vector<WormBody*> WormBody::wormBodies;
std::vector<Worm*> Worm::worms;

sista::SwappableField* field;
sista::Cursor cursor;
sista::Border border(
    '@', {
        RGB_ROCKS_FOREGROUND,
        RGB_ROCKS_BACKGROUND,
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
    Player::player = new Player({0, WIDTH - 1});
    Player::player->mode = Player::Mode::BULLET;
    field->addPawn(Player::player);
    spawnInitialEnemies();
    field->print(border);
    std::thread th(input);
    for (int i=0; !end; i++) {
        while (pause_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (!pause_) {
                sista::clearScreen(true);
                field->print(border);
                printKeys();
            } // Reprint after unpausing, just as a tool for allowing resizing
            if (end) break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(
            (int)(FRAME_DURATION / (std::pow(1 + (int)speedup, 2)))
        )); // If there is speedup, the waiting time is reduced by a factor of 4
        #if DEBUG
        auto start = std::chrono::high_resolution_clock::now();
        #endif
        std::lock_guard<std::mutex> lock(streamMutex); // Lock stays until scope ends
        for (int k = 0; k < BULLET_SPEED; k++) {
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
            for (unsigned j = 0; j < EnemyBullet::enemyBullets.size(); j++) {
                EnemyBullet* bullet = EnemyBullet::enemyBullets[j];
                if (bullet == nullptr) continue;
                if (bullet->collided) continue;
                bullet->move();
            }
            for (EnemyBullet* bullet : EnemyBullet::enemyBullets) {
                if (bullet->collided) {
                    bullet->remove();
                }
            }
        }
        for (unsigned j = 0; j < Mine::mines.size(); j++) {
            if (j >= Mine::mines.size()) break;
            Mine* mine = Mine::mines[j];
            if (mine->triggered) {
                if (Mine::explosion(rng)) {
                    mine->explode();
                }
            }
        }
        for (auto archer : Archer::archers) {
            if (Archer::moving(rng)) {
                archer->move();
            }
            if (Archer::shooting(rng)) {
                archer->shoot();
            }
        }
        for (unsigned j = 0; j < Worm::worms.size(); j++) {
            Worm* worm = Worm::worms[j];
            if (worm == nullptr) continue;
            if (Worm::turning(rng)) {
                // TODO: proper turning intelligence
                worm->turn(Direction::LEFT);
            }
            worm->move();
        }
        if (!Wall::walls.empty() && Wall::wearing(rng)) {
            for (int j = 0; j < DAMAGED_WALLS_COUNT; j++) {
                if (Wall::walls.empty()) break;
                int index = std::uniform_int_distribution<int>(0, Wall::walls.size() - 1)(rng);
                Wall::walls[index]->getHit();
            }
        }
        if (i % MEAT_DURATION_PERIOD == MEAT_DURATION_PERIOD - 1) {
            Player::player->inventory.meat--;
        }
        spawnEnemies();
        printSideInstructions(i);
        if (endConditions()) {
            end = true;
        }
        std::flush(std::cout);
        #if DEBUG
        auto stop = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> delta = stop - start;
        std::cerr << "Frame number " << i << " took " << delta.count() * 1000 << "ms" << std::endl;
        #endif
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
        // system("stty cooked echo");
        tcsetattr(0, TCSANOW, &orig_termios);
    #endif
}

bool endConditions() {
    // Check for enemies in the top right corner area
    for (unsigned short row = 0; row < TUNNEL_UNIT * 2; row++) {
        for (unsigned short column = WIDTH - 1; column >= WIDTH - TUNNEL_UNIT * 2; column--) {
            if (field->isOccupied(row, column)) {
                if (((Entity*)field->getPawn(row, column))->type == Type::ARCHER
                    || ((Entity*)field->getPawn(row, column))->type == Type::WORM_HEAD) {
                    return true;
                }
            }
        }
    }
    // Check for negative amount of meat
    if (Player::player->inventory.meat < 0) {
        return true;
    }
    return false;
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

    if (i == 0) printKeys();
}
void printKeys() {
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
                    if (row % (TUNNEL_UNIT * 3) == TUNNEL_UNIT * 3 - 1) {
                        passages[row] = std::vector<int>(TUNNEL_UNIT * 2);
                        std::iota(
                            passages[row].begin(),
                            passages[row].end(), 0
                        );
                        breaches[row] = {};
                    } // One of the breaches in the wall is always the built-in one
                } else if (column >= WIDTH-(TUNNEL_UNIT * 2)
                            && (row / TUNNEL_UNIT / 3) % 2 == 1) {
                    // On "odd" horizontal tunnels we leave tunnel space on the right
                    if (row % (TUNNEL_UNIT * 3) == TUNNEL_UNIT * 3 - 1) {
                        passages[row] = std::vector<int>(TUNNEL_UNIT * 2);
                        std::iota(
                            passages[row].begin(),
                            passages[row].end(),
                            WIDTH - (TUNNEL_UNIT * 2)
                        );
                        breaches[row] = {};
                    } // One of the breaches in the wall is always the built-in one
                    break; // On "odd" horizontal tunnels we leave tunnel space on the right
                }
                if (field->isFree((unsigned short)row, (unsigned short)column)) {
                    field->addPawn(new Wall({row, column}, INITIAL_WALL_STRENGTH - row / TUNNEL_UNIT / 3));
                }
            }
        }
    }
}

void spawnInitialEnemies() {
    std::deque<sista::Coordinates> freeBaseCoordinates;
    for (int column = 0; column < WIDTH; column++) {
        sista::Coordinates coords = {HEIGHT - 1, column};
        if (field->isFree(coords)) {
            freeBaseCoordinates.push_back({HEIGHT - 1, column});
        }
    }
    std::shuffle(freeBaseCoordinates.begin(), freeBaseCoordinates.end(), rng);

    for (int i = 0; i < INITIAL_ARCHERS; i++) {
        field->addPawn(new Archer(freeBaseCoordinates.front()));
        freeBaseCoordinates.pop_front();
    }
    for (int i = 0; i < INITIAL_WORMS; i++) {
        field->addPawn(new Worm(freeBaseCoordinates.front(), Direction::UP));
        freeBaseCoordinates.pop_front();
    }
}

void spawnEnemies() {
    if (Archer::spawning(rng)) {
        sista::Coordinates coords = {HEIGHT - 1, rand() % WIDTH};
        if (field->isFree(coords)) {
            field->addPrintPawn(new Archer(coords));
        }
    }
    if (Worm::spawning(rng)) {
        sista::Coordinates coords = {HEIGHT - 1, rand() % WIDTH};
        if (field->isFree(coords)) {
            field->addPrintPawn(new Worm(coords, Direction::UP));
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
    for (auto mine : Mine::mines) {
        delete mine;
    }
    for (auto bullet : EnemyBullet::enemyBullets) {
        delete bullet;
    }
    for (auto archer : Archer::archers) {
        delete archer;
    }
    for (auto wormBody : WormBody::wormBodies) {
        delete wormBody;
    }
    for (auto worm : Worm::worms) {
        delete worm;
    }
}

// Entity::Entity() : sista::Pawn(' ', sista::Coordinates(0, 0), Player::playerStyle), type(Type::PLAYER) {}
Entity::Entity(char symbol, sista::Coordinates coordinates, ANSI::Settings& settings, Type type) :
    sista::Pawn(symbol, coordinates, settings), type(type) {}


// Wall::Wall() : Entity('#', {0, 0}, wallStyle, Type::WALL) {}
Wall::Wall(sista::Coordinates coordinates, short int strength) :
    Entity('#', coordinates, wallStyle, Type::WALL), strength(strength) {
    Wall::walls.push_back(this);
}
void Wall::remove() {
    Wall::walls.erase(std::find(Wall::walls.begin(), Wall::walls.end(), this));
    field->erasePawn(this);
    delete this;
}
bool Wall::getHit() {
    if (--strength <= 0) {
        // Verify if this creates a new breach by a DFS
        std::stack<sista::Coordinates> dfs({
            coordinates + directionMap[Direction::UP],
            coordinates + directionMap[Direction::LEFT],
            coordinates + directionMap[Direction::DOWN],
            coordinates + directionMap[Direction::RIGHT]
        });
        std::set<sista::Coordinates> visited;
        visited.insert({coordinates});
        sista::Coordinates coords, breach;
        bool foundBelowExit = false;
        bool foundAboveExit = false;
        while (!dfs.empty() && !foundBelowExit && !foundAboveExit) {
            coords = dfs.top();
            dfs.pop();

            if (field->isOutOfBounds(coords)) continue; // Exiting the field
            if (visited.count(coords)) continue; // Already visited
            visited.insert(coords);

            if (field->isOccupied(coords)) { // Cell is not free
                Type type = ((Entity*)field->getPawn(coords))->type;
                if (type == Type::WALL || type == Type::PORTAL) continue;
            }
            if (coords.y % (TUNNEL_UNIT * 3) == 0) { // Exiting the breach from below
                foundBelowExit = true;
                // The breach was right above for how neighbouring is defined in a grid
                breach = coords + directionMap[Direction::UP];
            }
            if (coords.y % (TUNNEL_UNIT * 3) < TUNNEL_UNIT * 2) { // Exiting the breach on the upper side
                foundAboveExit = true;
                continue;
            }
            if (coords.x < TUNNEL_UNIT * 2
                && (coords.y / TUNNEL_UNIT / 3) % 2 == 0) {
                // On "even" tunnels we should not consider part of the breach the part to the left
                continue;
            } else if (coords.x >= WIDTH-(TUNNEL_UNIT * 2)
                        && (coords.y / TUNNEL_UNIT / 3) % 2 == 1) {
                // On "odd" tunnels we should not consider part of the breach the part to the right
                continue;
            }
            if (foundBelowExit && foundAboveExit) break;

            dfs.push(coords + directionMap[Direction::UP]);
            dfs.push(coords + directionMap[Direction::LEFT]);
            dfs.push(coords + directionMap[Direction::DOWN]);
            dfs.push(coords + directionMap[Direction::RIGHT]);
        }
        if (foundBelowExit && foundAboveExit) {
            if (breaches.count(breach.y)) {
                breaches[breach.y].push_back(breach.x);
            } else {
                breaches[breach.y] = {breach.x};
            }
        }
        this->remove();
        return true;
    }
    return false;
}
std::bernoulli_distribution Wall::wearing(WALL_WEARING_PROBABILITY);
ANSI::Settings Wall::wallStyle = {
    RGB_ROCKS_FOREGROUND,
    RGB_ROCKS_BACKGROUND,
    ANSI::Attribute::BRIGHT
};

// Bullet::Bullet() : Entity('>', {0, 0}, bulletStyle, Type::BULLET), direction(Direction::RIGHT), collided(false) {}
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
            case Type::MINE:
                ((Mine*)entity)->trigger();
                break;
            case Type::ENEMY_BULLET: {
                EnemyBullet* bullet = (EnemyBullet*)entity;
                if (bullet->collided) return;
                bullet->collided = true;
                break;
            }
            case Type::BULLET: {
                Bullet* bullet = (Bullet*)entity;
                if (bullet->collided) return;
                bullet->collided = true;
                break;
            }
            case Type::ARCHER: {
                ((Archer*)entity)->die();
                break;
            }
            case Type::WORM_HEAD: {
                Worm* worm = (Worm*)entity;
                worm->getHit();
                break;
            }
            case Type::WORM_BODY: {
                WormBody* wormBody = (WormBody*)entity;
                wormBody->die();
                break;
            }
            default:
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

// EnemyBullet::EnemyBullet() : Entity('>', {0, 0}, enemyBulletStyle, Type::ENEMY_BULLET), direction(Direction::RIGHT), collided(false) {}
EnemyBullet::EnemyBullet(sista::Coordinates coordinates, Direction direction) :
    Entity(directionSymbol[direction], coordinates, enemyBulletStyle, Type::ENEMY_BULLET), direction(direction), collided(false) {
    EnemyBullet::enemyBullets.push_back(this);
}
void EnemyBullet::remove() {
    EnemyBullet::enemyBullets.erase(std::find(EnemyBullet::enemyBullets.begin(), EnemyBullet::enemyBullets.end(), this));
    field->erasePawn(this);
    delete this;
}
void EnemyBullet::move() {
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
            case Type::MINE:
                ((Mine*)entity)->trigger();
                break;
            case Type::BULLET: {
                Bullet* bullet = (Bullet*)entity;
                if (bullet->collided) return;
                bullet->collided = true;
                break;
            }
            case Type::ENEMY_BULLET: {
                EnemyBullet* bullet = (EnemyBullet*)entity;
                if (bullet->collided) return;
                bullet->collided = true;
                break;
            }
            case Type::WORM_HEAD: {
                Worm* worm = (Worm*)entity;
                worm->getHit();
                break;
            }
            case Type::WORM_BODY: {
                WormBody* wormBody = (WormBody*)entity;
                wormBody->die();
                break;
            }
            case Type::PLAYER:
                end = true;
                break;
            default:
                break;
        }
        this->remove(); // Hit something and the situation was not handled
    }
}
ANSI::Settings EnemyBullet::enemyBulletStyle = {
    ANSI::ForegroundColor::F_GREEN,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};

// Chest::Chest() : Entity('C', {0, 0}, chestStyle, Type::CHEST), inventory(INITIAL_INVENTORY) {}
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

// Portal::Portal() : Entity('&', {0, 0}, portalStyle, Type::PORTAL) {}
Portal::Portal(sista::Coordinates coordinates) : Entity('&', coordinates, portalStyle, Type::PORTAL) {
    Portal::portals.push_back(this);
}
void Portal::remove() {
    Portal::portals.erase(std::find(Portal::portals.begin(), Portal::portals.end(), this));
    field->erasePawn(this);
    delete this;
}
ANSI::Settings Portal::portalStyle = {
    RGB_ROCKS_FOREGROUND,
    RGB_ROCKS_BACKGROUND,
    ANSI::Attribute::FAINT
};

// Mine::Mine() : Entity('*', {0, 0}, mineStyle, Type::MINE), triggered(false) {}
Mine::Mine(sista::Coordinates coordinates) : Entity('*', coordinates, mineStyle, Type::MINE), triggered(false) {
    Mine::mines.push_back(this);
}
void Mine::remove() {
    Mine::mines.erase(std::find(Mine::mines.begin(), Mine::mines.end(), this));
    field->erasePawn(this);
    delete this;
}
void Mine::trigger() {
    if (this->triggered) return;
    this->triggered = true;
    this->setSettings(Mine::triggeredMineStyle);
    this->setSymbol('%');
    field->rePrintPawn(this);
}
void Mine::explode() {
    for (int j=-2; j<=2; j++) {
        for (int i=-2; i<=2; i++) {
            if (i == 0 && j == 0) continue;
            sista::Coordinates target = this->coordinates + sista::Coordinates(j, i);
            if (field->isOutOfBounds(target)) continue;
            if (field->isOccupied(target)) {
                Entity* entity = (Entity*)field->getPawn(target);
                switch (entity->type) {
                    case Type::MINE:
                        ((Mine*)entity)->trigger();
                        break;
                    case Type::WALL: {
                        int damage = mineDamage(rng);
                        while (!((Wall*)entity)->getHit() && --damage);
                        break;
                    }
                    case Type::PLAYER: case Type::PORTAL:
                        break; // Player and Portals get no damage from mines
                    default:
                        entity->remove();
                }
            }
        }
    }
    this->remove();
}
std::bernoulli_distribution Mine::explosion(MINE_EXPLOSION_IN_FRAME_PROBABILITY);
std::uniform_int_distribution<int> Mine::mineDamage(MINE_MINIMUM_DAMAGE, MINE_MAXIMUM_DAMAGE);
ANSI::Settings Mine::mineStyle = {
    ANSI::RGBColor(200, 100, 200),
    RGB_ROCKS_FOREGROUND,
    ANSI::Attribute::BRIGHT
};
ANSI::Settings Mine::triggeredMineStyle = {
    RGB_BLACK,
    ANSI::RGBColor(0xff, 0, 0),
    ANSI::Attribute::BLINK
};

// Archer::Archer() : Entity('A', {0, 0}, archerStyle, Type::ARCHER) {}
Archer::Archer(sista::Coordinates coordinates) : Entity('A', coordinates, archerStyle, Type::ARCHER) {
    Archer::archers.push_back(this);
}
void Archer::move() {
    // There is always a probability of a dumb move
    if (dumbMoveDistribution(rng)) {
        Direction direction = (Direction)(rand() % 4);
        this->move(direction);
        return;
    }

    auto [row, column] = coordinates;
    sista::Coordinates above = coordinates + directionMap[Direction::UP];
    // It could be right below a passage or breach
    if (row % (TUNNEL_UNIT * 3) == 0) {
        if (std::find(breaches[above.y].begin(), breaches[above.y].end(), column) != breaches[above.y].end()) {
            // There is a breach right above, so try to step in
            this->move(Direction::UP);
            return;
        } else if (std::find(passages[above.y].begin(), passages[above.y].end(), column) != passages[above.y].end()) {
            // There is a passage, so try to ascend
            this->move(Direction::UP);
            return;
        }
    }
    // It is in a passage or breach (in the proper range on y axis)
    if (row % (TUNNEL_UNIT * 3) >= TUNNEL_UNIT * 2) {
        // it uses BFS to pick the next move to get to the other side
        std::queue<std::pair<sista::Coordinates, Direction>> bfs({
            {coordinates + directionMap[Direction::UP], Direction::UP},
            {coordinates + directionMap[Direction::LEFT], Direction::LEFT},
            {coordinates + directionMap[Direction::DOWN], Direction::DOWN},
            {coordinates + directionMap[Direction::RIGHT], Direction::RIGHT}
        }); // {coordinate, first direction taken in the path}
        std::set<sista::Coordinates> visited;
        visited.insert({coordinates});
        Direction chosenMove;
        bool found = false;
        while (!bfs.empty()) {
            auto [coords, choice] = bfs.front();
            bfs.pop();

            if (field->isOutOfBounds(coords)) continue; // Exiting the field
            if (std::find(visited.begin(), visited.end(), coords) != visited.end()) continue; // Already visited
            if (field->isOccupied(coords)) { // Cell is not free
                Type type = ((Entity*)field->getPawn(coords))->type;
                if (type == Type::WALL || type == Type::PORTAL) continue;
            }
            if (coords.y % (TUNNEL_UNIT * 3) == 0) continue; // Exiting the breach
            if (coords.y % (TUNNEL_UNIT * 3) < TUNNEL_UNIT * 2) { // Exiting the breach on the upper side
                chosenMove = choice;
                found = true;
                break;
            }
            visited.insert(coords);

            bfs.push({coords + directionMap[Direction::UP], choice});
            bfs.push({coords + directionMap[Direction::LEFT], choice});
            bfs.push({coords + directionMap[Direction::DOWN], choice});
            bfs.push({coords + directionMap[Direction::RIGHT], choice});
        }
        if (found) {
            this->move(chosenMove);
            return;
        } else {
            Direction direction = (Direction)(rand() % 4);
            this->move(direction);
            return;
        }
    }
    // It roughly (probabilistically by x/y ratio) points to the centermost (or a random-but-deterministic one) breach
    int next_passage_y = (row - (row % (TUNNEL_UNIT * 3))) - 1;
    int next_passage_x;
    if (!breaches[next_passage_y].empty()) {
        std::vector<int> centermost_breaches = breaches[next_passage_y];
        if (!centermost_breaches.empty()) {
            std::sort(centermost_breaches.begin(), centermost_breaches.end(), [](int a, int b) {
                return std::abs(a - WIDTH/2) < std::abs(b - WIDTH/2);
            });
            next_passage_x = centermost_breaches[(reinterpret_cast<intptr_t>(this)) % centermost_breaches.size()];
        }
    } else {
        // If no breaches are found, then it points to any passage of the following level (random and not necessarily deterministic)
        std::vector<int> available_passages = passages[next_passage_y];
        if (!available_passages.empty()) {
            next_passage_x = available_passages[rand() % available_passages.size()];
        } else {
            // If there is no passage on the next, it means we're pointing to a negative coordinate
            // It is dangerous as generating coordinates from that could lead to segmentation faults
            // but since we only use it to decide the next move it's actually innocuous
            next_passage_x = WIDTH - 1; // Point to the top right
        }
    }
    int delta_y = next_passage_y - row;
    int delta_x = next_passage_x - column;
    float ratio = (std::min(std::abs(delta_y), std::abs(delta_x)) + 1)
                / (std::max(std::abs(delta_y), std::abs(delta_x)) + 1);
    std::bernoulli_distribution verticalNotHorizontalDistribution(ratio);
    if (verticalNotHorizontalDistribution(rng)) {
        this->move(Direction::UP);
    } else {
        if (delta_x >= 0) {
            this->move(Direction::RIGHT);
        } else {
            this->move(Direction::LEFT);
        }
    }
}
bool Archer::move(Direction direction) {
    sista::Coordinates next = this->coordinates + directionMap[direction];
    if (field->isOutOfBounds(next)) {
        return false;
    } else if (field->isFree(next)) {
        field->movePawn(this, next);
        return true;
    } else if (field->isOccupied(next)) {
        Entity* entity = (Entity*)field->getPawn(next);
        switch (entity->type) {
            case Type::MINE:
                ((Mine*)entity)->trigger();
            default:
                return false;
        }
    }
}
void Archer::shoot() {
    if (dumbMoveDistribution(rng)) {
        this->shoot((Direction)(rand() % 4));
        return;
    }
    if (coordinates.x == Player::player->getCoordinates().x) {
        // Roughly vertically aligned with the player
        if (coordinates.y / (TUNNEL_UNIT * 3) != Player::player->getCoordinates().y / (TUNNEL_UNIT * 3)) {
            // They was a wall between them so the archer cannot see
            int next_breaches_y = (coordinates.y % (TUNNEL_UNIT * 3)) - 1;
            if (!breaches[next_breaches_y].empty()) {
                if (std::find(breaches[next_breaches_y].begin(), breaches[next_breaches_y].end(), coordinates.x) != breaches[next_breaches_y].end()) {
                    // ...unless there is a breach right above that allows the archer to hear the player moving behind the wall
                    this->shoot(Direction::UP);
                    return;
                }
            }
            this->shoot((Direction)(rand() % 4));
            return;
        }
        if (coordinates.y < Player::player->getCoordinates().y) {
            this->shoot(Direction::DOWN);
        } else if (coordinates.y > Player::player->getCoordinates().y) {
            this->shoot(Direction::UP);
        }
        // Exactly vertically aligned with the player
        if (coordinates.x == Player::player->getCoordinates().x) {
            this->move(Direction::DOWN); // Dodges incoming bullets
        }
    } else if (coordinates.y == Player::player->getCoordinates().y) {
        // Roughly horizontally aligned with the player
        if (coordinates.x < Player::player->getCoordinates().x) {
            this->shoot(Direction::RIGHT);
        } else if (coordinates.x > Player::player->getCoordinates().x) {
            this->shoot(Direction::LEFT);
        }
        // Exactly horizontally aligned with the player
        if (coordinates.y == Player::player->getCoordinates().y) {
            this->move(Direction::LEFT); // Dodges incoming bullets
        }
    } else {
        this->shoot((Direction)(rand() % 4));
    }
}
bool Archer::shoot(Direction direction) {
    sista::Coordinates target = this->coordinates + directionMap[direction];
    if (field->isOutOfBounds(target)) {
        return false;
    } else if (field->isFree(target)) {
        field->addPrintPawn(new EnemyBullet(target, direction));
        return true;
    } else if (field->isOccupied(target)) {
        Entity* entity = (Entity*)field->getPawn(target);
        switch (entity->type) {
            case Type::WALL:
                ((Wall*)entity)->getHit();
                break;
            case Type::PLAYER: // Counts as a dagger hit
                end = true;
                break;
            case Type::MINE:
                ((Mine*)entity)->trigger();
                break;
            default:
                break;
        }
        return false;
    }
}
void Archer::die() {
    Archer::archers.erase(std::find(Archer::archers.begin(), Archer::archers.end(), this));
    field->erasePawn(this);
    field->addPrintPawn(new Chest(coordinates, {
        LOOT_ARCHER_CLAY,
        LOOT_ARCHER_BULLETS,
        LOOT_ARCHER_MEAT
    }));
    delete this;
}
void Archer::remove() {
    Archer::archers.erase(std::find(Archer::archers.begin(), Archer::archers.end(), this));
    field->erasePawn(this);
    delete this;
}
std::bernoulli_distribution Archer::moving(ARCHER_MOVING_PROBABILITY);
std::bernoulli_distribution Archer::shooting(ARCHER_SHOOTING_PROBABILITY);
std::bernoulli_distribution Archer::spawning(ARCHER_SPAWNING_PROBABILITY);
ANSI::Settings Archer::archerStyle = {
    ANSI::ForegroundColor::F_CYAN,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::STRIKETHROUGH
};

// WormBody::WormBody() : Entity('<', {0, 0}, wormBodyStyle, Type::WORM_BODY) {}
WormBody::WormBody(sista::Coordinates coordinates, Direction direction) : Entity(directionSymbol[direction], coordinates, wormBodyStyle, Type::WORM_BODY) {
    WormBody::wormBodies.push_back(this);
}
void WormBody::die() {
    sista::Coordinates drop = this->coordinates;
    field->addPrintPawn(new Chest(drop, {0, 1, 0}));
    WormBody::wormBodies.erase(std::find(WormBody::wormBodies.begin(), WormBody::wormBodies.end(), this));
    this->head->body.erase(std::find(this->head->body.begin(), this->head->body.end(), this));
    delete this;
}
void WormBody::remove() {
    WormBody::wormBodies.erase(std::find(WormBody::wormBodies.begin(), WormBody::wormBodies.end(), this));
    field->erasePawn(this);
    delete this;
}
ANSI::Settings WormBody::wormBodyStyle = ANSI::Settings(
    ANSI::ForegroundColor::F_GREEN,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
);

// Worm::Worm() : Entity('H', {0, 0}, wormHeadStyle, Type::WORM_BODY) {}
Worm::Worm(sista::Coordinates coordinates) : Entity('H', coordinates, wormHeadStyle, Type::WORM_HEAD), hp(WORM_HEALTH_POINTS) {
    direction = (Direction)(rand() % 4);
    Worm::worms.push_back(this);
}
Worm::Worm(sista::Coordinates coordinates, Direction direction) : Worm(coordinates) {
    this->direction = direction;
}
void Worm::move() {
    sista::Coordinates oldHeadCoordinates = coordinates;
    sista::Coordinates next = coordinates + directionMap[direction];
    if (field->isOutOfBounds(next)) {
        Direction toTheLeft = (Direction)((direction + 3) % 4);
        Direction toTheRight = (Direction)((direction + 1) % 4);
        Direction oldDirection = direction;
        direction = ((Direction[]){toTheLeft, toTheRight})[rand() % 2];
        next = coordinates + directionMap[direction];
        if (field->isOutOfBounds(next)) {
            if (direction == toTheLeft) {
                next = coordinates + directionMap[toTheRight];
                if (field->isOutOfBounds(next)) {
                    direction = oldDirection;
                    this->getHit();
                    return;
                }
            } else if (direction == toTheRight) {
                next = coordinates + directionMap[toTheLeft];
                if (field->isOutOfBounds(next)) {
                    direction = oldDirection;
                    this->getHit();
                    return;
                }
            }
        }
    }
    if (field->isFree(next)) {
        /* Movement inspired from Dune (https://github.com/Lioydiano/Dune/blob/90a1f9c412258f701e3dfe949b05c6bcaa171e9f/dune.cpp#L386) */
        field->movePawn(this, next);
        // We now create a piece of body to leave behind the head, a "neck"
        WormBody* neck = new WormBody(oldHeadCoordinates, direction);
        neck->head = this;
        field->addPrintPawn(neck);
        body.push_back(neck);
        // Consider that we added a body piece, so we need to ensure it does not grow too much
        if (body.size() > WORM_LENGTH) {
            WormBody* tail = body.front();
            sista::Coordinates drop = tail->getCoordinates();
            field->erasePawn(tail);
            if (clayRelease(rng)) {
                field->addPrintPawn(new Chest(drop, {1, 0, 0}));
            }
            body.erase(body.begin());
            WormBody::wormBodies.erase(std::find(WormBody::wormBodies.begin(), WormBody::wormBodies.end(), tail));
            delete tail;
        }
    } else if (field->isOccupied(next)) {
        Entity* entity = (Entity*)field->getPawn(next);
        switch (entity->type) {
            case Type::PLAYER:
                end = true;
                break;
            case Type::WALL:
                if (((Wall*)entity)->strength > 1)
                    ((Wall*)entity)->getHit(); // They can weaken a wall but not destroy it
                this->turn(((Direction[]){Direction::LEFT, Direction::RIGHT})[rand() % 2]);
                break;
            case Type::WORM_HEAD:
                ((Worm*)entity)->getHit();
            case Type::WORM_BODY: case Type::PORTAL:
                this->turn(((Direction[]){Direction::LEFT, Direction::RIGHT})[rand() % 2]);
                break;
            default:
                entity->remove();
        }
    }
}
void Worm::turn(Direction direction_) {
    if (direction_ == Direction::LEFT)
        this->direction = (Direction)((this->direction + 3) % 4);
    else if (direction_ == Direction::RIGHT)
        this->direction = (Direction)((this->direction + 1) % 4);
}
void Worm::getHit() {
    if (--hp <= 0) {
        this->die();
    }
}
void Worm::die() {
    while (!body.empty()) {
        WormBody* tail = body.front();
        body.erase(body.begin());
        tail->die();
    }
    Worm::worms.erase(std::find(Worm::worms.begin(), Worm::worms.end(), this));
    field->erasePawn(this);
    field->addPrintPawn(new Chest(coordinates, {
        LOOT_WORM_HEAD_CLAY,
        LOOT_WORM_HEAD_BULLETS,
        LOOT_WORM_HEAD_MEAT
    }));
    delete this;
}
void Worm::remove() {
    Worm::worms.erase(std::find(Worm::worms.begin(), Worm::worms.end(), this));
    field->erasePawn(this);
    delete this;
}
std::bernoulli_distribution Worm::turning(WORM_TURNING_PROBABILITY);
std::bernoulli_distribution Worm::spawning(WORM_SPAWNING_PROBABILITY);
std::bernoulli_distribution Worm::clayRelease(CLAY_RELEASE_PROBABILITY);
ANSI::Settings Worm::wormHeadStyle = ANSI::Settings(
    ANSI::ForegroundColor::F_GREEN,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::RAPID_BLINK
);


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
        case Mode::MINE:
            if (this->inventory.containsAtLeast(COST_OF_MINE)) {
                this->inventory -= COST_OF_MINE;
                field->addPrintPawn(new Mine(target));
            }
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
std::map<int, std::vector<int>> passages; // Lateral passages, "main tunnel" tresholds
std::map<int, std::vector<int>> breaches; // Central breaches, "holes"
std::bernoulli_distribution dumbMoveDistribution(DUMB_MOVE_PROBABILITY);

void Inventory::operator+=(const Inventory& other) {
    clay += other.clay;
    bullets += other.bullets;
    meat += other.meat;
}
void Inventory::operator-=(const Inventory& other) {
    clay -= std::max(other.clay, (short)0);
    bullets -= std::max(other.bullets, (short)0);
    meat -= std::max(other.meat, (short)0);
}
inline bool Inventory::containsAtLeast(const Inventory other) const {
    return clay >= other.clay && bullets >= other.bullets && meat >= other.meat;
}
