
#ifndef START_MENU_GENERATOR_HPP
#define START_MENU_GENERATOR_HPP

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

class BillBoardGenerator {

    public:

        float billboardWidth = 8.0f;
        float billboardHeight = 2.33f;

        std::vector<Vertex> billboardVertices;
        std::vector<uint32_t> billboardIndices;
        glm::vec3 position;

        BillBoardGenerator(float billboardWidth = 7.65f, float billboardHeight = 2.0f, glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f))
            : billboardWidth(billboardWidth), billboardHeight(billboardHeight), position(position) { generateBillboardMesh(); }

        std::vector<Vertex> getBillboardVertices() { return billboardVertices; }
        std::vector<uint32_t> getBillboardIndices() { return billboardIndices; }

    private:

        void generateBillboardMesh() {

            billboardVertices.clear();
            billboardIndices.clear();

            float halfWidth = billboardWidth / 2.0f;
            float halfHeight = billboardHeight / 2.0f;

            // Fill vertices vector;
            std::vector<Vertex> billboardVerticesTemp = {
                { { position.x, position.y - halfHeight, position.z + halfWidth }, color, glm::vec2(0.0f, 1.0f), ENVIRONMENT_MAT },
                { { position.x, position.y - halfHeight, position.z - halfWidth }, color, glm::vec2(1.0f, 1.0f), ENVIRONMENT_MAT },
                { { position.x, position.y + halfHeight, position.z + halfWidth }, color, glm::vec2(0.0f, 0.0f), ENVIRONMENT_MAT },
                { { position.x, position.y + halfHeight, position.z - halfWidth }, color, glm::vec2(1.0f, 0.0f), ENVIRONMENT_MAT }
            };
            for (const auto& vertex : billboardVerticesTemp) { billboardVertices.push_back(vertex); }

            // Fill indices vector;
            std::vector<uint32_t> billboardIndicesTemp = { 0, 1, 2, 1, 2, 3 };
            for (const auto& index : billboardIndicesTemp) { billboardIndices.push_back(index); }
        }
};

// Class to generate;
class StartingMenuEnvGenerator {

    public:

        BillBoardGenerator titleGenerator = BillBoardGenerator();
        BillBoardGenerator spacebarGenerator = BillBoardGenerator(5.0f, 0.577f, glm::vec3(0.0f, -2.0f, 0.0f));

};

#endif