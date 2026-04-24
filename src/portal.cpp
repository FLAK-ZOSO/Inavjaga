#include "portal.hpp"
#include "constants.hpp"

extern std::shared_ptr<sista::SwappableField> field;

Portal::Portal(sista::Coordinates coordinates) : Entity('&', coordinates, portalStyle, Type::PORTAL) {
    // ownership moved to creator via std::shared_ptr; do not push here
}
Portal::Portal(sista::Coordinates coordinates, std::weak_ptr<Portal> exit) : Portal(coordinates) {
    this->exit = exit;
}
void Portal::remove() {
    [[maybe_unused]] auto keepAlive = Entity::keepAliveFrom(Portal::portals, this);
    field->erasePawn(this);
    Entity::removeOwner(Portal::portals, this);
}
sista::ANSISettings Portal::portalStyle = {
    RGB_ROCKS_FOREGROUND,
    RGB_ROCKS_BACKGROUND,
    sista::Attribute::FAINT
};