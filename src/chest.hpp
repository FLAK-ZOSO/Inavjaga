#include "../include/sista/sista.hpp"
#include "inventory.hpp"
#include "entity.hpp"
#include <memory>
#pragma once

class Chest : public Entity {
public:
    static sista::ANSISettings chestStyle;
    static std::vector<std::shared_ptr<Chest>> chests;
    Inventory inventory;

    Chest(sista::Coordinates, Inventory);
    void remove() override;
};