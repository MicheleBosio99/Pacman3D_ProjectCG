
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

#include "GameEnvGenerator.hpp"
#include "SoundManager.hpp"


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
};

// Possible directions ghosts can take at each frame;
std::vector<glm::ivec2> dirs = { glm::ivec2(0, 1), glm::ivec2(1, 0), glm::ivec2(0, -1), glm::ivec2(-1, 0) };


// Define a class for the common properties of all ghosts;
class Ghost {

    protected:

        std::string name; // Ghost name;
        glm::vec3 color; // Ghost main color;
        glm::vec3 frightenedColor; // Ghost color when frightened;

        std::string soundName = ""; // Name of the sound related to ghost;

        float currentSpeed; // Current ghost speed;
        float speedModifier; // Speed modifier (normally 1.0f);
        float sizeModifier; // Size modifier (normally 1.0f);

        glm::vec3 initialPosition; // Initial ghost position;
        glm::vec3 currentPosition; // Current ghost position;
        glm::vec3 targetPosition; // Current target position;
        glm::vec3 scatterPosition; // Position to go when in scatter mode;

        // Coordinates that come from the game of the player and the ghost are in continuous floats, however to compute shortwst path on map we need their discrete position;
        glm::ivec2 currentPositionInMap; // Current ghost position in integer format;
        glm::ivec2 targetPositionInMap; // Current target position in interger format;

        GhostState state; // Ghost state, generally NORMAL;
        float modeDuration; // Do not know if it is useful;

        std::vector<std::vector<int>> maze; // The maze saved here;

        ShortestPath shortestPath; // Shortest path to reach the target position;
        int distanceFromPacman = -1; // Distance found from the pacman (Manhattan);
        bool ghostGotPacman = false; // If the ghost got the pacman;


        glm::mat4 initialMatrixTransf; // Model matrix of initial state of ghost;
        glm::mat4 modelMatrix; // Multiplied to the model coordinates (vertices) makes it move in the space;
        glm::ivec2 frontDirection;

        float elapsedTime = 0.0f;

        bool ghostGotEaten = false;

        int i;

    public:

        // Constructor;
        Ghost(std::string ghostName, glm::vec3 startColor, std::vector<std::vector<int>> maze, glm::vec3 startPosition, glm::vec3 scatterPosition, float startSpeed, float speedModifier, float modeDuration) :
            name(ghostName), color(startColor), maze(maze), initialPosition(startPosition), currentPosition(startPosition), scatterPosition(scatterPosition), state(NORMAL), modeDuration(modeDuration),
            currentSpeed(startSpeed), speedModifier(speedModifier), sizeModifier(1.0f), frontDirection(glm::ivec2(0.0f, 1.0f)) {
            currentPositionInMap = toGridCoordinates(currentPosition);
        }

