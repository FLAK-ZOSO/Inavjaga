#include "enemyBullet.hpp"
#include "chest.hpp"
#include "mine.hpp"
#include "wall.hpp"
#include "worm.hpp"
#include "bullet.hpp"
#include <unordered_map>

extern std::unordered_map<Direction, char> directionSymbol;
extern std::unordered_map<Direction, sista::Coordinates> directionMap;
extern sista::SwappableField* field;
extern bool dead;
enum EndReason {STARVED, SHOT, EATEN, STABBED, TOUCHDOWN, QUIT};
void printEndInformation(EndReason);

EnemyBullet::EnemyBullet(sista::Coordinates coordinates, Direction direction) :
    Entity(directionSymbol[direction], coordinates, enemyBulletStyle, Type::ENEMY_BULLET), direction(direction), collided(false) {
    EnemyBullet::enemyBullets.push_back(this);
}
void EnemyBullet::remove() {
    EnemyBullet::enemyBullets.erase(std::find(EnemyBullet::enemyBullets.begin(), EnemyBullet::enemyBullets.end(), this));
    field->erasePawn(this);
    delete this;
}
void EnemyBullet::move() {
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
                ((Wall*)entity)->getHit();
                break;
            case Type::MINE:
                ((Mine*)entity)->trigger();
                break;
            case Type::BULLET: {
                Bullet* bullet = (Bullet*)entity;
                if (bullet->collided) return;
                bullet->collided = true;
                break;
            }
            case Type::ENEMY_BULLET: {
                EnemyBullet* bullet = (EnemyBullet*)entity;
                if (bullet->collided) return;
                bullet->collided = true;
                break;
            }
            case Type::WORM_HEAD: {
                Worm* worm = (Worm*)entity;
                worm->getHit();
                break;
            }
            case Type::WORM_BODY: {
                WormBody* wormBody = (WormBody*)entity;
                wormBody->die();
                break;
            }
            case Type::PLAYER:
                printEndInformation(EndReason::SHOT);
                dead = true;
                break;
            default:
                break;
        }
        this->remove(); // Hit something and the situation was not handled
    }
}
ANSI::Settings EnemyBullet::enemyBulletStyle = {
    ANSI::ForegroundColor::F_GREEN,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};