
#ifndef GHOST_BEHAVIOUR_HPP
#define GHOST_BEHAVIOUR_HPP

#include <iostream>
#include <vector>
#include <queue>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "EnvironmentGenerator.hpp"


// Point struct used to compute shortest path;
struct Point { int x, y; };

// Define constants for ghost states;
enum GhostState {
    NORMAL, // General state;
    SCATTER, // Behaviour when closed in an corner. Probably will not be used;
    FRIGHTENED // Behaviour when frighteneed;
};

// Shortest path struct;
struct ShortestPath {
    int distance;
    std::vector<Point> path;
    std::vector<glm::ivec2 > directions;

    // To ease a little the computation if player is still;
    glm::vec3 pacmanPosition;
    glm::ivec2 pacmanDirection;
};


// Possible directions ghosts can take at each frame;
std::vector<glm::ivec2> directions = { glm::ivec2(0, 1), glm::ivec2(1, 0), glm::ivec2(0, -1), glm::ivec2(-1, 0) };


// Define a class for the common properties of all ghosts;
class Ghost {

    protected:

        std::string name; // Ghost name;
        glm::vec3 color; // Ghost main color;
        glm::vec3 frightenedColor; // Ghost color when frightened;

        float currentSpeed; // Current ghost speed;
        float speedModifier; // Speed modifier (generally 1.0f);
        float sizeModifier; // Size modifier (generally 1.0f);

        glm::vec3 startingPosition; // Initial position of the ghost;
        glm::vec3 currentPosition; // Current ghost position;
        glm::vec3 targetPosition; // Current target position;
        glm::ivec2 movingDirection; // Current moving direction;

        // Coordinates that come from the game of the player and the ghost are in continuous floats, however to compute shortwst path on map we need their discrete position;
        glm::ivec2 currentPositionInMap; // Current ghost position in integer format;
        glm::ivec2 targetPositionInMap; // Current target position in interger format;

        GhostState state; // Ghost state, generally NORMAL;
        float modeDuration; // Do not know if it is useful;

        std::vector<std::vector<int>> maze; // The maze saved here;
        int distanceFromPacman = -1; // Distance found from the pacman (Manhattan);

        glm::mat4 modelMatrix = glm::mat4(1.0f); // Multiplied to the model coordinates (vertices) makes it move in the space;
        

    public:

        // Constructor;
        Ghost(std::string ghostName, glm::vec3 startColor, glm::vec3 startPosition, float startSpeed, float startSizeModifier)
            : movingDirection(glm::ivec2(1, 0)), speedModifier(1.0f), modeDuration(12.0f), frightenedColor(glm::vec3(0.2f, 0.2f, 0.2f)) {

            name = ghostName;
            color = startColor;

            currentSpeed = startSpeed;
            startSizeModifier = sizeModifier;

            startingPosition = startPosition;
            currentPosition = startPosition;

            currentPositionInMap = convertToDiscreteCoordinates(currentPosition);

            state = NORMAL;
        }

