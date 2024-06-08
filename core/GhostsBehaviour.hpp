
#ifndef GHOST_BEHAVIOUR_HPP
#define GHOST_BEHAVIOUR_HPP

#include <iostream>
#include <vector>
#include <queue>
#include <cmath>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/vector_angle.hpp>

#include "EnvironmentGenerator.hpp"


const float ghostsHeight = 0.6f;
const float ghostsScale = 0.275f;


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
    std::vector<glm::ivec2> path;
    std::vector<glm::vec3> pathInWorld;
    std::vector<glm::ivec2 > directions;

    ShortestPath(int distance, std::vector<glm::ivec2> path, std::vector<glm::vec3> pathInWorld, std::vector<glm::ivec2> directions) : distance(distance), path(path), pathInWorld(pathInWorld), directions(directions) { }
    ShortestPath(int distance, std::vector<glm::ivec2> path, std::vector<glm::ivec2> directions) : distance(distance), path(path), directions(directions) { }
    ShortestPath() : distance(-1), path({}), directions({}) { }

    //// To ease a little the computation if player is still;
    //glm::vec3 pacmanPosition;
    //glm::ivec2 pacmanDirection;
};

// Possible directions ghosts can take at each frame;
std::vector<glm::ivec2> dirs = { glm::ivec2(0, 1), glm::ivec2(1, 0), glm::ivec2(0, -1), glm::ivec2(-1, 0) };


// Define a class for the common properties of all ghosts;
class Ghost {

    protected:

        std::string name; // Ghost name;
        glm::vec3 color; // Ghost main color;
        glm::vec3 frightenedColor; // Ghost color when frightened;

        float currentSpeed; // Current ghost speed;
        float speedModifier; // Speed modifier (normally 1.0f);
        float sizeModifier; // Size modifier (normally 1.0f);

        glm::vec3 currentPosition; // Current ghost position;
        glm::vec3 targetPosition; // Current target position;

        // Coordinates that come from the game of the player and the ghost are in continuous floats, however to compute shortwst path on map we need their discrete position;
        glm::ivec2 currentPositionInMap; // Current ghost position in integer format;
        glm::ivec2 targetPositionInMap; // Current target position in interger format;

        GhostState state; // Ghost state, generally NORMAL;
        float modeDuration; // Do not know if it is useful;

        std::vector<std::vector<int>> maze; // The maze saved here;
        int distanceFromPacman = -1; // Distance found from the pacman (Manhattan);

        ShortestPath shortestPath; // Shortest path to reach the target position;

        glm::mat4 initialMatrixTransf; // Model matrix of initial state of ghost;
        glm::mat4 modelMatrix; // Multiplied to the model coordinates (vertices) makes it move in the space;


        glm::ivec2 frontDirection;

        float elapsedTime = 0.0f;
        

    public:

        // Constructor;
        Ghost(std::string ghostName, glm::vec3 startColor, glm::vec3 startPosition, float startSpeed) :
            name(ghostName), color(startColor), currentPosition(startPosition), state(NORMAL), modeDuration(12.0f),
            currentSpeed(startSpeed), speedModifier(1.0f), sizeModifier(1.0f), frontDirection(glm::ivec2(0.0f, 1.0f)) { }

