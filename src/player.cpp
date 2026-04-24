#include "player.hpp"
#include "direction.hpp"
#include "portal.hpp"
#include "chest.hpp"
#include "bullet.hpp"
#include "mine.hpp"
#include <memory>

const Inventory INITIAL_INVENTORY {
    INITIAL_CLAY,
    INITIAL_BULLETS,
    INITIAL_MEAT
};

Player::Player(sista::Coordinates coordinates) : Entity('$', coordinates, playerStyle, Type::PLAYER), mode(Player::Mode::COLLECT), inventory(INITIAL_INVENTORY) {}
Player::Player() : Entity('$', {0, 0}, playerStyle, Type::PLAYER), mode(Player::Mode::COLLECT), inventory(INITIAL_INVENTORY) {}
void Player::remove() {
    [[maybe_unused]] std::shared_ptr<Player> keepAlive;
    if (Player::player.get() == this) {
        keepAlive = Player::player;
    }
    field->erasePawn(this);
    if (Player::player.get() == this) {
        Player::player.reset();
    }
}
void Player::move(Direction direction) {
    sista::Coordinates next = this->coordinates + directionMap[direction];
    if (field->isFree(next)) {
        field->movePawn(this, next);
    } else if (field->isOutOfBounds(next)) {
        return;
    } else if (field->isOccupied(next)) {
        Entity* entity = (Entity*)field->getPawn(next);
        switch (entity->type) {
            case Type::PORTAL: {
                Portal* portal = (Portal*)entity;
                if (auto exitPtr = portal->exit.lock()) {
                    sista::Coordinates landing = exitPtr->getCoordinates() + directionMap[direction];
                    if (field->isFree(landing)) {
                        field->movePawn(this, landing);
                    }
                }
                break;
            }
            default:
                break;
        }
    }
}
void Player::shoot(Direction direction) {
    sista::Coordinates target = this->coordinates + directionMap[direction];
    if (!field->isFree(target)) {
        if (field->isOutOfBounds(target)) return;
        
        Entity* entity = (Entity*)field->getPawn(target);
        switch (this->mode) {
            case Mode::COLLECT: {
                if (entity->type == Type::CHEST) {
                    Chest* chest = (Chest*)entity;
                    this->inventory += chest->inventory;
                    chest->remove();
                }
                break;
            }
            case Mode::DUMPCHEST: {
                if (entity->type == Type::CHEST) {
                    Chest* chest = (Chest*)entity;
                    chest->inventory += {
                        this->inventory.clay,
                        this->inventory.bullets,
                        0 // Meat cannot be deposited
                    };
                    this->inventory = {0, 0, this->inventory.meat};
                }
                break;
            }
            default:
                return;
        }
        return;
    }
    
    switch (this->mode) {
        case Mode::BULLET:
            if (--inventory.bullets >= 0) {
                {
                    auto b = std::make_shared<Bullet>(target, direction);
                    Bullet::bullets.push_back(b);
                    field->addPrintPawn(b);
                }
            }
            inventory.bullets = std::max(inventory.bullets, (short)0);
            break;
        case Mode::DUMPCHEST:
            if (inventory.clay > 0 || inventory.bullets > 0) {
                {
                    // Meat cannot be deposited, so we set it to 0 in the chest and leave the meat to the player
                    auto c = std::make_shared<Chest>(target, Inventory{this->inventory.clay, this->inventory.bullets, 0});
                    Chest::chests.push_back(c);
                    field->addPrintPawn(c);
                }
                this->inventory = {0, 0, this->inventory.meat};
            }
            break;
        case Mode::MINE:
            if (this->inventory.containsAtLeast(COST_OF_MINE)) {
                this->inventory -= COST_OF_MINE;
                {
                    auto m = std::make_shared<Mine>(target);
                    Mine::mines.push_back(m);
                    field->addPrintPawn(m);
                }
            }
            break;
        default:
            return;
    }
}
sista::ANSISettings Player::playerStyle = {
    sista::ForegroundColor::RED,
    sista::BackgroundColor::BLACK,
    sista::Attribute::BRIGHT
};