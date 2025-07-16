#include "wall.hpp"
#include "constants.hpp"
#include "direction.hpp"
#include <stack>
#include <set>
#include <unordered_map>
#include <map>

extern std::unordered_map<Direction, sista::Coordinates> directionMap;
extern std::map<int, std::vector<int>> breaches; // Central breaches, "holes"
extern sista::SwappableField* field;

Wall::Wall(sista::Coordinates coordinates, short int strength) :
    Entity('#', coordinates, wallStyle, Type::WALL), strength(strength) {
    Wall::walls.push_back(this);
}
void Wall::remove() {
    Wall::walls.erase(std::find(Wall::walls.begin(), Wall::walls.end(), this));
    field->erasePawn(this);
    delete this;
}
bool Wall::getHit() {
    if (--strength <= 0) {
        // Verify if this creates a new breach by a DFS
        std::stack<sista::Coordinates> dfs({
            coordinates + directionMap[Direction::UP],
            coordinates + directionMap[Direction::LEFT],
            coordinates + directionMap[Direction::DOWN],
            coordinates + directionMap[Direction::RIGHT]
        });
        std::set<sista::Coordinates> visited;
        visited.insert({coordinates});
        #if DEBUG
        std::cerr << "Wall::getHit() - Starting DFS from {" << coordinates.y << "," << coordinates.x << "}\n";
        #endif
        sista::Coordinates coords, breach;
        bool foundBelowExit = false;
        bool foundAboveExit = false;
        while (!dfs.empty() && !foundBelowExit && !foundAboveExit) {
            coords = dfs.top();
            dfs.pop();
            #if DEBUG
            std::cerr << "\tWall::getHit() - DFS coords: {" << coords.y << "," << coords.x << "}\n";
            #endif

            if (field->isOutOfBounds(coords)) continue; // Exiting the field
            if (visited.count(coords)) continue; // Already visited
            visited.insert(coords);

            if (field->isOccupied(coords)) { // Cell is not free
                Type type = ((Entity*)field->getPawn(coords))->type;
                if (type == Type::WALL || type == Type::PORTAL) continue;
            }
            if (coords.y % (TUNNEL_UNIT * 3) == 0) { // Exiting the breach from below
                foundBelowExit = true;
                // The breach was right above for how neighbouring is defined in a grid
                breach = coords + directionMap[Direction::UP];
                #if DEBUG
                std::cerr << "\t\tWall::getHit() - Found breach below exit at {" << breach.y << "," << breach.x << "}\n";
                #endif
            }
            if (coords.y % (TUNNEL_UNIT * 3) < TUNNEL_UNIT * 2) { // Exiting the breach on the upper side
                foundAboveExit = true;
                #if DEBUG
                std::cerr << "\t\tWall::getHit() - Found breach above exit at {" << coords.y << "," << coords.x << "}\n";
                #endif
                continue;
            }
            if (coords.x < TUNNEL_UNIT * 2
                && (coords.y / TUNNEL_UNIT / 3) % 2 == 0) {
                // On "even" tunnels we should not consider part of the breach the part to the left
                #if DEBUG
                std::cerr << "\t\tWall::getHit() - Skipping left part of the breach at {" << coords.y << "," << coords.x << "}\n";
                #endif
                continue;
            } else if (coords.x >= WIDTH-(TUNNEL_UNIT * 2)
                        && (coords.y / TUNNEL_UNIT / 3) % 2 == 1) {
                // On "odd" tunnels we should not consider part of the breach the part to the right
                #if DEBUG
                std::cerr << "\t\tWall::getHit() - Skipping right part of the breach at {" << coords.y << "," << coords.x << "}\n";
                #endif
                continue;
            }
            if (foundBelowExit && foundAboveExit) break;
            
            dfs.push(coords + directionMap[Direction::UP]);
            dfs.push(coords + directionMap[Direction::LEFT]);
            dfs.push(coords + directionMap[Direction::DOWN]);
            dfs.push(coords + directionMap[Direction::RIGHT]);
        }
        if (foundBelowExit && foundAboveExit) {
            if (breaches.count(breach.y)) {
                breaches[breach.y].push_back(breach.x);
            } else {
                breaches[breach.y] = {breach.x};
            }
        }
        this->remove();
        return true;
    }
    return false;
}
std::bernoulli_distribution Wall::wearing(WALL_WEARING_PROBABILITY);
ANSI::Settings Wall::wallStyle = {
    RGB_ROCKS_FOREGROUND,
    RGB_ROCKS_BACKGROUND,
    ANSI::Attribute::BRIGHT
};