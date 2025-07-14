#include "worm.hpp"
#include "constants.hpp"
#include "direction.hpp"
#include "chest.hpp"
#include "mine.hpp"
#include "wall.hpp"
#include <unordered_map>

extern std::unordered_map<Direction, char> directionSymbol;
extern std::unordered_map<Direction, sista::Coordinates> directionMap;
extern sista::SwappableField* field;
extern std::mt19937 rng;
extern std::bernoulli_distribution dumbMoveDistribution;
extern bool dead;
enum EndReason {STARVED, SHOT, EATEN, STABBED, TOUCHDOWN, QUIT};
void printEndInformation(EndReason);

WormBody::WormBody(sista::Coordinates coordinates, Direction direction) : Entity(directionSymbol[direction], coordinates, wormBodyStyle, Type::WORM_BODY) {
    WormBody::wormBodies.push_back(this);
}
void WormBody::die() {
    sista::Coordinates drop = this->coordinates;
    #if DEBUG
    std::cerr << "WormBody::die() called for " << this << " at {" << drop.y << ", " << drop.x << "}" << std::endl;
    #endif
    field->addPrintPawn(new Chest(drop, {0, 1, 0}));
    WormBody::wormBodies.erase(std::find(WormBody::wormBodies.begin(), WormBody::wormBodies.end(), this));
    this->head->body.erase(std::find(this->head->body.begin(), this->head->body.end(), this));
    delete this;
}
void WormBody::remove() {
    WormBody::wormBodies.erase(std::find(WormBody::wormBodies.begin(), WormBody::wormBodies.end(), this));
    field->erasePawn(this);
    delete this;
}
ANSI::Settings WormBody::wormBodyStyle = {
    ANSI::RGBColor(50, 0xff, 150),
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};