        // setTargetPosition is a virtual function that differs with the behaviour of different ghosts so it is overridden in the sublasses;
        virtual void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) { };

        // Compute all needed to move the model of the ghost in the right direction for reaching pacman with its behaviour specifics;
        void move(float deltaTime, int index, std::vector<glm::ivec2> ghostsPositionsInMap, glm::vec3 pacmanPosition, glm::vec3 pacmanDirection = glm::vec3(0.0f)) {

            if (soundName == "") { initializeGhostSirenSounds(); }

            if (deltaTime > 0.5f) { return; }

            // pacmanPosition = computeRealPacmanPosition(pacmanPosition - glm::vec3(0.5f, 0.0f, -0.5f));

            glm::vec3 currentTargetPosition = pacmanPosition;
            switch (state) {
                case NORMAL: currentTargetPosition = pacmanPosition; break;
				case FRIGHTENED: currentTargetPosition = initialPosition; break;
				case SCATTER: currentTargetPosition = scatterPosition; break;
            }
            setTargetPosition(currentTargetPosition, glm::ivec2(pacmanDirection.x, pacmanDirection.z));

            currentPositionInMap = toGridCoordinates(currentPosition - glm::vec3(0.5f, 0.0f, -0.5f));
            targetPositionInMap = toGridCoordinates(targetPosition - glm::vec3(0.5f, 0.0f, -0.5f));


            if (currentPositionInMap == toGridCoordinates(pacmanPosition - glm::vec3(0.5f, 0.0f, -0.5f))) {
                if (state != FRIGHTENED) { ghostGotPacman = true; return; }
                else { SoundManager::playSound("pacman_ghost-eaten"); ghostGotEaten = true; ghostGotEatenBehaviour(); }
            }

            shortestPath = computeShortestPath(index, ghostsPositionsInMap);
            // printMazeWithPath(maze, shortestPath.path, index);

            if (shortestPath.path.empty() || shortestPath.pathInWorld.empty() || shortestPath.directions.empty() || shortestPath.distance == -1) { return; i++;  std::cout << "Ret: " << i << std::endl; return; }

            glm::ivec2 nextCellInMap = shortestPath.path[1];
            glm::vec3 nextCell = shortestPath.pathInWorld[1];
            glm::ivec2 nextDirection = shortestPath.directions[0];

            // Check if nextCell has already a ghost inside;
            for (int i = 0; i < ghostsPositionsInMap.size(); i ++) { if (i != index && ghostsPositionsInMap[i] == nextCellInMap) { return; } }


            // Compute rotation matrix;
            float angle = glm::radians(90.0f) - glm::orientedAngle(glm::vec2(frontDirection), glm::vec2(nextDirection));
            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));

            // Compute translation matrix;
            float distanceToNextCell = glm::distance(currentPosition, nextCell);
            float movement = deltaTime * currentSpeed * speedModifier;
            if (movement > distanceToNextCell) { movement = distanceToNextCell; }

            glm::vec3 translation = glm::normalize(nextCell - currentPosition) * movement;
            glm::vec3 newPosition = translation + currentPosition;
            glm::vec3 initialTranslation = glm::vec3(initialMatrixTransf[3]);
            glm::vec3 translationDifference = newPosition - initialTranslation;
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translationDifference);

            // Combine the new translation matrix and the new rotation matrix with the initial model matrix;
            modelMatrix = translationMatrix * initialMatrixTransf * rotationMatrix;

            // Update the current position;
            currentPosition = newPosition;

            // Generate directional sound;
            //if(glm::distance)

            glm::vec3 soundDirection = glm::normalize(pacmanPosition - currentPosition);
            irrklang::vec3df irrKlangPosition(currentPosition.x, currentPosition.y, currentPosition.z);

            // Play/Update the sound;
            if (!SoundManager::isSoundPlaying(soundName)) {
                SoundManager::soundsPlaying[soundName]->setPosition(irrKlangPosition);
                SoundManager::soundsPlaying[soundName]->setIsPaused(false);
            }
            else {
                SoundManager::updateSoundPosition(soundName, irrKlangPosition);
            }
        }

        // Behaviour when the ghost gets eaten;
        void ghostGotEatenBehaviour(int time = 5) {
            std::thread eatenThread([this, time] {
                modelMatrix = initialMatrixTransf * glm::translate(glm::mat4(1.0f), initialPosition);
                currentPosition = initialPosition;

                float s = speedModifier;
                speedModifier = 0.0f;
                std::this_thread::sleep_for(std::chrono::seconds(time));
                speedModifier = s;
                ghostGotEaten = false;
            });

            eatenThread.detach(); // Threads runs independently;
        }

        // Compute the real pacman position;
        glm::vec3 computeRealPacmanPosition(glm::vec3 pacmanPosition) {
            /*float dummyVariable;
            glm::ivec2 pacmanPositionInMap = toGridCoordinates(pacmanPosition);

            if (maze[pacmanPositionInMap.x][pacmanPositionInMap.y] == WALL) {

                if (std::modff(pacmanPosition.x, &dummyVariable) < 0.15f) { return pacmanPosition - glm::vec3(0.2f, 0.0f, 0.0f); }
                else if (std::modff(pacmanPosition.x, &dummyVariable) > 0.85f) { return pacmanPosition + glm::vec3(0.2f, 0.0f, 0.0f); }

                if (std::modff(pacmanPosition.z, &dummyVariable) < 0.15f) { return pacmanPosition - glm::vec3(0.2f, 0.0f, 0.0f); }
                else if (std::modff(pacmanPosition.z, &dummyVariable) > 0.85f) { return pacmanPosition + glm::vec3(0.2f, 0.0f, 0.0f); }

                std::cout << pacmanPosition.x << " " << pacmanPosition.z << std::endl;
            }*/
        }

        // Initialize the ghost siren sounds;
        void initializeGhostSirenSounds() {
            soundName = std::format("pacman_{}-siren", name);
            irrklang::vec3df irrKlangPos(currentPosition.x, currentPosition.y, currentPosition.z);
            irrklang::ISound* ghostSirenSound = SoundManager::playSound3D("pacman_ghost-siren", irrKlangPos, true, true);
            SoundManager::soundsPlaying[soundName] = ghostSirenSound;

            ghostSirenSound->setMinDistance(4.0f);
            ghostSirenSound->setMaxDistance(20.0f);
            ghostSirenSound->setPosition(irrKlangPos);
            ghostSirenSound->setIsPaused(false);
            ghostSirenSound->setVolume(1.0f);
        }

        // Determine equivalence among floats;
        bool floatEquivalence(double a, double b, double tolerance = 1e-4) { return std::abs(a - b) <= tolerance; }

        // Set the model matrix;
        void setInitialModelMatrix(glm::mat4 modelMat) { initialMatrixTransf = modelMat; modelMatrix = initialMatrixTransf; }
        
        // Return the model matrix;
        glm::mat4 getModelMatrix() { return modelMatrix; }

        // Return the current position;
        glm::vec3 getCurrentPosition() { return currentPosition; }

        // Return the current position in map;
        glm::ivec2 getCurrentPositionInMap() { return currentPositionInMap; }

        // Getter of ghost state;
        GhostState getGhostState() { return state; }

        // Setter of ghost state;
        void setGhostState(GhostState state) { this->state = state; }

        // Getter of ghost got pacman;
        bool didGhostGotPacman() { return ghostGotPacman; }

        // Setter of ghost got pacman;
        void setGhostGotPacman(bool ghostGotPacman) { this->ghostGotPacman = ghostGotPacman; }

        // Setter of ghost speedModifier;
        void setSpeedModifier(float speedModifier) { this->speedModifier = speedModifier; }

        // Getter of ghost got eaten;
        bool didGhostGotEaten() { return ghostGotEaten; }

        // Setter of ghost got eaten;
        void setGhostGotEaten(bool ghostGotEaten) { this->ghostGotEaten = ghostGotEaten; }


    private :

        // Lee algorithm to find the shortest path between the current and the target positions;
        ShortestPath shortestPathLeeAlgorithm(std::vector<std::vector<int>> mazeWithGhosts, glm::ivec2 currentPos, glm::ivec2 targetPos) {

            size_t rows = mazeWithGhosts.size();
            size_t cols = mazeWithGhosts[0].size();
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
                    if (newPos.x >= 0 && newPos.x < rows && newPos.y >= 0 && newPos.y < cols && mazeWithGhosts[newPos.x][newPos.y] != WALL && distances[newPos.x][newPos.y] == -1) {

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
        ShortestPath computeShortestPath(int index, std::vector<glm::ivec2> ghostsPositionsInMap) {

            size_t height = maze.size();
            size_t width = maze[0].size();

            // Convert start and target positions to grid coordinates
            glm::ivec2 currentPos = toGridCoordinates(currentPosition);
            glm::ivec2 targetPos = toGridCoordinates(targetPosition);

            if (currentPos == targetPos) { return ShortestPath(0, { { currentPos.x, currentPos.y } }, { dirs[0] }); }

            if (currentPos.x < 0 || currentPos.x >= height || currentPos.y < 0 || currentPos.y >= width || targetPos.x < 0 || targetPos.x >= height || targetPos.y < 0 || targetPos.y >= width) {
                return ShortestPath(-1, { {currentPos.x, currentPos.y} }, { dirs[0] }); // No path found, stay still;
            }

            // Create a copy of the maze with the ghosts positions and set them as GHOSTs;
            std::vector<std::vector<int>> mazeWithGhosts = maze;
            for (int i = 0; i < ghostsPositionsInMap.size(); i ++) { if(i != index) mazeWithGhosts[ghostsPositionsInMap[i].x][ghostsPositionsInMap[i].y] = WALL; }

            // Get the shortest path;
            ShortestPath shortestPath = shortestPathLeeAlgorithm(mazeWithGhosts, currentPos, targetPos);

            // Convert the path to world coordinates;
            for (const auto& pos : shortestPath.path) { shortestPath.pathInWorld.push_back(toWorldCoordinates(pos)); }


            // Convert the directions;
            for (auto& dir : shortestPath.directions) { dir = glm::ivec2(dir.x, - dir.y); }

            return shortestPath;
        }

        // Convert world coordinates to grid coordinates;
        glm::ivec2 toGridCoordinates(const glm::vec3& worldPos) {
            return glm::ivec2(
                floor(worldPos.x) + maze.size() / 2,
                floor( - worldPos.z) + maze[0].size() / 2
            );
        }

        // Convert grid coordinates to world coordinates;
        glm::vec3 toWorldCoordinates(const glm::ivec2& gridPos) {
           return glm::vec3(
               static_cast<float>(gridPos.x) - (maze.size() / 2) + 0.5f,
                0.6f,
               - (static_cast<float>(gridPos.y) - (maze[0].size() / 2) + 0.5f)
            );
        }

        // Print maze with path inside;
        void printMazeWithPath(std::vector<std::vector<int>> maze, const std::vector<glm::ivec2> path, int index) {
            size_t offsetX = 0; // maze.size() / 2;
            size_t offsetY = 0; // maze[0].size() / 2;

            std::string coloredPath;
            switch (index) {
                case 0: coloredPath = "\033[31mX\033[0m "; break;
                case 1: coloredPath = "\033[38;5;201mX\033[0m "; break;
                case 2: coloredPath = "\033[36mX\033[0m "; break;
                case 3: coloredPath = "\033[93mX\033[0m "; break;
            }

            // Mark the path in the maze;
            for (const glm::ivec2& p : path) { maze[p.x + offsetX][p.y + offsetY] = PATH; }

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
                    case PATH: std::cout << coloredPath; break;
                    }
                }
                std::cout << std::endl;
            }
        }

        // Print matrix;
        void printMatrix(glm::mat4 matrix) { for (int i = 0; i < 4; i++) { for (int j = 0; j < 4; j++) { std::cout << matrix[i][j] << " "; } std::cout << std::endl; } std::cout << std::endl; }

};


