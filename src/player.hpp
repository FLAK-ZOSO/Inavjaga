#include "../include/sista/sista.hpp"
#include "constants.hpp"
#include "direction.hpp"
#include "entity.hpp"
#include "inventory.hpp"
#include <unordered_map>
#pragma once

extern const Inventory INITIAL_INVENTORY;
extern std::unordered_map<Direction, sista::Coordinates> directionMap;
extern sista::SwappableField* field;

class Player : public Entity {
public:
    static ANSI::Settings playerStyle;
    static Player* player;
    enum Mode {
        COLLECT, BULLET,
        DUMPCHEST, TRAP, MINE
    } mode;
    Inventory inventory;

    Player();
    Player(sista::Coordinates);
    void remove() override;

    void move(Direction);
    void shoot(Direction);
};