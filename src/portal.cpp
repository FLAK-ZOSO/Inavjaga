#include "portal.hpp"
#include "constants.hpp"

extern sista::SwappableField* field;

Portal::Portal(sista::Coordinates coordinates) : Entity('&', coordinates, portalStyle, Type::PORTAL) {
    Portal::portals.push_back(this);
}
void Portal::remove() {
    Portal::portals.erase(std::find(Portal::portals.begin(), Portal::portals.end(), this));
    field->erasePawn(this);
    delete this;
}
ANSI::Settings Portal::portalStyle = {
    RGB_ROCKS_FOREGROUND,
    RGB_ROCKS_BACKGROUND,
    ANSI::Attribute::FAINT
};