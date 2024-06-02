﻿
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

const uint32_t MAZE_HEIGHT = 31;
const uint32_t MAZE_WIDTH = 28;


// Defines an enum for the content of the maze;
enum CellContent {
    EMPTY, // Empty cell;
    WALL, // Cell with a wall;
    PELLET, // Contains a pellet;
    POWER_PELLET, // Contains a power pellet (the ones pacman eats to defeat ghosts);
    GHOSTS_HUB, // Cell is one of the ghosts hub, the ones the ghosts came out from;
    TELEPORT_HORIZONTAL, // Cell on the map limit that teleports pacman to the other side of the maze horizontally;
    TELEPORT_VERTICAL // Cell on the map limit that teleports pacman to the other side of the maze vertically;
};


class MazeGenerator {


    public:

        std::vector<std::vector<int>> maze; // Maze as a vector of vectors of integers, containing only 0s and 1s, 1s are walls;
        std::vector<Vertex> mazeVertices; // Vertex composing the maze;
        std::vector<uint32_t> mazeIndices; // Indices connecting the vertices of the maze;

        // MazeGenerator constructor;
        MazeGenerator(bool generateNewMaze = false, std::string filename = "resources/PacmanOriginalMaze.txt") {

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
            glm::vec3 color(1.0f, 1.0f, 1.0f); // White color;
            float wallSize = 1.0f;
            float x_coord; float y_coord;

            for (uint32_t x = 0; x < MAZE_HEIGHT; x ++) {
                for (uint32_t y = 0; y < MAZE_WIDTH; y ++) {

                    x_coord = x * wallSize;
                    y_coord = y * wallSize;

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


#endif