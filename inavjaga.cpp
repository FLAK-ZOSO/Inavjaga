#include "include/cross_platform.hpp"
#include "inavjaga.hpp"
#include <algorithm>
#include <future>
#include <thread>
#include <chrono>
#include <mutex>
#include <stack>

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
std::mutex streamMutex;
bool speedup = false;
bool pause_ = false;
int lastDeathFrame = 0;
bool dead = false;
bool end = false;


int main(int argc, char* argv[]) {
    #ifdef __APPLE__
        term_echooff();
    #endif
    std::ios_base::sync_with_stdio(false);
    ANSI::reset(); // Reset the settings
    srand(time(0)); // Seed the random number generator

    sista::SwappableField field_(WIDTH, HEIGHT);
    field = &field_;
    generateTunnels();
    Player::player = new Player(SPAWN_COORDINATES);
    Player::player->mode = Player::Mode::BULLET;
    field->addPawn(Player::player);
    #if INTRO
    intro();
    #endif
    #if TUTORIAL
    tutorial();
    #endif
    spawnInitialEnemies();
    field->print(border);
    std::thread th(input);
    for (int i=0; !end; i++) {
        while (pause_) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (!pause_) {
                std::lock_guard<std::mutex> lock(streamMutex); // Lock stays until scope ends
                reprint();
            } // Reprint after unpausing, just as a tool for allowing resizing
            if (end) break;
        }
        if (dead) {
            dead = false;
            lastDeathFrame = i;
            sista::Coordinates deathCoordinates = Player::player->getCoordinates();
            sista::Coordinates respawnCoordinates = RESPAWN_COORDINATES;
            field->movePawn(Player::player, respawnCoordinates);
            #if DROP_INVENTORY_ON_DEATH
            field->addPrintPawn(new Chest(deathCoordinates, {
                Player::player->inventory.clay,
                Player::player->inventory.bullets,
                0
            }));
            #endif
            Player::player->inventory.clay = 0;
            Player::player->inventory.bullets = 0;
        }
        if (lastDeathFrame && i - lastDeathFrame == 20) {
            // After 20 frames it deletes the death reason
            std::lock_guard<std::mutex> lock(streamMutex); // Lock stays until scope ends
            reprint();
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
            if (worm->collided) continue;
            if (Worm::turning(rng)) {
                worm->turn();
            }
            if (Worm::moving(rng)) {
                worm->move();
            }
        }
        for (Worm* worm : Worm::worms) {
            if (worm->collided) {
                worm->die();
            }
        }
        for (unsigned j = 0; j < Mine::mines.size(); j++) {
            if (j >= Mine::mines.size()) break;
            Mine* mine = Mine::mines[j];
            if (!mine->triggered) {
                for (int k=-MINE_SENSITIVITY_RADIUS; k<=MINE_SENSITIVITY_RADIUS; k++) {
                    for (int h=-MINE_SENSITIVITY_RADIUS; h<=MINE_SENSITIVITY_RADIUS; h++) {
                        if (k == 0 && h == 0) continue;
                        sista::Coordinates target = mine->getCoordinates() + sista::Coordinates(k, h);
                        if (field->isOutOfBounds(target)) {
                            continue;
                        } else if (field->isOccupied(target)) {
                            Entity* entity = (Entity*)field->getPawn(target);
                            if (entity->type == Type::WORM_HEAD) {
                                mine->trigger();
                            }
                        }
                    }
                }
            }
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
        // Check for negative amount of meat
        if (Player::player->inventory.meat < 0) {
            printEndInformation(EndReason::STARVED);
            dead = true;
            end = true;
        }
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

    end = true; // Needed to ensure the input function returns and the thread th gets joined
    deallocateAll();
    th.join();
    field->clear();
    cursor.set(72, 0); // Move the cursor to the bottom of the screen, so the terminal is not left in a weird state
    std::this_thread::sleep_for(std::chrono::seconds(1)); // Give the time to see the final screen
    flushInput();
    #if __linux__
        getch();
    #elif __APPLE__
        getchar();
    #elif _WIN32
        getch();
    #endif
    #ifdef __APPLE__
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
                    printEndInformation(EndReason::TOUCHDOWN);
                    return true;
                }
            }
        }
    }
    return false;
}

