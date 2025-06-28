#include "cross_platform.hpp"
#include "inavjaga.hpp"
#include <thread>
#include <chrono>

#define VERSION "0.0.1"
#define DATE "2025-06-28"

#define WIDTH 70
#define HEIGHT 30

#define TUNNEL_UNIT 2

Player* Player::player;
std::vector<Wall*> Wall::walls;

sista::SwappableField* field;
sista::Cursor cursor;
sista::Border border(
    '@', {
        ANSI::ForegroundColor::F_BLACK,
        ANSI::BackgroundColor::B_WHITE,
        ANSI::Attribute::BRIGHT
    }
);


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

    field->print(border);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
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


void generateTunnels() {
    for (int row=0; row<HEIGHT; row++) {
        if (row % (TUNNEL_UNIT * 3) >= TUNNEL_UNIT) {
            for (int column=0; column<WIDTH; column++) {
                if (column < TUNNEL_UNIT * 2
                    && (row / TUNNEL_UNIT / 3) % 2 == 0) {
                    // On "even" horizontal tunnels we leave tunnel space on the left
                    column = TUNNEL_UNIT * 2;
                } else if (column >= WIDTH-(TUNNEL_UNIT * 2)
                            && (row / TUNNEL_UNIT / 3) % 2 == 1) {
                    break; // On "odd" horizontal tunnels we leave tunnel space on the right
                }
                field->addPawn(new Wall({row, column}, row));
            }
        }
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
ANSI::Settings Wall::wallStyle = {
    ANSI::ForegroundColor::F_BLUE,
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
