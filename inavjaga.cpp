#include "cross_platform.hpp"
#include "inavjaga.hpp"
#include <thread>
#include <chrono>

#define VERSION "0.0.1"
#define DATE "2025-06-28"


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
