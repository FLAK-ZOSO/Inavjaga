#include "constants.hpp"
#include "chest.hpp"

extern std::shared_ptr<sista::SwappableField> field;

Chest::Chest(sista::Coordinates coordinates, Inventory inventory) : Entity('C', coordinates, chestStyle, Type::CHEST), inventory(inventory) {
    // ownership moved to creator via std::shared_ptr; do not push here
}
void Chest::remove() {
    [[maybe_unused]] auto keepAlive = Entity::keepAliveFrom(Chest::chests, this);
    field->erasePawn(this);
    Entity::removeOwner(Chest::chests, this);
}
sista::ANSISettings Chest::chestStyle = {
    sista::RGBColor(193, 201, 104),
    RGB_BLACK,
    sista::Attribute::REVERSE
};