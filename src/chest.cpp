#include "constants.hpp"
#include "chest.hpp"

extern sista::SwappableField* field;

Chest::Chest(sista::Coordinates coordinates, Inventory inventory) : Entity('C', coordinates, chestStyle, Type::CHEST), inventory(inventory) {
    // ownership moved to creator via std::shared_ptr; do not push here
}
void Chest::remove() {
    auto it = std::find_if(Chest::chests.begin(), Chest::chests.end(), [this](const std::shared_ptr<Chest>& p){ return p.get() == this; });
    if (it != Chest::chests.end()) Chest::chests.erase(it);
    field->erasePawn(this);
}
sista::ANSISettings Chest::chestStyle = {
    sista::RGBColor(193, 201, 104),
    RGB_BLACK,
    sista::Attribute::REVERSE
};