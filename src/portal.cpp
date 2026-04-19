#include "portal.hpp"
#include "constants.hpp"

extern sista::SwappableField* field;

Portal::Portal(sista::Coordinates coordinates) : Entity('&', coordinates, portalStyle, Type::PORTAL) {
    // ownership moved to creator via std::shared_ptr; do not push here
}
void Portal::remove() {
    auto it = std::find_if(Portal::portals.begin(), Portal::portals.end(), [this](const std::shared_ptr<Portal>& p){ return p.get() == this; });
    std::shared_ptr<Portal> self;
    if (it != Portal::portals.end()) self = *it;
    field->erasePawn(this);
    if (it != Portal::portals.end()) Portal::portals.erase(it);
}
sista::ANSISettings Portal::portalStyle = {
    RGB_ROCKS_FOREGROUND,
    RGB_ROCKS_BACKGROUND,
    sista::Attribute::FAINT
};