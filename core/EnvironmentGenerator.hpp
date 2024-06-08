
#ifndef MAZE_GENERATOR_HPP
#define MAZE_GENERATOR_HPP

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
    PATH // Cell is a path;
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

            printMaze();
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
        void generateRandomMaze() {

        }

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

            for (uint32_t x = 0; x < maze.size(); x++) {
                for (uint32_t y = 0; y < maze[0].size(); y++) {

                    x_coord = x * wallSize - mazeCenterX;
                    y_coord = y * wallSize - mazeCenterY;

                    // Only if it is a wall;
                    if (maze[x][y] == WALL) {

                        // Create 8 vertices for the top and bottom of the wall;
                        std::vector<Vertex> wallVertices = {
                            // Bottom face;
                            { { x_coord, 0, y_coord }, color, glm::vec2(0.0f, 0.0f) },
                            { { x_coord + 1, 0, y_coord }, color, glm::vec2(0.25f, 0.0f) },
                            { { x_coord + 1, 0, y_coord + 1 }, color, glm::vec2(0.5f, 0.0f) },
                            { { x_coord, 0, y_coord + 1 }, color, glm::vec2(0.75f, 0.0f) },
                            // Top face;
                            { { x_coord, 2, y_coord }, color, glm::vec2(0.0f, 1.0f) },
                            { { x_coord + 1, 2, y_coord }, color, glm::vec2(0.25f, 1.0f) },
                            { { x_coord + 1, 2, y_coord + 1 }, color, glm::vec2(0.5f, 1.0f) },
                            { { x_coord, 2, y_coord + 1 }, color, glm::vec2(0.75f, 1.0f) }
                        };

                        // Add the vertices to the mazeVertices vector;
                        for (const auto& vertex : wallVertices) { mazeVertices.push_back(vertex); }

                        // Get the index of the first vertex of this wall;
                        uint32_t startIndex = static_cast<uint32_t>(mazeVertices.size() - 8);

                        // Define the indices for the faces;
                        std::vector<uint32_t> wallIndices = {
                            // Bottom face;
                            startIndex, startIndex + 1, startIndex + 2, startIndex, startIndex + 2, startIndex + 3,
                            // Top face;
                            startIndex + 4, startIndex + 5, startIndex + 6, startIndex + 4, startIndex + 6, startIndex + 7,
                            // Front face;
                            startIndex, startIndex + 1, startIndex + 5, startIndex, startIndex + 5, startIndex + 4,
                            // Back face;
                            startIndex + 2, startIndex + 3, startIndex + 7, startIndex + 2, startIndex + 7, startIndex + 6,
                            // Left face;
                            startIndex + 3, startIndex + 0, startIndex + 4, startIndex + 3, startIndex + 4, startIndex + 7,
                            // Right face;
                            startIndex + 1, startIndex + 2, startIndex + 6, startIndex + 1, startIndex + 6, startIndex + 5
                        };

                        // Add the indices to the mazeIndices vector;
                        for (const auto& index : wallIndices) { mazeIndices.push_back(index); }
                    }
                }
            }
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

        SkyGenerator(float radius = 24.0f, float maxHeight = -24.0f, int numLatSegments = 32, int numLonSegments = 64)
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

                    // Compute texture coordinates;
                    float u = static_cast<float>(lon) / numLatSegments;
                    float v = static_cast<float>(lat) / numLonSegments;

                    skyVertices.push_back(Vertex{ {x, y, z}, color, {u, v} });
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
        
        std::vector<Vertex> floorVertices;
        std::vector<uint32_t> floorIndices;

        FloorGenerator(float sideLength = 50.0f, int segments = 50, int textureRepeatCount = 50) : floorSide(sideLength), numOfSegments(segments), textureRepeatCount(textureRepeatCount) { generateFloorMesh(); }

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

                    floorVertices.push_back(Vertex{ {xPos, 0.0f, zPos}, color, {u, v} });
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


// Normal pellet generator class for pellets;
class PelletGenerator {

    public:
        bool isPowerPellet;
        glm::vec3 position;
        float pelletDiameter;
        bool eaten;
        float points;

        std::vector<Vertex> pelletVertices;
        std::vector<uint32_t> pelletIndices;

        PelletGenerator(glm::vec3 position, bool isPowerPellet = false, float pelletDiameter = 0.2f, float points = 25.0f)
            : position(position), isPowerPellet(isPowerPellet), pelletDiameter(pelletDiameter), eaten(false), points(points) { generatePelletMesh(); }

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

                    // Compute texture coordinates;
                    float u = static_cast<float>(lon) / numLonSegments;
                    float v = static_cast<float>(lat) / numLatSegments;

                    pelletVertices.push_back(Vertex{ { x, y, z }, color, { u, v } });
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


// Class that holds all meshes classes generators that "handly" generate vertices and indices;
class EnvironmentGenerator {

    public:

        MazeGenerator mazeGenerator;
        FloorGenerator floorGenerator;
        SkyGenerator skyGenerator;

};


#endif