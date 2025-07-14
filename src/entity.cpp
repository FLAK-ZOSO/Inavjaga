#include "entity.hpp"

Entity::Entity(char symbol, sista::Coordinates coordinates, ANSI::Settings& settings, Type type) :
    sista::Pawn(symbol, coordinates, settings), type(type) {}