        // setTargetPosition is a virtual function that differs with the behaviour of different ghosts so it is overridden in the sublasses;
        virtual void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) = 0;

        // Compute all needed to move the model of the ghost in the right direction for reaching pacman with its behaviour specifics;
        void move(float deltaTime, glm::vec3 pacmanPosition, glm::vec3 pacmanDirection = glm::vec3(1.0f, 0.0f, 0.0f)) {

            glm::ivec2 newDirection = movingDirection;
            pacmanPosition.y = 0.0f;
           
            setTargetPosition(pacmanPosition, getPacmanIntDirection(pacmanDirection)); // Set the target position based on the ghost's behavior;
            // printf("%s: going to (%f, %f)\n", name.c_str(), targetPosition.x, targetPosition.z);

            // Convert to discrete coordinates;
            currentPositionInMap = convertToDiscreteCoordinates(currentPosition);
            targetPositionInMap = convertToDiscreteCoordinates(targetPosition);

            // Change direction (if needed) only if the ghost is (more or less) in the center of the cell it's on, to not make them going against the walls;
            if (isOkToChangeDirection()) {
                // Compute the shortest path to the target position;
                ShortestPath shortestPath = shortestPathLeeAlgorithm(maze, Point(currentPositionInMap.x, currentPositionInMap.y), Point(targetPositionInMap.x, targetPositionInMap.y));

                distanceFromPacman = shortestPath.distance;
                newDirection = shortestPath.directions[0];
            }

            if (state == FRIGHTENED) {  } // TODO: go in opposite direction if possible, otherwise IDK :C;

            updateModelMatrix(deltaTime, newDirection); // Update model matrix to move towards the target position;
            movingDirection = newDirection; // Update movingDirection for next iteration;
        }

        // Get the player direction as one of these: {0, 1}, {1, 0}, {0, -1}, {-1, 0} to simplify computation;
        glm::ivec2 getPacmanIntDirection(glm::vec3 pacmanDirection) {
            glm::vec3 absDir = glm::abs(pacmanDirection);
            glm::vec3 primaryDir = glm::vec3(0.0f);
            glm::ivec2 direction;

            if (absDir.x >= absDir.z) { primaryDir.x = glm::sign(pacmanDirection.x); }
            else { primaryDir.z = glm::sign(pacmanDirection.z); }

            direction.x = static_cast<int>(primaryDir.x);
            direction.y = static_cast<int>(primaryDir.z);

            return direction;
        }

        // Convert currentPosition or targetPosition into discrete coordinates. Cannot be done now since I have no idea how the maze will be positioned in the world coordinates;
        glm::ivec2 convertToDiscreteCoordinates(glm::vec3 continuousCoordinates) {

            int col = static_cast<int>(continuousCoordinates.x + 0.5f); // Not sure it is completely correct to round up like this; TODO: check;
            int row = static_cast<int>(continuousCoordinates.z + 0.5f); // Not sure it is completely correct to round up like this;

            // Clamp row and col to ensure they are within the maze boundaries;
            row = glm::clamp(row, 0, 30);
            col = glm::clamp(col, 0, 26);

            // std::cout << row << ", " << col << ";\n";
            return glm::ivec2(col, row);
        }

        // Get the difference between the currentPosition and the integer one wrt the map;
        float difference(float currentPos, int currentPosMap) { return currentPos - currentPosMap; }

        // Check the ghost is circa in the center of the cell it's in;
        bool isOkToChangeDirection() {
            float diffX = difference(currentPosition.x, currentPositionInMap.x);
            float diffY = difference(currentPosition.z, currentPositionInMap.y);
            return (diffX > 0.45f && diffX < 0.55f && diffY > 0.45f && diffY);
        }

        // Compute angle between 2 vectors;
        float angleBetweenVectors(glm::vec2 v1, glm::vec2 v2) {
            float dot = glm::dot(glm::normalize(v1), glm::normalize(v2));
            dot = glm::clamp(dot, -1.0f, 1.0f);

            float angle = std::acos(dot);
            float cross = glm::cross(glm::vec3(v1, 0), glm::vec3(v2, 0)).z;

            return cross >= 0 ? angle : -angle;
        }

        // Update model matrix;
        void updateModelMatrix(float deltaTime, glm::ivec2 newDirection) {
            float angle = angleBetweenVectors(glm::normalize(glm::vec2(movingDirection)), glm::normalize(glm::vec2(newDirection)));

            modelMatrix =
                glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f)) * // Only needs to rotate the "pitch" for the ghost;
                glm::translate(glm::mat4(1.0f), glm::vec3(currentSpeed * speedModifier * deltaTime, 0.0f, 0.0f)); // If the ghost is moving but not watching in front of itself this line can solve the problem. TODO: check;
        }

        // Determine equivalence among floats;
        bool floatEquivalent(float a, float b, float epsilon = 1e-3f) {
            float diff = std::fabs(a - b);
            if (diff <= epsilon) { return true; }
            return diff <= epsilon * std::fmax(std::fabs(a), std::fabs(b));
        }


        // Setters and getters:

        // Set the maze;
        void setMaze(std::vector<std::vector<int>> maze) { this->maze = maze; }
        
        // Return the model matrix;
        glm::mat4 getModelMatrix() { return modelMatrix; }


    private :

        // Lee algorithm to find the shortest path between the current and the target positions;
        ShortestPath shortestPathLeeAlgorithm(const std::vector<std::vector<int>>& maze, Point currentPos, Point targetPos) {

            int rows = maze.size();
            int cols = maze[0].size();
            std::vector<std::vector<int>> distances(rows, std::vector<int>(cols, -1));
            std::vector<std::vector<Point>> parents(rows, std::vector<Point>(cols, { -1, -1 }));
            std::queue<Point> q;

            distances[currentPos.x][currentPos.y] = 0;
            q.push(currentPos);


            while (!q.empty()) {
                Point current = q.front();
                q.pop();

                for (int i = 0; i < 4; i ++) {
                    Point newPos = { current.x + directions[i].x, current.y + directions[i].y };

                    // Check if the new position is within the maze boundaries;
                    if (newPos.x >= 0 && newPos.x < rows && newPos.y >= 0 && newPos.y < cols && maze[newPos.x][newPos.y] != 1 && distances[newPos.x][newPos.y] == -1) {

                        // Update distance and parent;
                        distances[newPos.x][newPos.y] = distances[current.x][current.y] + 1;
                        parents[newPos.x][newPos.y] = current;
                        q.push(newPos);

                        // If we reached the target, reconstruct the path;
                        if (newPos.x == targetPos.x && newPos.y == targetPos.y) {
                            std::vector<Point> path;
                            std::vector<glm::ivec2> directionIndices;
                            Point backtrack = targetPos;

                            while (backtrack.x != currentPos.x || backtrack.y != currentPos.y) {
                                path.push_back(backtrack);
                                Point parent = parents[backtrack.x][backtrack.y];

                                // Determine the direction index from parent to backtrack;
                                for (int j = 0; j < 4; j ++) { if (parent.x + directions[j].x == backtrack.x && parent.y + directions[j].y == backtrack.y) { directionIndices.push_back(directions[j]); } }
                                backtrack = parent;
                            }

                            path.push_back(currentPos);
                            reverse(path.begin(), path.end());
                            reverse(directionIndices.begin(), directionIndices.end());

                            return ShortestPath(distances[targetPos.x][targetPos.y], path, directionIndices);
                        }
                    }
                }
            }
            return ShortestPath(- 1, {}, {} ); // No path found;
        }

};


