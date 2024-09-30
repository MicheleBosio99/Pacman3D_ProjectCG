
#ifndef GAME_ENV_GENERATOR_HPP
#define GAME_ENV_GENERATOR_HPP

#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <iostream>
#include <random>
#include <ctime>
#include <array>
#include <glm/glm.hpp>
#include <stdio.h>

struct Vertex;

const glm::vec3 color = glm::vec3(1.0f); // Color is generic for every mesh since there will be then a texture applied on;
const float PI = 3.14159265359f;


// Defines an enum for the content of the maze;
enum CellContent {
    EMPTY, // Empty cell;
    WALL, // Cell with a wall;
    PELLET, // Contains a pellet;
    POWER_PELLET, // Contains a power pellet (the ones pacman eats to defeat ghosts);
    GHOSTS_HUB, // Cell is one of the ghosts hub, the ones the ghosts came out from;
    TELEPORT_HORIZONTAL, // Cell on the map limit that teleports pacman to the other side of the maze horizontally;
    TELEPORT_VERTICAL, // Cell on the map limit that teleports pacman to the other side of the maze vertically;
    PATH, // Cell is a path;
    GHOST
};

// Maze generator class creates the mesh of the maze, either by loading the maze as .txt or by generating one random;
class MazeGenerator {

    public:

        std::vector<std::vector<int>> maze; // Maze as a vector of vectors of integers, containing only 0s and 1s, 1s are walls;
        std::vector<Vertex> mazeVertices; // Vertex composing the maze;
        std::vector<uint32_t> mazeIndices; // Indices connecting the vertices of the maze;

        // MazeGenerator constructor;
        MazeGenerator(bool generateNewMaze = false, std::string filename = "resources/PacmanModifiedMaze.txt") {

            if (!generateNewMaze) { loadMazeFromFile(filename); }
            else { generateRandomMaze(); }

            // printMaze();
            generateMazeMesh();
        }
        
        // Getters;
        std::vector<std::vector<int>> getMaze() { return maze; }
        std::vector<Vertex> getMazeVertices() { return mazeVertices; }
        std::vector<uint32_t> getMazeIndices() { return mazeIndices; }

        // Maze print on console;
        void printMaze() {
            for (const auto& row : maze) {
                for (int cell : row) {
                    switch(cell) {
                        case EMPTY: std::cout << "  "; break;
                        case WALL: std::cout << char(254) << " "; break;
                        case PELLET: std::cout << char(167) << " "; break;
                        case POWER_PELLET: std::cout << char(155) << " "; break;
                        case GHOSTS_HUB: std::cout << char(126) << " "; break;
                        case TELEPORT_HORIZONTAL: std::cout << char(196) << " "; break;
                        case TELEPORT_VERTICAL: std::cout << char(179) << " "; break;
                    }
                }
                std::cout << std::endl;
            }
        }


    private:

        // Load matrix from file, default parameter for filename is the path to the original Pacman maze structure;
        void loadMazeFromFile(std::string filename) {
            std::ifstream file(filename);

            if (!file.is_open()) { std::cerr << "Unable to open file: " << filename << std::endl; }

            std::string line;
            while (std::getline(file, line)) {
                std::vector<int> row;
                for (char c : line) { if (std::isdigit(c)) { row.push_back(c - '0'); } }
                if (!row.empty()) { maze.push_back(row); }
            }

            file.close();
        }

        // Generate a random maze. Still doesn't works...
        void generateRandomMaze() { }

