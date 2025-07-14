#include "include/sista/sista.hpp"
#include "entity.hpp"
#pragma once

class Portal : public Entity {
public:
    static ANSI::Settings portalStyle;
    static std::vector<Portal*> portals;
    Portal* exit; // The matching portal

    Portal(sista::Coordinates);
    Portal(sista::Coordinates, Portal*);
    void remove() override;
};