// Define subclasses for different ghost personalities;

class ChaserGhost : public Ghost {

    public:

        ChaserGhost(std::string ghostName, glm::vec3 startColor, glm::vec3 startPosition, float startSpeed, float startSize) : Ghost(ghostName, startColor, startPosition, startSpeed, startSize) { }

        void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) override { targetPosition = pacmanPosition; }
};

class AmbusherGhost : public Ghost {

    protected:
        int ambushDistance;

    public:

        AmbusherGhost(std::string ghostName, glm::vec3 startColor, glm::vec3 startPosition, float startSpeed, float startSize, int ambushDistance)
            : Ghost(ghostName, startColor, startPosition, startSpeed, startSize), ambushDistance(ambushDistance) { }

        void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) override {
            glm::vec3 target = pacmanPosition;

            target.x += ambushDistance * pacmanDirection.x;
            target.z += ambushDistance * pacmanDirection.y;

            // Check if the target is within the maze boundaries and is not a wall or ghost hub;
            if (target.x >= 0 && target.x < maze.size() && target.z >= 0 && target.z < maze[0].size() &&
                maze[target.x][target.z] != WALL && maze[target.x][target.z] != GHOSTS_HUB) { targetPosition = target; }

            // Repeat if no cells found until we come back again on the pacman current position;
            while (!floatEquivalent(target.x, pacmanPosition.x) || !floatEquivalent(target.z, pacmanPosition.z)) {

                Point perpendicularDirection = { pacmanDirection.y, pacmanDirection.x }; // Direction perpendicular to the one in which pacman is moving;
                // Check the 2 positions adjacent in the perpendicular direction;s
                for (int i = -1; i <= 1; i += 2) {
                    int nx = static_cast<int>(target.x) + i * perpendicularDirection.x;
                    int nz = static_cast<int>(target.z) + i * perpendicularDirection.y;
                    if (nx >= 0 && nx < maze.size() && nz >= 0 && nz < maze[0].size() && maze[nx][nz] != WALL && maze[nx][nz] != GHOSTS_HUB) { targetPosition = { nx + 0.5f, 0.0f, nz + 0.5f }; return; }
                }

                // Didn't found a suitable cell, move one back and continue the search;
                target.x -= pacmanDirection.x;
                target.z -= pacmanDirection.y;
            }

            // Set pacmanPosition if no suitable cell is found before;
            targetPosition = pacmanPosition; return;
        }
};

class ProtectorGhost : public Ghost {

    public:

        ProtectorGhost(std::string ghostName, glm::vec3 startColor, glm::vec3 startPosition, float startSpeed, float startSize) : Ghost(ghostName, startColor, startPosition, startSpeed, startSize) { }

        void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) override {
            if (distanceFromPacman > 8) { targetPosition = {29.0f, 0.0f, 2.0f}; } // Only working with original pacman maze!!! TODO ;
            else { targetPosition = pacmanPosition; }
        }
};

#endif