        // Generate the maze mesh filling vertices and indices arrays;
        void generateMazeMesh() {

            mazeVertices.clear();
            mazeIndices.clear();

            // Define the color and texture coordinate for the vertices;
            float wallSize = 1.0f;
            float x_coord; float y_coord;

            // Calculate the center of the maze
            float mazeCenterX = (maze.size() / 2.0f) * wallSize;
            float mazeCenterY = (maze[0].size() / 2.0f) * wallSize;

            float wallScale = 0.85f; // Scale factor to make the walls smaller;

            for (uint32_t x = 0; x < maze.size(); x++) {
                for (uint32_t y = 0; y < maze[0].size(); y++) {

                    if (maze[x][y] == WALL) {

                        float negX = 1.0f - wallScale, negY = 1.0f - wallScale, posX = wallScale, posY = wallScale;
                        float cornerAdjX = 0.0f, cornerAdjY = 0.0f;

                        if (x > 0 && maze[x - 1][y] == WALL) { negX = 0.0f; } // Up wall;
                        if (y > 0 && maze[x][y - 1] == WALL) { negY = 0.0f; } // Right wall;
                        if (x < maze.size() - 1 && maze[x + 1][y] == WALL) { posX = 1.0f; } // Down wall;
                        if (y < maze[0].size() - 1 && maze[x][y + 1] == WALL) { posY = 1.0f; } // Left wall;

                        if (x == 0) { negX = 0.0f; } // Left limit;
                        else if (x == maze.size() - 1) { posX = 1.0f; } // Right limit;
                        if (y == 0) { negY = 0.0f; } // Up limit;
                        else if (y == maze[0].size() - 1) { posY = 1.0f; } // Down limit;

                        x_coord = x * wallSize - mazeCenterX;
                        y_coord = y * wallSize - mazeCenterY;

                        auto [p, corner] = isCorner(x, y);

                        if (corner) {
                            auto adjX = p.x, adjY = p.y;
                            std::vector<Vertex> wallVertices;

                            if (adjX > 0 && adjY > 0) {
                                // Bottom face;
                                wallVertices.push_back({ { x_coord + negX, 0.0f, y_coord + negY }, color, { 0.0f, -1.0f, 0.0f }, glm::vec2(0.0f, 1.0f), ENVIRONMENT_MAT });
                                wallVertices.push_back({ { x_coord + posX, 0.0f, y_coord + negY }, color, { 0.0f, -1.0f, 0.0f }, glm::vec2(1.0f, 1.0f), ENVIRONMENT_MAT });
                                wallVertices.push_back({ { x_coord + posX, 0.0f, y_coord + posY - wallScale }, color, { 0.0f, -1.0f, 0.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT });
                                wallVertices.push_back({ { x_coord + posX - wallScale, 0.0f, y_coord + posY - wallScale }, color, { 0.0f, -1.0f, 0.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT });
                                wallVertices.push_back({ { x_coord + posX - wallScale, 0.0f, y_coord + posY }, color, { 0.0f, -1.0f, 0.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT });
                                wallVertices.push_back({ { x_coord + negX, 0.0f, y_coord + posY }, color, { 0.0f, -1.0f, 0.0f }, glm::vec2(0.0f, 0.0f), ENVIRONMENT_MAT });
                                // Top face;
                                wallVertices.push_back({ { x_coord + negX, 2.0f, y_coord + negY }, color, { 0.0f, 1.0f, 0.0f }, glm::vec2(0.0f, 1.0f), ENVIRONMENT_MAT });
                                wallVertices.push_back({ { x_coord + posX, 2.0f, y_coord + negY }, color, { 0.0f, 1.0f, 0.0f }, glm::vec2(1.0f, 1.0f), ENVIRONMENT_MAT });
                                wallVertices.push_back({ { x_coord + posX, 2.0f, y_coord + posY - wallScale }, color, { 0.0f, 1.0f, 0.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT });
                                wallVertices.push_back({ { x_coord + posX - wallScale, 2.0f, y_coord + posY - wallScale }, color, { 0.0f, 1.0f, 0.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT });
                                wallVertices.push_back({ { x_coord + posX - wallScale, 2.0f, y_coord + posY }, color, { 0.0f, 1.0f, 0.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT });
                                wallVertices.push_back({ { x_coord + negX, 2.0f, y_coord + posY }, color, { 0.0f, 1.0f, 0.0f }, glm::vec2(0.0f, 0.0f), ENVIRONMENT_MAT });
                                


                                // Add the vertices to the mazeVertices vector;
                                for (const auto& vertex : wallVertices) { mazeVertices.push_back(vertex); }

                                // Get the index of the first vertex of this wall;
                                uint32_t startIndex = static_cast<uint32_t>(mazeVertices.size() - wallVertices.size());

                                std::vector<uint32_t> wallIndices = {
                                    // Bottom face;
                                    startIndex, startIndex + 1, startIndex + 2, startIndex, startIndex + 2, startIndex + 3,
                                    startIndex, startIndex + 3, startIndex + 4, startIndex, startIndex + 4, startIndex + 5,
                                    // Top face;
                                    startIndex + 6, startIndex + 7, startIndex + 8, startIndex + 6, startIndex + 7, startIndex + 9,
                                    startIndex + 6, startIndex + 9, startIndex + 10, startIndex + 6, startIndex + 10, startIndex + 11,
                                    // Front face;
                                    //startIndex + 8, startIndex + 9, startIndex + 10, startIndex + 8, startIndex + 10, startIndex + 11,
                                    //// Back face;
                                    //startIndex + 12, startIndex + 13, startIndex + 14, startIndex + 12, startIndex + 14, startIndex + 15,
                                    //// Right face;
                                    //startIndex + 16, startIndex + 17, startIndex + 18, startIndex + 16, startIndex + 18, startIndex + 19,
                                    //// Left face;
                                    //startIndex + 20, startIndex + 21, startIndex + 22, startIndex + 20, startIndex + 22, startIndex + 23
                                };

                                // Add the indices to the mazeIndices vector;
                                for (const auto& index : wallIndices) { mazeIndices.push_back(index); }
                            }

                            
                        } // Wrong;
                        else {
                            std::vector<Vertex> wallVertices = {
                                // Bottom face;
                                { { x_coord + negX, 0.0f, y_coord + negY }, color, { 0.0f, -1.0f, 0.0f }, glm::vec2(0.0f, 1.0f), ENVIRONMENT_MAT },
                                { { x_coord + posX, 0.0f, y_coord + negY }, color, { 0.0f, -1.0f, 0.0f }, glm::vec2(1.0f, 1.0f), ENVIRONMENT_MAT },
                                { { x_coord + posX, 0.0f, y_coord + posY }, color, { 0.0f, -1.0f, 0.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT },
                                { { x_coord + negX, 0.0f, y_coord + posY }, color, { 0.0f, -1.0f, 0.0f }, glm::vec2(0.0f, 0.0f), ENVIRONMENT_MAT },
                                // Top face;
                                { { x_coord + negX, 2.0f, y_coord + negY }, color, { 0.0f, 1.0f, 0.0f }, glm::vec2(0.0f, 1.0f), ENVIRONMENT_MAT },
                                { { x_coord + posX, 2.0f, y_coord + negY }, color, { 0.0f, 1.0f, 0.0f }, glm::vec2(1.0f, 1.0f), ENVIRONMENT_MAT },
                                { { x_coord + posX, 2.0f, y_coord + posY }, color, { 0.0f, 1.0f, 0.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT },
                                { { x_coord + negX, 2.0f, y_coord + posY }, color, { 0.0f, 1.0f, 0.0f }, glm::vec2(0.0f, 0.0f), ENVIRONMENT_MAT },
                                // Front face (positive x);
                                { { x_coord + posX, 0.0f, y_coord + negY }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(0.0f, 0.0f), ENVIRONMENT_MAT },
                                { { x_coord + posX, 0.0f, y_coord + posY }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT },
                                { { x_coord + posX, 2.0f, y_coord + posY }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(1.0f, 2.0f), ENVIRONMENT_MAT },
                                { { x_coord + posX, 2.0f, y_coord + negY }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(0.0f, 2.0f), ENVIRONMENT_MAT },
                                // Back face (negative x);
                                { { x_coord + negX, 0.0f, y_coord + negY }, color, { -1.0f, 0.0f, 0.0f }, glm::vec2(0.0f, 0.0f), ENVIRONMENT_MAT },
                                { { x_coord + negX, 0.0f, y_coord + posY }, color, { -1.0f, 0.0f, 0.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT },
                                { { x_coord + negX, 2.0f, y_coord + posY }, color, { -1.0f, 0.0f, 0.0f }, glm::vec2(1.0f, 2.0f), ENVIRONMENT_MAT },
                                { { x_coord + negX, 2.0f, y_coord + negY }, color, { -1.0f, 0.0f, 0.0f }, glm::vec2(0.0f, 2.0f), ENVIRONMENT_MAT },
                                // Right face (negative z);
                                { { x_coord + negX, 0.0f, y_coord + negY }, color, { 0.0f, 0.0f, -1.0f }, glm::vec2(0.0f, 0.0f), ENVIRONMENT_MAT },
                                { { x_coord + posX, 0.0f, y_coord + negY }, color, { 0.0f, 0.0f, -1.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT },
                                { { x_coord + posX, 2.0f, y_coord + negY }, color, { 0.0f, 0.0f, -1.0f }, glm::vec2(1.0f, 2.0f), ENVIRONMENT_MAT },
                                { { x_coord + negX, 2.0f, y_coord + negY }, color, { 0.0f, 0.0f, -1.0f }, glm::vec2(0.0f, 2.0f), ENVIRONMENT_MAT },
                                // Left face (positive z);
                                { { x_coord + negX, 0.0f, y_coord + posY }, color, { 0.0f, 0.0f, 1.0f }, glm::vec2(0.0f, 0.0f), ENVIRONMENT_MAT },
                                { { x_coord + posX, 0.0f, y_coord + posY }, color, { 0.0f, 0.0f, 1.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT },
                                { { x_coord + posX, 2.0f, y_coord + posY }, color, { 0.0f, 0.0f, 1.0f }, glm::vec2(1.0f, 2.0f), ENVIRONMENT_MAT },
                                { { x_coord + negX, 2.0f, y_coord + posY }, color, { 0.0f, 0.0f, 1.0f }, glm::vec2(0.0f, 2.0f), ENVIRONMENT_MAT }
                            };

                            // Add the vertices to the mazeVertices vector;
                            for (const auto& vertex : wallVertices) { mazeVertices.push_back(vertex); }

                            // Get the index of the first vertex of this wall;
                            uint32_t startIndex = static_cast<uint32_t>(mazeVertices.size() - wallVertices.size());

                            std::vector<uint32_t> wallIndices = {
                                // Bottom face;
                                startIndex, startIndex + 1, startIndex + 2, startIndex, startIndex + 2, startIndex + 3,
                                // Top face;
                                startIndex + 4, startIndex + 5, startIndex + 6, startIndex + 4, startIndex + 6, startIndex + 7,
                                // Front face;
                                startIndex + 8, startIndex + 9, startIndex + 10, startIndex + 8, startIndex + 10, startIndex + 11,
                                // Back face;
                                startIndex + 12, startIndex + 13, startIndex + 14, startIndex + 12, startIndex + 14, startIndex + 15,
                                // Right face;
                                startIndex + 16, startIndex + 17, startIndex + 18, startIndex + 16, startIndex + 18, startIndex + 19,
                                // Left face;
                                startIndex + 20, startIndex + 21, startIndex + 22, startIndex + 20, startIndex + 22, startIndex + 23
                            };

                            // Add the indices to the mazeIndices vector;
                            for (const auto& index : wallIndices) { mazeIndices.push_back(index); }
                        }

                        
                    }
                }
            }
        }

        // Check if the wall is a corner;
        std::tuple<glm::ivec2, bool> isCorner(int x, int y) {
            return std::make_tuple(glm::ivec2(0, 0), false);
            if (x > 0 && y > 0 && maze[x - 1][y - 1] == WALL) { return std::make_tuple(glm::vec2(-1, -1), true); }
            else if (x > 0 && y < maze[0].size() - 1 && maze[x - 1][y + 1] == WALL) { return std::make_tuple(glm::ivec2(-1, 1), true); }
            else if (x < maze.size() - 1 && y > 0 && maze[x + 1][y - 1] == WALL) { return std::make_tuple(glm::ivec2(1, -1), true); }
            else if (x < maze.size() - 1 && y < maze[0].size() - 1 && maze[x + 1][y + 1] == WALL) { return std::make_tuple(glm::ivec2(1, 1), true); }
            else { return std::make_tuple(glm::ivec2(0, 0), false); }
		}
};

// Sky generator class creates the sky mesh. The sky is a dome;gene
class SkyGenerator {

    public:

        float skyRadius;
        float maxHeight;
        int numLatSegments;
        int numLonSegments;

        std::vector<Vertex> skyVertices;
        std::vector<uint32_t> skyIndices;

        SkyGenerator(float radius = 50.0f, float maxHeight = -50.0f, int numLatSegments = 32, int numLonSegments = 64)
            : skyRadius(radius), maxHeight(maxHeight), numLatSegments(numLatSegments), numLonSegments(numLatSegments) { generateSkyMesh(); }

        std::vector<Vertex> geSkyVertices() { return skyVertices; }
        std::vector<uint32_t> getSkyIndices() { return skyIndices; }

    private:

        // Generate the mesh of a dome;
        void generateSkyMesh() {

            skyVertices.clear();
            skyIndices.clear();

            float maxTheta = asin(maxHeight / skyRadius);

            // Fill vertices vector;
            for (int lat = 0; lat <= numLatSegments; lat ++) {

                float theta = maxTheta * (static_cast<float>(lat) / numLatSegments);
                float sinTheta = sin(theta);
                float cosTheta = cos(theta);

                for (int lon = 0; lon <= numLonSegments; lon ++) {
                    float phi = 2.0f * PI * (static_cast<float>(lon) / numLonSegments);
                    float sinPhi = sin(phi);
                    float cosPhi = cos(phi);

                    // Compute position coordinates;
                    float x = skyRadius * cosPhi * sinTheta;
                    float y = skyRadius * cosTheta;
                    float z = skyRadius * sinPhi * sinTheta;

                    glm::vec3 normCoord = - glm::normalize(glm::vec3(x, y, z)); // Invert the normal vector to make it point inwards the sphere;

                    // Compute texture coordinates;
                    float u = static_cast<float>(lon) / numLatSegments;
                    float v = static_cast<float>(lat) / numLonSegments;

                    skyVertices.push_back(Vertex{ {x, y, z}, color, normCoord, {u, v}, SKY_MAT });
                }
            }

            // Fill indices vector;
            for (int lat = 0; lat < numLatSegments; lat ++) {
                for (int lon = 0; lon < numLonSegments; lon ++) {

                    int first = (lat * (numLonSegments + 1)) + lon;
                    int second = first + numLonSegments + 1;

                    skyIndices.push_back(first);
                    skyIndices.push_back(second);
                    skyIndices.push_back(first + 1);

                    skyIndices.push_back(second);
                    skyIndices.push_back(second + 1);
                    skyIndices.push_back(first + 1);
                }
            }
        }
};

// Floor generator class creates the floor mesh;
class FloorGenerator {

    public:

        float floorSide;
        int numOfSegments;
        int textureRepeatCount;
        float floorHeight = 0.0f;
        
        std::vector<Vertex> floorVertices;
        std::vector<uint32_t> floorIndices;

        FloorGenerator(float sideLength = 75.0f, int segments = 30, int textureRepeatCount = 30) : floorSide(sideLength), numOfSegments(segments), textureRepeatCount(textureRepeatCount) { generateFloorMesh(); }

        std::vector<Vertex> getFloorVertices() { return floorVertices; }
        std::vector<uint32_t> getFloorIndices() { return floorIndices; }

    private:

        void generateFloorMesh() {

            floorVertices.clear();
            floorIndices.clear();

            float halfLength = floorSide / 2.0f;
            float segmentSize = floorSide / numOfSegments;

            // Fill vertices vector;
            for (int z = 0; z <= numOfSegments; z++) {
                for (int x = 0; x <= numOfSegments; x++) {

                    float xPos = -halfLength + x * segmentSize;
                    float zPos = -halfLength + z * segmentSize;

                    // Modified texture coordinates for repetition;
                    float u = static_cast<float>(x) / numOfSegments * textureRepeatCount;
                    float v = static_cast<float>(z) / numOfSegments * textureRepeatCount;

                    floorVertices.push_back(Vertex{ {xPos, floorHeight, zPos}, color, { 0.0f, 1.0f, 0.0f }, { u, v }, ENVIRONMENT_MAT });
                }
            }

            // Fill indices vector;
            for (int z = 0; z < numOfSegments; z ++) {
                for (int x = 0; x < numOfSegments; x ++) {

                    int topLeft = (z * (numOfSegments + 1)) + x;
                    int topRight = topLeft + 1;
                    int bottomLeft = topLeft + (numOfSegments + 1);
                    int bottomRight = bottomLeft + 1;

                    floorIndices.push_back(topLeft);
                    floorIndices.push_back(bottomLeft);
                    floorIndices.push_back(topRight);

                    floorIndices.push_back(topRight);
                    floorIndices.push_back(bottomLeft);
                    floorIndices.push_back(bottomRight);
                }
            }
        }

};

// Teleporter generator class creates the mesh of the teleporter;
class TeleporterGenerator {

    public:

        float teleporterWidth = 1.3f;
        float teleporterHeight = 2.0f;
        float teleporterDepth = 0.1f;
        glm::vec3 teleporterPosition = glm::vec3(0.0f, 0.0f, 0.0f);

        std::vector<Vertex> teleporterVertices;
        std::vector<uint32_t> teleporterIndices;

        TeleporterGenerator() { generateTeleporterMesh(); }

        std::vector<Vertex> getTeleporterVertices() { return teleporterVertices; }
        std::vector<uint32_t> getTeleporterIndices() { return teleporterIndices; }

    private:

        void generateTeleporterMesh() {

            teleporterVertices.clear();
            teleporterIndices.clear();

            float halfWidth = teleporterWidth / 2.0f;
            float halfDepth = teleporterDepth / 2.0f;

            // Fill vertices vector;
            std::vector<Vertex> teleporterVerticesTemp = {
                { { teleporterPosition.x + halfWidth, teleporterPosition.y, teleporterPosition.z - teleporterDepth }, color, { 0.0f, 0.0f, 1.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT },
                { { teleporterPosition.x - halfWidth, teleporterPosition.y, teleporterPosition.z - teleporterDepth }, color, { 0.0f, 0.0f, 1.0f }, glm::vec2(0.0f, 0.0f), ENVIRONMENT_MAT },
                { { teleporterPosition.x + halfWidth, teleporterPosition.y + teleporterHeight, teleporterPosition.z - teleporterDepth }, color, { 0.0f, 0.0f, 1.0f }, glm::vec2(1.0f, 1.0f), ENVIRONMENT_MAT },
                { { teleporterPosition.x - halfWidth, teleporterPosition.y + teleporterHeight, teleporterPosition.z - teleporterDepth }, color, { 0.0f, 0.0f, 1.0f }, glm::vec2(0.0f, 1.0f), ENVIRONMENT_MAT },

                { { teleporterPosition.x + halfWidth, teleporterPosition.y, teleporterPosition.z }, color, { 0.0f, 0.0f, -1.0f }, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT },
                { { teleporterPosition.x - halfWidth, teleporterPosition.y, teleporterPosition.z }, color, { 0.0f, 0.0f, -1.0f }, glm::vec2(0.0f, 0.0f), ENVIRONMENT_MAT },
                { { teleporterPosition.x + halfWidth, teleporterPosition.y + teleporterHeight, teleporterPosition.z }, color, { 0.0f, 0.0f, -1.0f }, glm::vec2(1.0f, 1.0f), ENVIRONMENT_MAT },
                { { teleporterPosition.x - halfWidth, teleporterPosition.y + teleporterHeight, teleporterPosition.z }, color, { 0.0f, 0.0f, -1.0f }, glm::vec2(0.0f, 1.0f), ENVIRONMENT_MAT }
            };
            for (const auto& vertex : teleporterVerticesTemp) { teleporterVertices.push_back(vertex); }

            // Fill indices vector;
            std::vector<uint32_t> teleporterIndicesTemp = {
                0, 1, 2, 1, 2, 3,
                4, 5, 6, 5, 6, 7,
                0, 4, 2, 4, 2, 6,
                1, 5, 3, 5, 3, 7,
                0, 1, 4, 1, 4, 5,
                2, 3, 6, 3, 6, 7
            };
            for (const auto& index : teleporterIndicesTemp) { teleporterIndices.push_back(index); }
        }
};

// Gate generator class creates the mesh of the gate;
class ParallelepGenerator {

    public:

        float gateWidth;
        float gateHeight;
        float gateDepth;
        MATERIAL_TYPE type;

        std::vector<Vertex> gateVertices;
        std::vector<uint32_t> gateIndices;

        ParallelepGenerator(float gateWidth, float gateHeight, float gateDepth, MATERIAL_TYPE type = ENVIRONMENT_MAT)
            : gateWidth(gateWidth), gateHeight(gateHeight), gateDepth(gateDepth), type(type) { generateGateMesh(); }

        std::vector<Vertex> getGateVertices() { return gateVertices; }
        std::vector<uint32_t> getGateIndices() { return gateIndices; }

    private:

        void generateGateMesh() {
            gateVertices.clear();
            gateIndices.clear();

            glm::vec3 gatePosition(0.0f, 0.0f, 0.0f);
            float halfWidth = gateWidth / 2.0f;
            float halfDepth = gateDepth / 2.0f;

            std::vector<Vertex> vertices = {
                { { gatePosition.x - halfDepth, gatePosition.y, gatePosition.z + halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(0.0f, 0.0f), type},
                { { gatePosition.x - halfDepth, gatePosition.y, gatePosition.z - halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(1.0f, 0.0f), type },
                { { gatePosition.x - halfDepth, gatePosition.y + gateHeight, gatePosition.z + halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(0.0f, -1.0f), type },
                { { gatePosition.x - halfDepth, gatePosition.y + gateHeight, gatePosition.z - halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(1.0f, -1.0f), type },

                { { gatePosition.x + halfDepth, gatePosition.y, gatePosition.z + halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(0.0f, 0.0f), type},
                { { gatePosition.x + halfDepth, gatePosition.y, gatePosition.z - halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(1.0f, 0.0f), type },
                { { gatePosition.x + halfDepth, gatePosition.y + gateHeight, gatePosition.z + halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(0.0f, -1.0f), type },
                { { gatePosition.x + halfDepth, gatePosition.y + gateHeight, gatePosition.z - halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(1.0f, -1.0f), type }
            };

            for (const auto& vertex : vertices) { gateVertices.push_back(vertex); }

            std::vector<uint32_t> gateIndicesTemp = {
                0, 1, 2, 1, 2, 3,
                4, 5, 6, 5, 6, 7,
                0, 4, 2, 4, 2, 6,
                1, 5, 3, 5, 3, 7,
                0, 1, 4, 1, 4, 5,
                2, 3, 6, 3, 6, 7
            };
            for (const auto& index : gateIndicesTemp) { gateIndices.push_back(index); }
        }
};

// Pellet generator class for pellets;
class PelletGenerator {

    public:

        bool isPowerPellet;
        glm::vec3 position;
        float pelletDiameter;
        bool eaten;
        float points;
        MATERIAL_TYPE mat;

        std::vector<Vertex> pelletVertices;
        std::vector<uint32_t> pelletIndices;

        PelletGenerator(glm::vec3 position, bool isPowerPellet = false, float pelletDiameter = 0.2f, float points = 10.0f, MATERIAL_TYPE material = PELLET_MAT)
            : position(position), isPowerPellet(isPowerPellet), pelletDiameter(pelletDiameter), eaten(false), points(points), mat(material) { generatePelletMesh(); }

        std::vector<Vertex> getPelletVertices() { return pelletVertices; }
        std::vector<uint32_t> getPelletIndices() { return pelletIndices; }

    private:

        void generatePelletMesh() {
            pelletVertices.clear();
            pelletIndices.clear();

            int numLatSegments = 12;
            int numLonSegments = 18;
            float radius = pelletDiameter / 2.0f;

            // Fill vertices vector;
            for (int lat = 0; lat <= numLatSegments; ++lat) {
                float theta = PI * (static_cast<float>(lat) / numLatSegments);
                float sinTheta = sinf(theta);
                float cosTheta = cosf(theta);

                for (int lon = 0; lon <= numLonSegments; ++lon) {
                    float phi = 2.0f * PI * (static_cast<float>(lon) / numLonSegments);
                    float sinPhi = sinf(phi);
                    float cosPhi = cosf(phi);

                    // Compute position coordinates;
                    float x = radius * cosPhi * sinTheta;
                    float y = radius * cosTheta;
                    float z = radius * sinPhi * sinTheta;

                    glm::vec3 normCoord = glm::normalize(glm::vec3(x, y, z));

                    // Compute texture coordinates;
                    float u = static_cast<float>(lon) / numLonSegments;
                    float v = static_cast<float>(lat) / numLatSegments;

                    pelletVertices.push_back(Vertex{ { x, y, z }, color, normCoord, { u, v }, mat });
                }
            }

            // Fill indices vector;
            for (int lat = 0; lat < numLatSegments; ++lat) {
                for (int lon = 0; lon < numLonSegments; ++lon) {
                    int first = (lat * (numLonSegments + 1)) + lon;
                    int second = first + numLonSegments + 1;

                    pelletIndices.push_back(first);
                    pelletIndices.push_back(second);
                    pelletIndices.push_back(first + 1);

                    pelletIndices.push_back(second);
                    pelletIndices.push_back(second + 1);
                    pelletIndices.push_back(first + 1);
                }
            }
        }
};


// Class that holds all meshes classes generators that generate vertices and indices;
class GameEnvGenerator {

    public:

        MazeGenerator mazeGenerator;
        FloorGenerator floorGenerator;
        SkyGenerator skyGenerator;
        TeleporterGenerator teleporterGenerator;
        ParallelepGenerator gateGenerator = ParallelepGenerator(2.3f, 1.5f, 0.15f);

};


#endif