void intro() {
    #if defined(__linux__)
    std::future<int> future = std::async([](){ return static_cast<int>(getch()); });
    std::chrono::duration refresh = std::chrono::milliseconds(FRAME_DURATION);
    #elif defined(_WIN32)
    std::future<int> future = std::async(getch);
    // Lower refresh rate for Windows to contain the flickering effect in the terminal
    std::chrono::duration refresh = std::chrono::milliseconds(FRAME_DURATION * 10);
    #elif defined(__APPLE__)
    std::future<int> future = std::async(getchar);
    std::chrono::duration refresh = std::chrono::milliseconds(FRAME_DURATION);
    #endif
    while (true) {
        std::cout << "Make sure that the following hash signs fit the best in a line in your terminal.\n";
        std::cout << "Use ctrl+<minus> and ctrl+<plus> or ctrl+<mouse-scroll> to resize your terminal.\n";
        std::cout << "Maximize your terminal window for an optimal view on the field, then enter any key to proceed.\n";
        field->print(border);
        ANSI::reset();
        cursor.set(8, (unsigned short)(WIDTH / 2.1));
        std::cout << "Inävjaga";
        cursor.set(TUNNEL_UNIT * 3 + 7, TUNNEL_UNIT * 2 + 2);
        Player::playerStyle.apply();
        std::cout << "Inävjaga v" << VERSION;
        cursor.set(TUNNEL_UNIT * 3 + 7, (unsigned short)(WIDTH / 2.6));
        ANSI::reset();
        ANSI::setAttribute(ANSI::Attribute::ITALIC);
        ANSI::setAttribute(ANSI::Attribute::FAINT);
        std::cout << " originally by ";
        ANSI::reset();
        Player::playerStyle.apply();
        std::cout << AUTHOR << "     " << DATE;
        ANSI::reset();
        cursor.set(TUNNEL_UNIT * 3 + 9, (unsigned short)(WIDTH / 3.5));
        ANSI::setAttribute(ANSI::Attribute::UNDERSCORE);
        std::cout << "https://github.com/FLAK-ZOSO/Inavjaga";
        std::cout << std::flush;

        if (future.wait_for(refresh) == std::future_status::ready) {
            sista::clearScreen(true);
            return;
        }
        sista::clearScreen(true);
    }
}

void tutorial() {
    field->print(border);
    printSideInstructions(0);

    cursor.set(TUNNEL_UNIT * 3 + 3 + 1, 4);
    ANSI::reset();
    std::cout << "If you want to skip the tutorial, click 'n' at any point";
    cursor.set(TUNNEL_UNIT * 3 + 3 + 2, 4);
    std::cout << "To disable it, set TUTORIAL to 0 in constants.hpp and recompile";

    cursor.set(TUNNEL_UNIT * 4 + 4, 4);
    ANSI::reset();
    std::cout << "You are the ";
    Player::playerStyle.apply();
    std::cout << "$ player";
    ANSI::reset();
    std::cout << ", try moving around a bit" << std::endl;

    char input_;
    flushInput();
    for (int i = 0; i < 5; i++) {
        #if defined(_WIN32) or defined(__linux__)
            input_ = getch();
        #elif __APPLE__
            input_ = getchar();
        #endif
        if (movementKeys.find(input_) != movementKeys.end()) {
            act(input_);
        } else if (input_ == 'n') {
            flushInput();
            sista::clearScreen(true);
            return;
        }
        std::cout << std::flush;
    }
    field->addPrintPawn(new Archer({Player::player->getCoordinates().y, WIDTH - 3 * TUNNEL_UNIT - 1}));

    cursor.set(TUNNEL_UNIT * 5 + 3 + 1, 4);
    ANSI::reset();
    std::cout << "An ";
    Archer::archerStyle.apply();
    std::cout << "Archer";
    ANSI::reset();
    std::cout << "! Enter bullet mode and shoot it!" << std::endl;

    flushInput();
    while (input_ != 'j' && input_ != 'J') {
        #if defined(_WIN32) or defined(__linux__)
            input_ = getch();
        #elif __APPLE__
            input_ = getchar();
        #endif
        if (movementKeys.find(input_) != movementKeys.end())
            continue;
        if (input_ == 'n') {
            flushInput();
            sista::clearScreen(true);
            return;
        } else if (input_ == 'j' || input_ != 'J') {
            act(input_);
        }
        std::cout << std::flush;
    }

    while (!Archer::archers.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(FRAME_DURATION));
        Bullet::bullets[0]->move();
        std::cout << std::flush;
    }

    cursor.set(TUNNEL_UNIT * 6 + 3 + 1, 4);
    ANSI::reset();
    std::cout << "You can loot its ";
    Chest::chestStyle.apply();
    std::cout << "Chest";
    ANSI::reset();
    std::cout << ". Enter collect mode and pick it up." << std::endl;

    while (!Chest::chests.empty()) {
        #if defined(_WIN32) or defined(__linux__)
            input_ = getch();
        #elif __APPLE__
            input_ = getchar();
        #endif
        if (input_ == 'n') {
            flushInput();
            sista::clearScreen(true);
            return;
        }
        act(input_);
        printSideInstructions(0);
        std::cout << std::flush;
    }

    cursor.set(TUNNEL_UNIT * 7 + 3, 4);
    ANSI::reset();
    std::cout << "The rest is simple: protect the highlighted area.";
    cursor.set(TUNNEL_UNIT * 7 + 3 + 1, 4);
    std::cout << "Oh, and don't starve. You consume meat. That's all.";

    ANSI::Settings highlight(
        ANSI::ForegroundColor::F_RED,
        ANSI::BackgroundColor::B_RED,
        ANSI::Attribute::BLINK
    );
    std::vector<sista::Pawn*> highlightPawns;
    for (int i = 0; i < TUNNEL_UNIT * 2; i++) {
        for (int j = WIDTH - TUNNEL_UNIT * 2; j < WIDTH; j++) {
            sista::Pawn* pawn = new sista::Pawn(' ', {i, j}, highlight);
            highlightPawns.push_back(pawn);
            field->addPrintPawn(pawn);
        }
    }
    
    cursor.set(TUNNEL_UNIT * 7 + 3 + 4, WIDTH / 2.6);
    ANSI::reset();
    ANSI::setAttribute(ANSI::Attribute::ITALIC);
    ANSI::setAttribute(ANSI::Attribute::BLINK);
    std::cout << "Press any key to play Inävjaga...";
    std::cout << std::flush;
    std::this_thread::sleep_for(std::chrono::seconds(1));
    flushInput();

    #if defined(_WIN32) or defined(__linux__)
        input_ = getch();
    #elif __APPLE__
        input_ = getchar();
    #endif

    while (!highlightPawns.empty()) {
        field->erasePawn(highlightPawns[highlightPawns.size() - 1]);
        delete highlightPawns.back();
        highlightPawns.pop_back();
    }

    flushInput();
    sista::clearScreen(true);
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
void reprint() {
    sista::clearScreen(true);
    field->print(border);
    printKeys();
}