// Subclasses for different ghost personalities;

class ChaserGhost : public Ghost {

    public:

        ChaserGhost(std::string ghostName, glm::vec3 startColor, std::vector<std::vector<int>> maze, glm::vec3 startPosition, glm::vec3 scatterPosition, float startSpeed, float speedModifier, float modeDuration)
            : Ghost(ghostName, startColor, maze, startPosition, scatterPosition, startSpeed, speedModifier, modeDuration) { }

        void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) override { targetPosition = pacmanPosition; }

};

class AmbusherGhost : public Ghost {

    protected:
        int ambushDistance;

    public:

        AmbusherGhost(std::string ghostName, glm::vec3 startColor, std::vector<std::vector<int>> maze, glm::vec3 startPosition, glm::vec3 scatterPosition, float startSpeed, float speedModifier, float modeDuration, int ambushDistance)
            : Ghost(ghostName, startColor, maze, startPosition, scatterPosition, startSpeed, speedModifier, modeDuration), ambushDistance(ambushDistance) { }

        void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) override {

            targetPosition = pacmanPosition; return;

            // TODO, ambusher behaviour;
        }

};

class ProtectorGhost : public Ghost {

    public:

        ProtectorGhost(std::string ghostName, glm::vec3 startColor, std::vector<std::vector<int>> maze, glm::vec3 startPosition, glm::vec3 scatterPosition, float startSpeed, float speedModifier, float modeDuration)
            : Ghost(ghostName, startColor, maze, startPosition, scatterPosition, startSpeed, speedModifier, modeDuration) { }

