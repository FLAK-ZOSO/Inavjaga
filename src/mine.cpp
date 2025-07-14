#include "mine.hpp"
#include "wall.hpp"
#include "archer.hpp"
#include "worm.hpp"
#include "constants.hpp"

extern sista::SwappableField* field;
extern std::mt19937 rng;

Mine::Mine(sista::Coordinates coordinates) : Entity('*', coordinates, mineStyle, Type::MINE), triggered(false) {
    Mine::mines.push_back(this);
}
void Mine::remove() {
    Mine::mines.erase(std::find(Mine::mines.begin(), Mine::mines.end(), this));
    field->erasePawn(this);
    delete this;
}
void Mine::trigger() {
    if (this->triggered) return;
    this->triggered = true;
    this->setSettings(Mine::triggeredMineStyle);
    this->setSymbol('%');
    field->rePrintPawn(this);
}
void Mine::explode() {
    for (int j=-MINE_DAMAGE_RADIUS; j<=MINE_DAMAGE_RADIUS; j++) {
        for (int i=-MINE_DAMAGE_RADIUS; i<=MINE_DAMAGE_RADIUS; i++) {
            if (i == 0 && j == 0) continue;
            sista::Coordinates target = this->coordinates + sista::Coordinates(j, i);
            if (field->isOutOfBounds(target)) continue;
            if (field->isOccupied(target)) {
                Entity* entity = (Entity*)field->getPawn(target);
                switch (entity->type) {
                    case Type::MINE:
                        ((Mine*)entity)->trigger();
                        break;
                    case Type::WALL: {
                        int damage = mineDamage(rng);
                        while (!((Wall*)entity)->getHit() && --damage);
                        break;
                    }
                    case Type::PLAYER: case Type::PORTAL:
                        break; // Player and Portals get no damage from mines
                    case Type::ARCHER:
                        ((Archer*)entity)->die();
                        break;
                    case Type::WORM_BODY:
                        ((WormBody*)entity)->die();
                        break;
                    case Type::WORM_HEAD:
                        ((Worm*)entity)->getHit();
                        break;
                    default:
                        entity->remove();
                }
            }
        }
    }
    this->remove();
}
std::bernoulli_distribution Mine::explosion(MINE_EXPLOSION_IN_FRAME_PROBABILITY);
std::uniform_int_distribution<int> Mine::mineDamage(MINE_MINIMUM_DAMAGE, MINE_MAXIMUM_DAMAGE);
ANSI::Settings Mine::mineStyle = {
    ANSI::RGBColor(200, 100, 200),
    RGB_ROCKS_FOREGROUND,
    ANSI::Attribute::BRIGHT
};
ANSI::Settings Mine::triggeredMineStyle = {
    RGB_BLACK,
    ANSI::RGBColor(0xff, 0, 0),
    ANSI::Attribute::BLINK
};