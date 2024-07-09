
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

        BillBoardGenerator(float billboardWidth = 7.65f, float billboardHeight = 2.0f, glm::vec3 position = glm::vec3(0.0f, 1.2f, 0.0f))
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


class BillBoardRepeatedGenerator : public BillBoardGenerator {

    public:

		int repeatCount;

		BillBoardRepeatedGenerator(float billboardWidth = 7.65f, float billboardHeight = 2.0f, glm::vec3 position = glm::vec3(0.0f, 1.2f, 0.0f), int repeatCount = 1)
			: BillBoardGenerator(billboardWidth, billboardHeight, position), repeatCount(repeatCount) { generateBillboardMesh(); }

	private:

        void generateBillboardMesh() {

            billboardVertices.clear();
            billboardIndices.clear();

            float halfWidth = billboardWidth / 2.0f;
            float halfHeight = billboardHeight / 2.0f;

            // Fill vertices vector;
            std::vector<Vertex> billboardVerticesTemp = {
                { { position.x, position.y - halfHeight, position.z + halfWidth }, color, glm::vec2(0.0f, repeatCount), ENVIRONMENT_MAT },
                { { position.x, position.y - halfHeight, position.z - halfWidth }, color, glm::vec2(repeatCount, repeatCount), ENVIRONMENT_MAT },
                { { position.x, position.y + halfHeight, position.z + halfWidth }, color, glm::vec2(0.0f, 0.0f), ENVIRONMENT_MAT },
                { { position.x, position.y + halfHeight, position.z - halfWidth }, color, glm::vec2(repeatCount, 0.0f), ENVIRONMENT_MAT }
            };
            for (const auto& vertex : billboardVerticesTemp) { billboardVertices.push_back(vertex); }

            // Fill indices vector;
            std::vector<uint32_t> billboardIndicesTemp = { 0, 1, 2, 1, 2, 3 };
            for (const auto& index : billboardIndicesTemp) { billboardIndices.push_back(index); }
        }
};


class MenuFloorGenerator {

    public:

        float floorSide;
        int numOfSegments;
        int textureRepeatCount;
        float floorHeight = 0.0f;

        std::vector<Vertex> floorVertices;
        std::vector<uint32_t> floorIndices;

        MenuFloorGenerator(float floorHeight = 0.0f, float sideLength = 50.0f, int segments = 50, int textureRepeatCount = 50)
            : floorSide(sideLength), numOfSegments(segments), textureRepeatCount(textureRepeatCount) { generateFloorMesh(); }

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

                    floorVertices.push_back(Vertex{ {xPos, floorHeight, zPos}, color, {u, v}, ENVIRONMENT_MAT });
                }
            }

            // Fill indices vector;
            for (int z = 0; z < numOfSegments; z++) {
                for (int x = 0; x < numOfSegments; x++) {

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


// Class to generate;
class StartingMenuEnvGenerator {

    public:

        BillBoardGenerator titleGenerator = BillBoardGenerator();
        BillBoardGenerator spacebarGenerator = BillBoardGenerator(5.0f, 0.577f, glm::vec3(0.0f, -0.8f, 0.0f));
        MenuFloorGenerator floorGenerator = MenuFloorGenerator(0.0f, 20.0f);

};

class GameOverEnvGenerator {

	public:

        BillBoardRepeatedGenerator gameOverWallpaperGenerator = BillBoardRepeatedGenerator(32.0f, 32.0f, glm::vec3(-2.0f, 0.0f, 0.0f), 64);
		BillBoardGenerator gameOverWriteGenerator = BillBoardGenerator(5.0f, 5.0f, glm::vec3(0.0f, 0.0f, 0.0f));

};

#endif