        // setTargetPosition is a virtual function that differs with the behaviour of different ghosts so it is overridden in the sublasses;
        virtual void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) { };

        // Compute all needed to move the model of the ghost in the right direction for reaching pacman with its behaviour specifics;
        void move(float deltaTime, glm::vec3 pacmanPosition, glm::vec3 pacmanDirection = glm::vec3(0.0f)) {

            deltaTime = deltaTime < 1.0f ? deltaTime : 0.0f;

            // If the ghost is frightened, set the target position to the ghost hub; TODO;

            // setTargetPosition(pacmanPosition, pacmanDirection);
            setTargetPosition({ 11.5f, 0.6f, 10.5f }, { 0.0f, 0.0f });

            // Compute the shortest path from the current position to the pacman position;
            // ShortestPath sp = shortestPathLeeAlgorithm(maze, { currentPositionInMap.x, currentPositionInMap.y }, { targetPositionInMap.x, targetPositionInMap.y });

            ShortestPath sp = computeShortestPath();

            if (sp.path.empty() || sp.directions.empty()) { return; } // If (for any reason) the path is empty, return;

            glm::vec3 nextCellPath = sp.pathInWorld[0];
            glm::ivec2 nextDirection = sp.directions[0];
            distanceFromPacman = sp.distance;


            float angle = glm::orientedAngle(glm::vec2(frontDirection), glm::vec2(nextDirection));
            angle = glm::radians(90.0f) - angle;
            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));


            glm::vec3 translation = glm::vec3(nextDirection.x, 0.0f, nextDirection.y) * deltaTime * currentSpeed * 0.5f;
            glm::vec3 newPosition = translation + currentPosition;
            glm::vec3 initialTranslation = glm::vec3(initialMatrixTransf[3]);
            glm::vec3 constantCellCenter = glm::vec3(0.5f, 0.0f, -0.5f);
            glm::vec3 translationDifference = newPosition + constantCellCenter - initialTranslation;
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translationDifference);


            // Combine the new translation matrix with the initial model matrix;
            modelMatrix = translationMatrix * initialMatrixTransf * rotationMatrix;

            // Update the current position and direction;
            currentPosition = newPosition;
            //initialFrontDirection = nextDirection;
        }

        // Determine equivalence among floats;
        bool floatEquivalence(double a, double b, double tolerance = 1e-4) { return std::abs(a - b) <= tolerance; }


        // Setters and getters:

        // Set the maze;
        void setMaze(std::vector<std::vector<int>> maze) { this->maze = maze; }

        // Set the model matrix;
        void setInitialModelMatrix(glm::mat4 modelMat) { initialMatrixTransf = modelMat; modelMatrix = initialMatrixTransf; }
        
        // Return the model matrix;
        glm::mat4 getModelMatrix() { return modelMatrix; }

        // Return the current position;
        glm::vec3 getCurrentPosition() { return currentPosition; }


    private :

        // Lee algorithm to find the shortest path between the current and the target positions;
        ShortestPath shortestPathLeeAlgorithm(glm::ivec2 currentPos, glm::ivec2 targetPos) {

            size_t rows = maze.size();
            size_t cols = maze[0].size();
            std::vector<std::vector<int>> distances(rows, std::vector<int>(cols, -1));
            std::vector<std::vector<glm::ivec2>> parents(rows, std::vector<glm::ivec2>(cols, { -1, -1 }));
            std::queue<glm::ivec2> q;

            distances[currentPos.x][currentPos.y] = 0;
            q.push(currentPos);

            while (!q.empty()) {
                glm::ivec2 current = q.front();
                q.pop();

                for (int i = 0; i < 4; i++) {
                    glm::ivec2 newPos = { current.x + dirs[i].x, current.y + dirs[i].y };

                    // Check if the new position is within the maze boundaries;
                    if (newPos.x >= 0 && newPos.x < rows && newPos.y >= 0 && newPos.y < cols && maze[newPos.x][newPos.y] != WALL && distances[newPos.x][newPos.y] == -1) {

                        // Update distance and parent;
                        distances[newPos.x][newPos.y] = distances[current.x][current.y] + 1;
                        parents[newPos.x][newPos.y] = current;
                        q.push(newPos);

                        // If we reached the target, reconstruct the path;
                        if (newPos.x == targetPos.x && newPos.y == targetPos.y) {
                            std::vector<glm::ivec2> path;
                            std::vector<glm::ivec2> directionIndices;
                            glm::ivec2 backtrack = targetPos;

                            while (backtrack.x != currentPos.x || backtrack.y != currentPos.y) {
                                path.push_back(glm::ivec2(backtrack.x, backtrack.y));
                                glm::ivec2 parent = parents[backtrack.x][backtrack.y];

                                // Determine the direction index from parent to backtrack;
                                for (int j = 0; j < 4; j++) { if (parent.x + dirs[j].x == backtrack.x && parent.y + dirs[j].y == backtrack.y) { directionIndices.push_back(dirs[j]); } }
                                backtrack = parent;
                            }

                            path.push_back(glm::ivec2(currentPos.x, currentPos.y));
                            reverse(path.begin(), path.end());
                            reverse(directionIndices.begin(), directionIndices.end());

                            return ShortestPath(distances[targetPos.x][targetPos.y], path, directionIndices);
                        }
                    }
                }
            }
            return ShortestPath(-1, { {currentPos.x, currentPos.y} }, { dirs[0] }); // No path found, stay still;
        }

        // Compute the shortest path from the current position to the target position;
        ShortestPath computeShortestPath() {

            size_t height = maze.size();
            size_t width = maze[0].size();

            // Convert start and target positions to grid coordinates
            glm::ivec2 currentPos = toGridCoordinates(currentPosition, height, width);
            glm::ivec2 targetPos = toGridCoordinates(targetPosition, height, width);

            if (currentPos.x < 0 || currentPos.x >= width || currentPos.y < 0 || currentPos.y >= height || targetPos.x < 0 || targetPos.x >= width || targetPos.y < 0 || targetPos.y >= height) {
                return ShortestPath(-1, { {currentPos.x, currentPos.y} }, { dirs[0] }); // No path found, stay still;
            }

            // Get the shortest path;
            ShortestPath shortestPath = shortestPathLeeAlgorithm(currentPos, targetPos);

            // Convert the path to world coordinates;
            for (const auto& pos : shortestPath.path) { shortestPath.pathInWorld.push_back(toWorldCoordinates(pos, height, width)); }


            // Convert the directions;
            for (auto& dir : shortestPath.directions) { dir = glm::ivec2(dir.x, -dir.y); }

            return shortestPath;
        }

        // Convert world coordinates to grid coordinates;
        glm::ivec2 toGridCoordinates(const glm::vec3& worldPos, int height, int width) { return glm::ivec2(floor(worldPos.x) + height / 2, (- floor(worldPos.z) + width / 2)); }

        // Convert grid coordinates to world coordinates;
        glm::vec3 toWorldCoordinates(const glm::ivec2& gridPos, int height, int width) { return glm::vec3(gridPos.x - height / 2 + 0.5f, 0.0f, - (gridPos.y - width / 2 + 1) + 0.5f); } // Is the + 1 correct? TODO check;


        // Print maze with path inside;
        void printMazeWithPath(std::vector<std::vector<int>> maze, const std::vector<Point>& path) {
            size_t offsetX = maze.size() / 2;
            size_t offsetY = maze[0].size() / 2;

            // Mark the path in the maze;
            for (const Point& p : path) { maze[p.x + offsetX][p.y + offsetY] = PATH; }

            // Print the maze with the path;
            for (const auto& row : maze) {
                for (int cell : row) {
                    switch (cell) {
                    case EMPTY: std::cout << "  "; break;
                    case WALL: std::cout << char(254) << " "; break;
                    case PELLET: std::cout << " " /*char(167)*/ << " "; break;
                    case POWER_PELLET: std::cout << " " /*char(155)*/ << " "; break;
                    case GHOSTS_HUB: std::cout << char(126) << " "; break;
                    case TELEPORT_HORIZONTAL: std::cout << char(196) << " "; break;
                    case TELEPORT_VERTICAL: std::cout << char(179) << " "; break;
                    case PATH: std::cout << "X "; break;
                    }
                }
                std::cout << std::endl;
            }
        }

        // Print matrix;
        void printMatrix(glm::mat4 matrix) { for (int i = 0; i < 4; i++) { for (int j = 0; j < 4; j++) { std::cout << matrix[i][j] << " "; } std::cout << std::endl; } std::cout << std::endl; }

};


