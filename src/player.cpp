#include "player.hpp"
#include "direction.hpp"
#include "portal.hpp"
#include "chest.hpp"
#include "bullet.hpp"
#include "mine.hpp"

const Inventory INITIAL_INVENTORY {
    INITIAL_CLAY,
    INITIAL_BULLETS,
    INITIAL_MEAT
};

Player::Player(sista::Coordinates coordinates) : Entity('$', coordinates, playerStyle, Type::PLAYER), mode(Player::Mode::COLLECT), inventory(INITIAL_INVENTORY) {}
Player::Player() : Entity('$', {0, 0}, playerStyle, Type::PLAYER), mode(Player::Mode::COLLECT), inventory(INITIAL_INVENTORY) {}
void Player::remove() {
    field->erasePawn(this);
    delete this;
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
                sista::Coordinates landing = portal->exit->getCoordinates() + directionMap[direction];
                if (field->isFree(landing)) {
                    field->movePawn(this, landing);
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
                field->addPrintPawn(new Bullet(target, direction));
            }
            inventory.bullets = std::max(inventory.bullets, (short)0);
            break;
        case Mode::DUMPCHEST:
            if (inventory.clay > 0 || inventory.bullets > 0) {
                field->addPrintPawn(new Chest(target, {
                    this->inventory.clay,
                    this->inventory.bullets,
                    0 // Meat cannot be deposited
                }));
                this->inventory = {0, 0, this->inventory.meat};
            }
            break;
        case Mode::MINE:
            if (this->inventory.containsAtLeast(COST_OF_MINE)) {
                this->inventory -= COST_OF_MINE;
                field->addPrintPawn(new Mine(target));
            }
            break;
        default:
            return;
    }
}
ANSI::Settings Player::playerStyle = {
    ANSI::ForegroundColor::F_RED,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::BRIGHT
};