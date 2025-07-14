#include "inventory.hpp"
#include <algorithm>

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