Worm::Worm(sista::Coordinates coordinates) : Entity('H', coordinates, wormHeadStyle, Type::WORM_HEAD), hp(WORM_HEALTH_POINTS), collided(false) {
    direction = (Direction)(rand() % 4);
    Worm::worms.push_back(this);
}
Worm::Worm(sista::Coordinates coordinates, Direction direction) : Worm(coordinates) {
    this->direction = direction;
}
void Worm::move() {
    sista::Coordinates oldHeadCoordinates = coordinates;
    sista::Coordinates next = coordinates + directionMap[direction];
    if (field->isOutOfBounds(next)) {
        Direction toTheLeft = (Direction)((direction + 3) % 4);
        Direction toTheRight = (Direction)((direction + 1) % 4);
        Direction turningOptions[] = {toTheLeft, toTheRight};
        Direction oldDirection = direction;
        direction = turningOptions[rand() % 2];
        next = coordinates + directionMap[direction];
        if (field->isOutOfBounds(next)) {
            if (direction == toTheLeft) {
                direction = toTheRight;
                next = coordinates + directionMap[direction];
                if (field->isOutOfBounds(next)) {
                    direction = oldDirection;
                    this->getHit();
                    return;
                }
            } else if (direction == toTheRight) {
                direction = toTheLeft;
                next = coordinates + directionMap[direction];
                if (field->isOutOfBounds(next)) {
                    direction = oldDirection;
                    this->getHit();
                    return;
                }
            }
        }
    }
    if (field->isFree(next)) {
        /* Movement inspired from Dune (https://github.com/Lioydiano/Dune/blob/90a1f9c412258f701e3dfe949b05c6bcaa171e9f/dune.cpp#L386) */
        field->movePawn(this, next);
        // We now create a piece of body to leave behind the head, a "neck"
        WormBody* neck = new WormBody(oldHeadCoordinates, direction);
        neck->head = this;
        field->addPrintPawn(neck);
        body.push_back(neck);
        // Consider that we added a body piece, so we need to ensure it does not grow too much
        if (body.size() > WORM_LENGTH) {
            WormBody* tail = body.front();
            sista::Coordinates drop = tail->getCoordinates();
            #if DEBUG
            std::cerr << "In Worm::move() deleting the tail piece " << this << " at {" << drop.y << ", " << drop.x << "}" << std::endl;
            #endif
            field->erasePawn(tail);
            if (clayRelease(rng)) {
                field->addPrintPawn(new Chest(drop, {1, 0, 0}));
            }
            body.erase(body.begin());
            WormBody::wormBodies.erase(std::find(WormBody::wormBodies.begin(), WormBody::wormBodies.end(), tail));
            delete tail;
        }
    } else if (field->isOccupied(next)) {
        Entity* entity = (Entity*)field->getPawn(next);
        switch (entity->type) {
            case Type::PLAYER:
                this->turn(options[rand() % 2]);
                printEndInformation(EndReason::EATEN);
                dead = true;
                break;
            case Type::WALL:
                if (((Wall*)entity)->strength > 1)
                    ((Wall*)entity)->getHit(); // They can weaken a wall but not destroy it
                this->turn(options[rand() % 2]);
                break;
            case Type::WORM_HEAD:
                if (((Worm*)entity)->hp <= 1) {
                    ((Worm*)entity)->collided = true;
                }
                ((Worm*)entity)->getHit();
            case Type::PORTAL:
                this->turn(options[rand() % 2]);
                break;
            case Type::WORM_BODY:
                if (!eatingTail(rng)) {
                    this->turn(options[rand() % 2]);
                    break;
                }
                ((WormBody*)entity)->die();
                break;
            case Type::ARCHER:
                if (!eatingArcher(rng)) {
                    this->turn(options[rand() % 2]);
                    break;
                }
                break;
            case Type::MINE:
                ((Mine*)entity)->trigger();
                this->turn(options[rand() % 2]);
                break;
            default:
                entity->remove();
        }
    }
}
void Worm::turn() {
    if (dumbMoveDistribution(rng)) {
        this->turn(options[rand() % 2]);
        return;
    }
    // TODO: proper turning intelligence
    this->turn(options[rand() % 2]);
}
void Worm::turn(Direction direction_) {
    if (direction_ == Direction::LEFT)
        this->direction = (Direction)((this->direction + 3) % 4);
    else if (direction_ == Direction::RIGHT)
        this->direction = (Direction)((this->direction + 1) % 4);
}
void Worm::getHit() {
    if (--hp <= 0) {
        if (collided) return;
        this->die();
    }
}
void Worm::die() {
    while (!body.empty()) {
        WormBody* tail = body.front();
        tail->die(); // Self deletes from the body too
    }
    Worm::worms.erase(std::find(Worm::worms.begin(), Worm::worms.end(), this));
    field->erasePawn(this);
    field->addPrintPawn(new Chest(coordinates, {
        LOOT_WORM_HEAD_CLAY,
        LOOT_WORM_HEAD_BULLETS,
        LOOT_WORM_HEAD_MEAT
    }));
    delete this;
}
void Worm::remove() {
    Worm::worms.erase(std::find(Worm::worms.begin(), Worm::worms.end(), this));
    field->erasePawn(this);
    delete this;
}
Direction Worm::options[2] = {Direction::LEFT, Direction::RIGHT};
std::bernoulli_distribution Worm::turning(WORM_TURNING_PROBABILITY);
std::bernoulli_distribution Worm::moving(WORM_MOVING_PROBABILITY);
std::bernoulli_distribution Worm::spawning(WORM_SPAWNING_PROBABILITY);
std::bernoulli_distribution Worm::eatingTail(WORM_EATING_TAIL_PROBABILITY);
std::bernoulli_distribution Worm::eatingArcher(WORM_EATING_ARCHER_PROBABILITY);
std::bernoulli_distribution Worm::clayRelease(CLAY_RELEASE_PROBABILITY);
ANSI::Settings Worm::wormHeadStyle = {
    ANSI::ForegroundColor::F_GREEN,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::RAPID_BLINK
};
