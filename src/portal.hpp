#include "entity.hpp"
#include <memory>
#pragma once

class Portal : public Entity {
public:
    static sista::ANSISettings portalStyle;
    static std::vector<std::shared_ptr<Portal>> portals;
    std::weak_ptr<Portal> exit; // The matching portal

    Portal(sista::Coordinates);
    Portal(sista::Coordinates, std::weak_ptr<Portal>);
    void remove() override;
};