
#ifndef GHOST_BEHAVIOUR_HPP
#define GHOST_BEHAVIOUR_HPP

#include <iostream>
#include <vector>
#include <glm/glm.hpp>
#include <queue>
#include <unordered_set>


// Direction the ghosts can take to reach target position;
enum GhostDirection {
    UP,
    DOWN,
    RIGHT,
    LEFT,
    STOP
};

// Define constants for ghost states;
enum GhostState {
    NORMAL,
    SCATTER,
    FRIGHTENED
};


// Define a class for the common properties of all ghosts;
class Ghost {

    protected:

        std::string name;
        glm::vec3 color;

        float currentSpeed;
        float speedModifier;
        float sizeModifier;

        glm::vec3 startingPosition;
        glm::vec3 currentPosition;
        glm::vec3 targetPosition;

        // Coordinates that come from the game of the player and the ghost are in continuous floats, however to compute shortwst path on map we need their discrete position;
        glm::ivec2 currentPositionInMap;
        glm::ivec2 targetPositionInMap;

        GhostState state;
        // float modeDuration; // Do not know if it is useful;
        

    public:

        Ghost(std::string ghostName, glm::vec3 startColor, float startSpeed, float startSizeModifier, glm::vec3 startPosition) : speedModifier(1.0f) {
            name = ghostName;
            color = startColor;

            currentSpeed = startSpeed;
            startSizeModifier = sizeModifier;

            startingPosition = startPosition;
            currentPosition = startPosition;

            currentPositionInMap = convertToDiscreteCoordinates(currentPosition);


            state = NORMAL;
        }

        virtual void setTargetPosition(std::vector<std::vector<int>> map, glm::vec3 playerPosition) = 0;

        void move(const std::vector<std::vector<int>> map, const glm::vec3 playerPosition) {
            
            setTargetPosition(map, playerPosition); // Set the target position based on the ghost's behavior;

            // Convert to discrete coordinates;
            currentPositionInMap = convertToDiscreteCoordinates(currentPosition);
            targetPositionInMap = convertToDiscreteCoordinates(targetPosition);

            // Compute the shortest path to the target position;
            // GhostDirection dir = leeAlgorithm(map, currentPositionInMap, targetPositionInMap);

            // Move towards the target position;

        }

        // Convert currentPosition or targetPosition into discrete coordinates. Cannot be done now since I have no idea how the maze will be positioned in the world coordinates;
        glm::ivec2 convertToDiscreteCoordinates(glm::vec3 continuousCoordinates) {
            glm::ivec2 discreteCoordinates = glm::ivec2(1, 1);

            // TODO: transform;

            return discreteCoordinates;
        }



        // Setter and getter for parameters needed;

        std::string getName() const { return name; }

        glm::vec3 getColor() const { return color; }

        void setColor(const glm::vec3& newColor) { color = newColor; }

        float getCurrentSpeed() const { return currentSpeed; }

        void setCurrentSpeed(float speed) {  currentSpeed = speed; }

        float getSpeedModifier() const { return speedModifier; }

        void setSpeedModifier(float modifier) { speedModifier = modifier; }

        glm::vec3 getStartingPosition() const { return startingPosition; }

        void setStartingPosition(const glm::vec3& position) { startingPosition = position; }

        glm::vec3 getCurrentPosition() const { return currentPosition; }

        void setCurrentPosition(const glm::vec3& position) { currentPosition = position; }

        glm::vec3 getTargetPosition() const { return targetPosition; }

        void setTargetPosition(const glm::vec3& position) { targetPosition = position; }

        GhostState getState() const { return state; }

        void setState(GhostState newState) { state = newState; }

        float getSize() const { return sizeModifier; }



    private :

        // Function to check if a cell is valid (within the map boundaries and not a wall);
        bool isValidCell(std::vector<std::vector<int>> map, int row, int col) {
            return row >= 0 && row < static_cast<int>(map.size()) && col >= 0 && col < static_cast<int>(map[0].size()) && map[row][col] != 1;
        }

        // Lee algorithm to find the shortest path between the current position and the target one;
        GhostDirection leeAlgorithm(std::vector<std::vector<int>> map, glm::ivec2 startPos, glm::ivec2 targetPos) {
            GhostDirection moveInDirection = STOP;
        }
};

// Define subclasses for different ghost personalities;

class ChaserGhost : public Ghost {

    public:

        ChaserGhost(std::string ghostName, glm::vec3 startColor, float startSpeed, float startSize, glm::vec3 startPosition) : Ghost(ghostName, startColor, startSpeed, startSize, startPosition) { }

        void setTargetPosition(std::vector<std::vector<int>> map, glm::vec3 playerPosition) override {
            std::cout << "I am chaser";
            targetPosition = playerPosition;
        }
};

class AmbusherGhost : public Ghost {

    public:

        AmbusherGhost(std::string ghostName, glm::vec3 startColor, float startSpeed, float startSize, glm::vec3 startPosition) : Ghost(ghostName, startColor, startSpeed, startSize, startPosition) { }

        void setTargetPosition(const std::vector<std::vector<int>> map, const glm::vec3 playerPosition) override {
            std::cout << "I am ambusher";
            targetPosition = glm::vec3(); // ?
        }
};

class FleerGhost : public Ghost {

    public:

        FleerGhost(std::string ghostName, glm::vec3 startColor, float startSpeed, float startSize, glm::vec3 startPosition) : Ghost(ghostName, startColor, startSpeed, startSize, startPosition) { }

        void setTargetPosition(const std::vector<std::vector<int>> map, const glm::vec3 playerPosition) override {
            std::cout << "I am fleer";
            targetPosition = glm::vec3(); // ?
        }
};


#endif