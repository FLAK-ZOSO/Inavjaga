#include "inventory.hpp"
#pragma once

inline bool Inventory::containsAtLeast(const Inventory other) const {
    return clay >= other.clay && bullets >= other.bullets && meat >= other.meat;
}