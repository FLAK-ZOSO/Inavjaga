#include "../include/sista/sista.hpp"
#include "inventory.hpp"
#include "entity.hpp"
#pragma once

class Chest : public Entity {
public:
    static sista::ANSISettings chestStyle;
    static std::vector<Chest*> chests;
    Inventory inventory;

    Chest(sista::Coordinates, Inventory);
    void remove() override;
};