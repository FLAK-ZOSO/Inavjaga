#include "worm.hpp"
#include "constants.hpp"
#include "direction.hpp"
#include "chest.hpp"
#include "mine.hpp"
#include "wall.hpp"
#include <unordered_map>
#include <memory>

extern std::unordered_map<Direction, char> directionSymbol;
extern std::unordered_map<Direction, sista::Coordinates> directionMap;
extern std::shared_ptr<sista::SwappableField> field;
extern std::mt19937 rng;
extern std::bernoulli_distribution dumbMoveDistribution;
extern bool dead;
enum EndReason {STARVED, SHOT, EATEN, STABBED, TOUCHDOWN, QUIT};
void printEndInformation(EndReason);

WormBody::WormBody(sista::Coordinates coordinates, Direction direction) : Entity(directionSymbol[direction], coordinates, wormBodyStyle, Type::WORM_BODY) {
    // ownership moved to creator via std::shared_ptr; do not push here
}
void WormBody::die() {
    sista::Coordinates drop = this->coordinates;
    #if DEBUG
    std::cerr << "WormBody::die() called for " << this << " at {" << drop.y << ", " << drop.x << "}" << std::endl;
    #endif
    // Free the pawn's coordinates first so we can place a chest there
    field->erasePawn(this);
    // create chest and keep ownership in Chest::chests to ensure it stays alive
    {
        auto c = std::make_shared<Chest>(drop, Inventory{0,1,0});
        Chest::chests.push_back(c);
        field->addPrintPawn(c);
    }
    auto wormBodyIt = std::find_if(WormBody::wormBodies.begin(), WormBody::wormBodies.end(), [this](const std::shared_ptr<WormBody>& p){ return p.get() == this; });
    if (wormBodyIt != WormBody::wormBodies.end()) WormBody::wormBodies.erase(wormBodyIt);
    auto head = this->head.lock();
    if (head) {
        auto headBodyIt = std::find_if(head->body.begin(), head->body.end(), [this](const std::shared_ptr<WormBody>& p){ return p.get() == this; });
        if (headBodyIt != head->body.end()) head->body.erase(headBodyIt);
    }
    // destruction handled by shared_ptr owners
}
void WormBody::remove() {
    [[maybe_unused]] auto keepAlive = Entity::keepAliveFrom(WormBody::wormBodies, this);
    field->erasePawn(this);
    Entity::removeOwner(WormBody::wormBodies, this);
}
sista::ANSISettings WormBody::wormBodyStyle = {
    sista::RGBColor(50, 0xff, 150),
    sista::BackgroundColor::BLACK,
    sista::Attribute::BRIGHT
};

Worm::Worm(sista::Coordinates coordinates) : Entity('H', coordinates, wormHeadStyle, Type::WORM_HEAD), hp(WORM_HEALTH_POINTS), collided(false) {
    direction = (Direction)(rand() % 4);
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
                    this->takeHit();
                    return;
                }
            } else if (direction == toTheRight) {
                direction = toTheLeft;
                next = coordinates + directionMap[direction];
                if (field->isOutOfBounds(next)) {
                    direction = oldDirection;
                    this->takeHit();
                    return;
                }
            }
        }
    }
    if (field->isFree(next)) {
        /* Movement inspired from Dune (https://github.com/Lioydiano/Dune/blob/90a1f9c412258f701e3dfe949b05c6bcaa171e9f/dune.cpp#L386) */
        field->movePawn(this, next);
        // We now create a piece of body to leave behind the head, a "neck"
        auto neck = std::make_shared<WormBody>(oldHeadCoordinates, direction);
        neck->head = this->weak_from_this();
        WormBody::wormBodies.push_back(neck);
        field->addPrintPawn(neck);
        body.push_back(neck);
        // Consider that we added a body piece, so we need to ensure it does not grow too much
        if (body.size() > WORM_LENGTH) {
            auto tail_ptr = body.front();
            WormBody* tail = tail_ptr.get();
            sista::Coordinates drop = tail->getCoordinates();
            #if DEBUG
            std::cerr << "In Worm::move() deleting the tail piece " << this << " at {" << drop.y << ", " << drop.x << "}" << std::endl;
            #endif
            field->erasePawn(tail);
            if (clayRelease(rng)) {
                auto c = std::make_shared<Chest>(drop, Inventory{1,0,0});
                Chest::chests.push_back(c);
                field->addPrintPawn(c);
            }
            body.erase(body.begin());
            auto itwb = std::find_if(WormBody::wormBodies.begin(), WormBody::wormBodies.end(), [tail](const std::shared_ptr<WormBody>& p){ return p.get() == tail; });
            if (itwb != WormBody::wormBodies.end()) WormBody::wormBodies.erase(itwb);
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
                    ((Wall*)entity)->takeHit(); // They can weaken a wall but not destroy it
                this->turn(options[rand() % 2]);
                break;
            case Type::WORM_HEAD:
                if (((Worm*)entity)->hp <= 1) {
                    ((Worm*)entity)->collided = true;
                }
                ((Worm*)entity)->takeHit();
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
void Worm::takeHit() {
    if (--hp <= 0) {
        if (collided) return;
        this->die();
    }
}
void Worm::die() {
    // Save coordinates early and keep self alive while mutating Worm::worms.
    sista::Coordinates drop = coordinates;
    while (!body.empty()) {
        auto tail_ptr = body.front();
        WormBody* tail = tail_ptr.get();
        tail->die(); // removes itself from head->body and wormBodies
    }
    auto keepAlive = Entity::keepAliveFrom(Worm::worms, this);
    field->erasePawn(this);
    {
        auto c = std::make_shared<Chest>(
            drop, Inventory{
                LOOT_WORM_HEAD_CLAY,
                LOOT_WORM_HEAD_BULLETS,
                LOOT_WORM_HEAD_MEAT
            }
        );
        Chest::chests.push_back(c);
        field->addPrintPawn(c);
    }
    Entity::removeOwner(Worm::worms, this);
}
void Worm::remove() {
    [[maybe_unused]] auto keepAlive = Entity::keepAliveFrom(Worm::worms, this);
    field->erasePawn(this);
    Entity::removeOwner(Worm::worms, this);
}
Direction Worm::options[2] = {Direction::LEFT, Direction::RIGHT};
std::bernoulli_distribution Worm::turning(WORM_TURNING_PROBABILITY);
std::bernoulli_distribution Worm::moving(WORM_MOVING_PROBABILITY);
std::bernoulli_distribution Worm::spawning(WORM_SPAWNING_PROBABILITY);
std::bernoulli_distribution Worm::eatingTail(WORM_EATING_TAIL_PROBABILITY);
std::bernoulli_distribution Worm::eatingArcher(WORM_EATING_ARCHER_PROBABILITY);
std::bernoulli_distribution Worm::clayRelease(CLAY_RELEASE_PROBABILITY);
sista::ANSISettings Worm::wormHeadStyle = {
    sista::ForegroundColor::GREEN,
    sista::BackgroundColor::BLACK,
    sista::Attribute::RAPID_BLINK
};
