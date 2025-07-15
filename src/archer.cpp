#include "archer.hpp"
#include "constants.hpp"
#include "worm.hpp"
#include "direction.hpp"
#include "chest.hpp"
#include "mine.hpp"
#include "player.hpp"
#include "wall.hpp"
#include "enemyBullet.hpp"
#include <unordered_map>
#include <map>
#include <set>

extern std::unordered_map<Direction, char> directionSymbol;
extern std::unordered_map<Direction, sista::Coordinates> directionMap;
extern std::map<int, std::vector<int>> passages; // {y, {x1, x2, x3...}}
extern std::map<int, std::vector<int>> breaches; // Central breaches, "holes"
extern sista::SwappableField* field;
extern std::mt19937 rng;
extern std::bernoulli_distribution dumbMoveDistribution;
extern bool dead;
enum EndReason {STARVED, SHOT, EATEN, STABBED, TOUCHDOWN, QUIT};
void printEndInformation(EndReason);

Archer::Archer(sista::Coordinates coordinates) : Entity('A', coordinates, archerStyle, Type::ARCHER) {
    Archer::archers.push_back(this);
}
void Archer::move() {
    // There is always a probability of a dumb move
    if (dumbMoveDistribution(rng)) {
        Direction direction = (Direction)(rand() % 4);
        this->move(direction);
        return;
    }

    auto [row, column] = coordinates;
    sista::Coordinates above = coordinates + directionMap[Direction::UP];
    // It could be right below a passage or breach
    if (row % (TUNNEL_UNIT * 3) == 0) {
        if (std::find(breaches[above.y].begin(), breaches[above.y].end(), column) != breaches[above.y].end()) {
            // There is a breach right above, so try to step in
            this->move(Direction::UP);
            return;
        } else if (std::find(passages[above.y].begin(), passages[above.y].end(), column) != passages[above.y].end()) {
            // There is a passage, so try to ascend
            this->move(Direction::UP);
            return;
        }
    }
    // It is in a passage or breach (in the proper range on y axis)
    if (row % (TUNNEL_UNIT * 3) >= TUNNEL_UNIT * 2) {
        // it uses BFS to pick the next move to get to the other side
        std::queue<std::pair<sista::Coordinates, Direction>> bfs({
            {coordinates + directionMap[Direction::UP], Direction::UP},
            {coordinates + directionMap[Direction::LEFT], Direction::LEFT},
            {coordinates + directionMap[Direction::DOWN], Direction::DOWN},
            {coordinates + directionMap[Direction::RIGHT], Direction::RIGHT}
        }); // {coordinate, first direction taken in the path}
        std::set<sista::Coordinates> visited;
        visited.insert({coordinates});
        #if DEBUG
        std::cerr << "Archer::move() - Starting BFS from {" << coordinates.y << "," << coordinates.x << "}\n";
        #endif
        Direction chosenMove;
        bool found = false;
        while (!bfs.empty()) {
            auto [coords, choice] = bfs.front();
            bfs.pop();
            #if DEBUG
            std::cerr << "\tArcher::move() - BFS coords: {" << coords.y << "," << coords.x << "}, choice: " << directionSymbol[choice] << "\n";
            #endif

            if (field->isOutOfBounds(coords)) continue; // Exiting the field
            if (std::find(visited.begin(), visited.end(), coords) != visited.end()) continue; // Already visited
            if (field->isOccupied(coords)) { // Cell is not free
                Type type = ((Entity*)field->getPawn(coords))->type;
                if (type == Type::WALL || type == Type::PORTAL) {
                    #if DEBUG
                    std::cerr << "\tArcher::move() - Cell occupied by " << type << ", skipping\n";
                    #endif
                    continue;
                }
            }
            if (coords.y % (TUNNEL_UNIT * 3) == 0) continue; // Exiting the breach
            if (coords.y % (TUNNEL_UNIT * 3) < TUNNEL_UNIT * 2) { // Exiting the breach on the upper side
                chosenMove = choice;
                found = true;
                break;
            }
            visited.insert(coords);

            bfs.push({coords + directionMap[Direction::UP], choice});
            bfs.push({coords + directionMap[Direction::LEFT], choice});
            bfs.push({coords + directionMap[Direction::DOWN], choice});
            bfs.push({coords + directionMap[Direction::RIGHT], choice});
        }
        if (found) {
            this->move(chosenMove);
            return;
        } else {
            Direction direction = (Direction)(rand() % 4);
            this->move(direction);
            return;
        }
    }
    // It roughly (probabilistically by x/y ratio) points to the centermost (or a random-but-deterministic one) breach
    int next_passage_y = (row - (row % (TUNNEL_UNIT * 3))) - 1;
    int next_passage_x;
    if (!breaches[next_passage_y].empty()) {
        std::vector<int> centermost_breaches = breaches[next_passage_y];
        if (!centermost_breaches.empty()) {
            std::sort(centermost_breaches.begin(), centermost_breaches.end(), [](int a, int b) {
                return std::abs(a - WIDTH/2) < std::abs(b - WIDTH/2);
            });
            next_passage_x = centermost_breaches[(reinterpret_cast<intptr_t>(this)) % centermost_breaches.size()];
        }
    } else {
        // If no breaches are found, then it points to any passage of the following level (random and not necessarily deterministic)
        std::vector<int> available_passages = passages[next_passage_y];
        if (!available_passages.empty()) {
            next_passage_x = available_passages[rand() % available_passages.size()];
        } else {
            // If there is no passage on the next, it means we're pointing to a negative coordinate
            // It is dangerous as generating coordinates from that could lead to segmentation faults
            // but since we only use it to decide the next move it's actually innocuous
            next_passage_x = WIDTH - 1; // Point to the top right
        }
    }
    int delta_y = next_passage_y - row;
    int delta_x = next_passage_x - column;
    float ratio = (std::min(std::abs(delta_y), std::abs(delta_x)) + 1)
                / (std::max(std::abs(delta_y), std::abs(delta_x)) + 1);
    std::bernoulli_distribution verticalNotHorizontalDistribution(ratio);
    if (verticalNotHorizontalDistribution(rng)) {
        this->move(Direction::UP);
    } else {
        if (delta_x >= 0) {
            this->move(Direction::RIGHT);
        } else {
            this->move(Direction::LEFT);
        }
    }
}
bool Archer::move(Direction direction) {
    sista::Coordinates next = this->coordinates + directionMap[direction];
    if (field->isOutOfBounds(next)) {
        return false;
    } else if (field->isFree(next)) {
        field->movePawn(this, next);
        return true;
    } else if (field->isOccupied(next)) {
        Entity* entity = (Entity*)field->getPawn(next);
        switch (entity->type) {
            case Type::MINE:
                ((Mine*)entity)->trigger();
            default:
                return false;
        }
    }
    return false;
}
void Archer::shoot() {
    if (dumbMoveDistribution(rng)) {
        this->shoot((Direction)(rand() % 4));
        return;
    }
    if (coordinates.x == Player::player->getCoordinates().x) {
        // Roughly vertically aligned with the player
        if (coordinates.y / (TUNNEL_UNIT * 3) != Player::player->getCoordinates().y / (TUNNEL_UNIT * 3)) {
            // They was a wall between them so the archer cannot see
            int next_breaches_y = (coordinates.y % (TUNNEL_UNIT * 3)) - 1;
            if (!breaches[next_breaches_y].empty()) {
                if (std::find(breaches[next_breaches_y].begin(), breaches[next_breaches_y].end(), coordinates.x) != breaches[next_breaches_y].end()) {
                    // ...unless there is a breach right above that allows the archer to hear the player moving behind the wall
                    this->shoot(Direction::UP);
                    return;
                }
            }
            this->shoot((Direction)(rand() % 4));
            return;
        }
        if (coordinates.y < Player::player->getCoordinates().y) {
            this->shoot(Direction::DOWN);
        } else if (coordinates.y > Player::player->getCoordinates().y) {
            this->shoot(Direction::UP);
        }
        // Exactly vertically aligned with the player
        if (coordinates.x == Player::player->getCoordinates().x) {
            this->move(Direction::DOWN); // Dodges incoming bullets
        }
    } else if (coordinates.y == Player::player->getCoordinates().y) {
        // Roughly horizontally aligned with the player
        if (coordinates.x < Player::player->getCoordinates().x) {
            this->shoot(Direction::RIGHT);
        } else if (coordinates.x > Player::player->getCoordinates().x) {
            this->shoot(Direction::LEFT);
        }
        // Exactly horizontally aligned with the player
        if (coordinates.y == Player::player->getCoordinates().y) {
            this->move(Direction::LEFT); // Dodges incoming bullets
        }
    } else {
        this->shoot((Direction)(rand() % 4));
    }
}
bool Archer::shoot(Direction direction) {
    sista::Coordinates target = this->coordinates + directionMap[direction];
    if (field->isOutOfBounds(target)) {
        return false;
    } else if (field->isFree(target)) {
        field->addPrintPawn(new EnemyBullet(target, direction));
        return true;
    } else if (field->isOccupied(target)) {
        Entity* entity = (Entity*)field->getPawn(target);
        switch (entity->type) {
            case Type::WALL:
                ((Wall*)entity)->getHit();
                break;
            case Type::PLAYER: // Counts as a dagger hit
                printEndInformation(EndReason::STABBED);
                dead = true;
                break;
            case Type::MINE:
                ((Mine*)entity)->trigger();
                break;
            default:
                break;
        }
        return false;
    }
    return false;
}
void Archer::die() {
    Archer::archers.erase(std::find(Archer::archers.begin(), Archer::archers.end(), this));
    field->erasePawn(this);
    field->addPrintPawn(new Chest(coordinates, {
        LOOT_ARCHER_CLAY,
        LOOT_ARCHER_BULLETS,
        LOOT_ARCHER_MEAT
    }));
    delete this;
}
void Archer::remove() {
    Archer::archers.erase(std::find(Archer::archers.begin(), Archer::archers.end(), this));
    field->erasePawn(this);
    delete this;
}
std::bernoulli_distribution Archer::moving(ARCHER_MOVING_PROBABILITY);
std::bernoulli_distribution Archer::shooting(ARCHER_SHOOTING_PROBABILITY);
std::bernoulli_distribution Archer::spawning(ARCHER_SPAWNING_PROBABILITY);
ANSI::Settings Archer::archerStyle = {
    ANSI::ForegroundColor::F_CYAN,
    ANSI::BackgroundColor::B_BLACK,
    ANSI::Attribute::STRIKETHROUGH
};