void generateTunnels() {
    std::uniform_int_distribution<int> distr(TUNNEL_UNIT * 2, WIDTH-(TUNNEL_UNIT * 2)-1); // Inclusive
    int portalCoordinate;
    for (int row=0; row<HEIGHT; row++) {
        if (row % (TUNNEL_UNIT * 3) == 0 && row + TUNNEL_UNIT * 3 < HEIGHT) {
            for (int i = 0; i < PORTALS_PER_LINE; i++) {
                sista::Coordinates abovePortalCoordinates;
                sista::Coordinates belowPortalCoordinates;
                do {
                    portalCoordinate = distr(rng);
                    abovePortalCoordinates = {row + TUNNEL_UNIT * 2, portalCoordinate};
                    belowPortalCoordinates = {row + TUNNEL_UNIT * 3 - 1, portalCoordinate};
                } while (!field->isFree(abovePortalCoordinates) || !field->isFree(belowPortalCoordinates));
                Portal* abovePortal = new Portal(abovePortalCoordinates);
                Portal* belowPortal = new Portal(belowPortalCoordinates);
                abovePortal->exit = belowPortal;
                belowPortal->exit = abovePortal;
                field->addPawn(abovePortal);
                field->addPawn(belowPortal);
            }
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
        // case 't': case 'T':
        //     Player::player->mode = Player::Mode::TRAP;
        //     break;
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
            printEndInformation(EndReason::QUIT);
            end = true;
            return;
        default:
            break;
    }
}

void printEndInformation(EndReason endReason) {
    cursor.set(HEIGHT, WIDTH + 10);
    
    ANSI::reset();
    ANSI::setAttribute(ANSI::Attribute::BLINK);
    switch (endReason) {
        case EndReason::EATEN:
            std::cout << "You have been eaten by a ";
            Worm::wormHeadStyle.apply();
            ANSI::setAttribute(ANSI::Attribute::BLINK);
            std::cout << "WORM";
            break;
        case EndReason::QUIT:
            std::cout << "You quit the game with the capital 'Q' key";
            break;
        case EndReason::SHOT:
            std::cout << "You have been shot with a ";
            EnemyBullet::enemyBulletStyle.apply();
            ANSI::setAttribute(ANSI::Attribute::BLINK);
            std::cout << "BULLET";
            break;
        case EndReason::STABBED:
            std::cout << "You have been stabbed by an ";
            Archer::archerStyle.apply();
            ANSI::setAttribute(ANSI::Attribute::BLINK);
            std::cout << "ARCHER";
            break;
        case EndReason::STARVED:
            std::cout << "You have starved because you ran out of meat";
            break;
        case EndReason::TOUCHDOWN:
            std::cout << "Some enemy reached the top right area";
            break;
        default:
            std::cout << "Something unexpected went wrong internally";
            break;
    }
    std::cout << std::flush;
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
std::set<char> movementKeys = {
    'w', 'W', 'd', 'D', 's', 'S', 'a', 'A'
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
