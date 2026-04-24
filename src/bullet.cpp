#include "bullet.hpp"
#include "wall.hpp"
#include "mine.hpp"
#include "archer.hpp"
#include "enemyBullet.hpp"
#include "worm.hpp"
#include <unordered_map>

extern std::unordered_map<Direction, char> directionSymbol;
extern std::unordered_map<Direction, sista::Coordinates> directionMap;
extern std::shared_ptr<sista::SwappableField> field;

Bullet::Bullet(sista::Coordinates coordinates, Direction direction) :
    Entity(directionSymbol[direction], coordinates, bulletStyle, Type::BULLET), direction(direction), collided(false) {
    // ownership moved to creator via std::shared_ptr; do not push here
}
void Bullet::remove() {
    [[maybe_unused]] auto keepAlive = Entity::keepAliveFrom(Bullet::bullets, this);
    field->erasePawn(this);
    Entity::removeOwner(Bullet::bullets, this);
}
void Bullet::move() {
    sista::Coordinates next = this->coordinates + directionMap[direction];
    if (field->isFree(next)) {
        field->movePawn(this, next);
    } else if (field->isOutOfBounds(next)) {
        this->remove();
    } else if (field->isOccupied(next)) {
        Entity* entity = (Entity*)field->getPawn(next);
        Type entityType = entity->type;
        switch (entityType) {
            case Type::WALL:
                ((Wall*)entity)->takeHit();
                break;
            case Type::MINE:
                ((Mine*)entity)->trigger();
                break;
            case Type::ENEMY_BULLET: {
                EnemyBullet* bullet = (EnemyBullet*)entity;
                if (bullet->collided) return;
                bullet->collided = true;
                break;
            }
            case Type::BULLET: {
                Bullet* bullet = (Bullet*)entity;
                if (bullet->collided) return;
                bullet->collided = true;
                break;
            }
            case Type::ARCHER: {
                ((Archer*)entity)->die();
                break;
            }
            case Type::WORM_HEAD: {
                Worm* worm = (Worm*)entity;
                worm->takeHit();
                break;
            }
            case Type::WORM_BODY: {
                WormBody* wormBody = (WormBody*)entity;
                wormBody->die();
                break;
            }
            default:
                break;
        }
        this->remove(); // Hit something and the situation was not handled
    }
}
sista::ANSISettings Bullet::bulletStyle = {
    sista::ForegroundColor::MAGENTA,
    sista::BackgroundColor::BLACK,
    sista::Attribute::BRIGHT
};