// Define subclasses for different ghost personalities;

class ChaserGhost : public Ghost {

    public:

        ChaserGhost(std::string ghostName, glm::vec3 startColor, glm::vec3 startPosition, float startSpeed) : Ghost(ghostName, startColor, startPosition, startSpeed) { }

        void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) override { targetPosition = pacmanPosition; }

};

class AmbusherGhost : public Ghost {

    protected:
        int ambushDistance;

    public:

        AmbusherGhost(std::string ghostName, glm::vec3 startColor, glm::vec3 startPosition, float startSpeed, int ambushDistance)
            : Ghost(ghostName, startColor, startPosition, startSpeed), ambushDistance(ambushDistance) { }

        void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) override {

            targetPosition = pacmanPosition; return;

            glm::vec3 target = pacmanPosition;
            size_t offsetX = maze.size() / 2;
            size_t offsetZ = maze[0].size() / 2;

            target.x += ambushDistance * pacmanDirection.x;
            target.z += ambushDistance * pacmanDirection.y;

            // Check if the target is within the maze boundaries and is not a wall or ghost hub;
            if (target.x + offsetX >= 0 && target.x + offsetX < maze.size() && target.z + offsetZ >= 0 && target.z + offsetZ < maze[0].size() &&
                maze[static_cast<size_t>(target.x) + offsetX][static_cast<size_t>(target.z) + offsetZ] != WALL && maze[static_cast<size_t>(target.x) + offsetX][static_cast<size_t>(target.z) + offsetZ] != GHOSTS_HUB) {

                targetPosition = target;
            }

            // Repeat if no cells found until we come back again on the pacman current position;
            while (!floatEquivalence(target.x + offsetX, pacmanPosition.x) || !floatEquivalence(target.z + offsetZ, pacmanPosition.z)) {

                Point perpendicularDirection = { pacmanDirection.y, pacmanDirection.x }; // Direction perpendicular to the one in which pacman is moving;
                // Check the 2 positions adjacent in the perpendicular direction;s
                for (int i = -1; i <= 1; i += 2) {
                    int nx = static_cast<int>(target.x + offsetX) + i * perpendicularDirection.x;
                    int nz = static_cast<int>(target.z + offsetZ) + i * perpendicularDirection.y;
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

        ProtectorGhost(std::string ghostName, glm::vec3 startColor, glm::vec3 startPosition, float startSpeed) : Ghost(ghostName, startColor, startPosition, startSpeed) { }

        void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) override {
            if (distanceFromPacman > 8) { targetPosition = {0.0f, 0.0f, 0.0f}; } // Only working with original pacman maze!!! TODO ;
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
            blinky(std::make_shared<ChaserGhost>("Blinky", glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(-4.0f, ghostsHeight, 0.0f), 2.5f)),
            pinky(std::make_shared<AmbusherGhost>("Pinky", glm::vec3(1.0f, 0.7f, 0.9f), glm::vec3(-1.0f, ghostsHeight, 2.0f), 2.5f, 4)),
            inky(std::make_shared<AmbusherGhost>("Inky", glm::vec3(0.5f, 0.96f, 1.0f), glm::vec3(-1.0f, ghostsHeight, 0.0f), 2.5f, 2)),
            clyde(std::make_shared<ProtectorGhost>("Clyde", glm::vec3(0.91f, 0.7f, 0.0f), glm::vec3(-1.0f, ghostsHeight, -2.0f), 2.5f)) {
            setMazeInGhosts(maze);
        }

        void moveAllGhosts(float deltaTime, glm::vec3 playerPosition, glm::vec3 playerDirection) {
            blinky->move(deltaTime, playerPosition);
            //pinky->move(deltaTime, playerPosition, playerDirection);
            //inky->move(deltaTime, playerPosition, playerDirection);
            //clyde->move(deltaTime, playerPosition) ;
        }

        void setMazeInGhosts(std::vector<std::vector<int>> maze) { blinky->setMaze(maze); pinky->setMaze(maze); inky->setMaze(maze); clyde->setMaze(maze); }
};

#endif