        void setTargetPosition(glm::vec3 pacmanPosition, glm::ivec2 pacmanDirection) override {
            if (distanceFromPacman > 8) { targetPosition = initialPosition; }
            else { targetPosition = pacmanPosition; }
        }

};

class GhostCollection {

    public:

        float speedModifier = 4.0f;
        float modeDuration = 6.0f;
        GhostState generalGhostState = NORMAL;

        std::shared_ptr<ChaserGhost> blinky;
        std::shared_ptr<AmbusherGhost> pinky;
        std::shared_ptr<AmbusherGhost> inky;
        std::shared_ptr<ProtectorGhost> clyde;

        std::vector<glm::ivec2> ghostsPositionsInMap;

        GhostCollection(std::vector<std::vector<int>> maze) :
            blinky(std::make_shared<ChaserGhost>
                ("Blinky", glm::vec3(1.0f, 0.0f, 0.0f), maze, glm::vec3(-1.5f, ghostsHeight, 0.0f), glm::vec3(-13.5f, ghostsHeight, 12.5f), 1.0f, 0.0f, modeDuration)),
            pinky(std::make_shared<AmbusherGhost>
                ("Pinky", glm::vec3(1.0f, 0.7f, 0.9f), maze, glm::vec3(0.0f, ghostsHeight, 2.0f), glm::vec3(-13.5f, ghostsHeight, -12.5f), 1.0f, 0.0f, modeDuration, 4)),
            inky(std::make_shared<AmbusherGhost>
                ("Inky", glm::vec3(0.5f, 0.96f, 1.0f), maze, glm::vec3(0.0f, ghostsHeight, 0.0f), glm::vec3(13.5f, ghostsHeight, 12.5f), 1.0f, 0.0f, modeDuration, 2)),
            clyde(std::make_shared<ProtectorGhost>
                ("Clyde", glm::vec3(0.91f, 0.7f, 0.0f), maze, glm::vec3(0.0f, ghostsHeight, -2.0f), glm::vec3(13.5f, ghostsHeight, -12.5f), 1.0f, 0.0f, modeDuration)) {

            ghostsPositionsInMap = { blinky->getCurrentPositionInMap(), pinky->getCurrentPositionInMap(), inky->getCurrentPositionInMap(), clyde->getCurrentPositionInMap() };
        }

