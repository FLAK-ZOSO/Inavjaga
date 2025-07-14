#include "constants.hpp"
#include "chest.hpp"

extern sista::SwappableField* field;

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