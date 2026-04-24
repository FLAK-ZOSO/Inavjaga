#include "entity.hpp"

Entity::Entity(char symbol, sista::Coordinates coordinates, sista::ANSISettings& settings, Type type) :
    sista::Pawn(symbol, coordinates, settings), type(type) {}