        void moveAllGhosts(float deltaTime, glm::vec3 playerPosition, glm::vec3 playerDirection) {
            if(!blinky->didGhostGotEaten()) blinky->move(deltaTime, 0, ghostsPositionsInMap, playerPosition); ghostsPositionsInMap[0] = blinky->getCurrentPositionInMap();
            if(!pinky->didGhostGotEaten()) pinky->move(deltaTime, 1, ghostsPositionsInMap, playerPosition,  playerDirection); ghostsPositionsInMap[1] = pinky->getCurrentPositionInMap();
            if(!inky->didGhostGotEaten()) inky->move(deltaTime, 2, ghostsPositionsInMap, playerPosition, playerDirection); ghostsPositionsInMap[2] = inky->getCurrentPositionInMap();
            if(!clyde->didGhostGotEaten()) clyde->move(deltaTime, 3, ghostsPositionsInMap, playerPosition); ghostsPositionsInMap[3] = clyde->getCurrentPositionInMap();
        }

        void changeGhostsState(GhostState state) {
            generalGhostState = state;
			if(!blinky->didGhostGotEaten()) blinky->setGhostState(state);
            if(!pinky->didGhostGotEaten()) pinky->setGhostState(state);
            if(!inky->didGhostGotEaten()) inky->setGhostState(state);
            if(!clyde->didGhostGotEaten()) clyde->setGhostState(state);
		}

        void changeGhostsSpeedModifier(float speedMod) {
            if(!blinky->didGhostGotEaten()) blinky->setSpeedModifier(speedMod);
            if(!pinky->didGhostGotEaten()) pinky->setSpeedModifier(speedMod);
            if(!inky->didGhostGotEaten()) inky->setSpeedModifier(speedMod);
            if(!clyde->didGhostGotEaten()) clyde->setSpeedModifier(speedMod);
        }

        void resetAfterEatingPacman(int time) {
            blinky->setGhostGotPacman(false);
			pinky->setGhostGotPacman(false);
			inky->setGhostGotPacman(false);
			clyde->setGhostGotPacman(false);

            blinky->ghostGotEatenBehaviour(time);
            pinky->ghostGotEatenBehaviour(time);
            inky->ghostGotEatenBehaviour(time);
            clyde->ghostGotEatenBehaviour(time);
        }

        bool checkIfGhostsGotPacman() { return blinky->didGhostGotPacman() || pinky->didGhostGotPacman() || inky->didGhostGotPacman() || clyde->didGhostGotPacman(); }
};

#endif