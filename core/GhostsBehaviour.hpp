
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

    //// To ease a little the computation if player is still;
    //glm::vec3 pacmanPosition;
    //glm::ivec2 pacmanDirection;
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
        Ghost(std::string ghostName, glm::vec3 startColor, glm::vec3 startPosition, float startSpeed, float startSize) { }

        // setTargetPosition is a virtual function that differs with the behaviour of different ghosts so it is overridden in the sublasses;
        virtual void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) { };

        // Compute all needed to move the model of the ghost in the right direction for reaching pacman with its behaviour specifics;
        void move(float deltaTime, glm::vec3 pacmanPosition, glm::vec3 pacmanDirection = glm::vec3(0.0f)) {

            // Compute the shortest path from the current position to the pacman position;
            // ShortestPath shortestPath = shortestPathLeeAlgorithm(maze, { currentPositionInMap.x, currentPositionInMap.y }, { static_cast<int>(pacmanPosition.x), static_cast<int>(pacmanPosition.z) });

            


        }

        








        // Update model matrix;
        void updateModelMatrix() {
        }

        // Determine equivalence among floats;
        bool floatEquivalence(double a, double b, double tolerance = 1e-4) { return std::abs(a - b) <= tolerance; }


        // Setters and getters:

        // Set the maze;
        void setMaze(std::vector<std::vector<int>> maze) { this->maze = maze; }

        // Set the model matrix;
        void setModelMatrix(glm::mat4 modelMat) { this->modelMatrix = modelMat; }
        
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
            while (!floatEquivalence(target.x, pacmanPosition.x) || !floatEquivalence(target.z, pacmanPosition.z)) {

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

class GhostCollection {

    public:

        std::shared_ptr<ChaserGhost> blinky;
        std::shared_ptr<AmbusherGhost> pinky;
        std::shared_ptr<AmbusherGhost> inky;
        std::shared_ptr<ProtectorGhost> clyde;

        GhostCollection(std::vector<std::vector<int>> maze) :
            blinky(std::make_shared<ChaserGhost>("Blinky", glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-4.0f, ghostsHeight, 0.0f), 1.0f, 1.0f)),
            pinky(std::make_shared<AmbusherGhost>("Pinky", glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(-1.0f, ghostsHeight, 2.0f), 1.0f, 1.0f, 4)),
            inky(std::make_shared<AmbusherGhost>("Inky", glm::vec3(0.5f, 0.96f, 1.0f), glm::vec3(-1.0f, ghostsHeight, 0.0f), 1.0f, 1.0f, 2)),
            clyde(std::make_shared<ProtectorGhost>("Clyde", glm::vec3(0.91f, 0.7f, 0.0f), glm::vec3(-1.0f, ghostsHeight, -2.0f), 1.0f, 1.0f)) {
            setMazeInGhosts(maze);
        }

        void moveAllGhosts(float deltaTime, glm::vec3 playerPosition, glm::vec3 playerDirection) {
            blinky->move(deltaTime, playerPosition);
            pinky->move(deltaTime, playerPosition, playerDirection);
            inky->move(deltaTime, playerPosition, playerDirection);
            clyde->move(deltaTime, playerPosition);
        }

        void setMazeInGhosts(std::vector<std::vector<int>> maze) { blinky->setMaze(maze); pinky->setMaze(maze); inky->setMaze(maze); clyde->setMaze(maze); }
};

#endif