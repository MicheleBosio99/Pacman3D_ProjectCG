
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
        MATERIAL_TYPE type;

        BillBoardGenerator(float billboardWidth = 7.65f, float billboardHeight = 2.0f, glm::vec3 position = glm::vec3(0.0f, 1.2f, 0.0f), MATERIAL_TYPE type = HUD_MAT)
            : billboardWidth(billboardWidth), billboardHeight(billboardHeight), position(position), type(type) { generateBillboardMesh(); }

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
                { { position.x, position.y - halfHeight, position.z + halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(0.0f, 1.0f), type},
                { { position.x, position.y - halfHeight, position.z - halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(1.0f, 1.0f), type },
                { { position.x, position.y + halfHeight, position.z + halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(0.0f, 0.0f), type },
                { { position.x, position.y + halfHeight, position.z - halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(1.0f, 0.0f), type }
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

		BillBoardRepeatedGenerator(float billboardWidth = 7.65f, float billboardHeight = 2.0f, glm::vec3 position = glm::vec3(0.0f, 1.2f, 0.0f),
            int repeatCount = 1, MATERIAL_TYPE type = HUD_MAT)
			: BillBoardGenerator(billboardWidth, billboardHeight, position, type), repeatCount(repeatCount) { generateBillboardMesh(); }

	private:

        void generateBillboardMesh() {

            billboardVertices.clear();
            billboardIndices.clear();

            float halfWidth = billboardWidth / 2.0f;
            float halfHeight = billboardHeight / 2.0f;

            // Fill vertices vector;
            std::vector<Vertex> billboardVerticesTemp = {
                { { position.x, position.y - halfHeight, position.z + halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(0.0f, repeatCount), type },
                { { position.x, position.y - halfHeight, position.z - halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(repeatCount, repeatCount), type },
                { { position.x, position.y + halfHeight, position.z + halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(0.0f, 0.0f), type },
                { { position.x, position.y + halfHeight, position.z - halfWidth }, color, { 1.0f, 0.0f, 0.0f }, glm::vec2(repeatCount, 0.0f), type }
            };
            for (const auto& vertex : billboardVerticesTemp) { billboardVertices.push_back(vertex); }

            // Fill indices vector;
            std::vector<uint32_t> billboardIndicesTemp = { 0, 1, 2, 1, 2, 3 };
            for (const auto& index : billboardIndicesTemp) { billboardIndices.push_back(index); }
        }
};


class MenuFloorGenerator {

public:

    float floorLengthX;
    float floorLengthZ;
    int numOfSegmentsX;
    int numOfSegmentsZ;
    int textureRepeatCountX;
    int textureRepeatCountZ;
    float floorHeight = 0.0f;
    MATERIAL_TYPE type;

    std::vector<Vertex> floorVertices;
    std::vector<uint32_t> floorIndices;

    MenuFloorGenerator(float floorHeight = 0.0f, float lengthX = 50.0f, float lengthZ = 50.0f, int segmentsX = 50, int segmentsZ = 50, int textureRepeatCountX = 50,
        int textureRepeatCountZ = 50, MATERIAL_TYPE type = HUD_MAT) : floorLengthX(lengthX), floorLengthZ(lengthZ), numOfSegmentsX(segmentsX),
        numOfSegmentsZ(segmentsZ), textureRepeatCountX(textureRepeatCountX), textureRepeatCountZ(textureRepeatCountZ), type(type) { generateFloorMesh(); }

    std::vector<Vertex> getFloorVertices() { return floorVertices; }
    std::vector<uint32_t> getFloorIndices() { return floorIndices; }

private:

    void generateFloorMesh() {

        floorVertices.clear();
        floorIndices.clear();

        float halfLengthX = floorLengthX / 2.0f;
        float halfLengthZ = floorLengthZ / 2.0f;
        float segmentSizeX = floorLengthX / numOfSegmentsX;
        float segmentSizeZ = floorLengthZ / numOfSegmentsZ;

        // Fill vertices vector;
        for (int z = 0; z <= numOfSegmentsZ; z++) {
            for (int x = 0; x <= numOfSegmentsX; x++) {

                float xPos = -halfLengthX + x * segmentSizeX;
                float zPos = -halfLengthZ + z * segmentSizeZ;

                // Modified texture coordinates for repetition;
                float u = static_cast<float>(x) / numOfSegmentsX * textureRepeatCountX;
                float v = static_cast<float>(z) / numOfSegmentsZ * textureRepeatCountZ;

                floorVertices.push_back(Vertex{ {xPos, floorHeight, zPos}, color, { 0.0f, 1.0f, 0.0f }, {u, v}, type });
            }
        }

        // Fill indices vector;
        for (int z = 0; z < numOfSegmentsZ; z++) {
            for (int x = 0; x < numOfSegmentsX; x++) {

                int topLeft = (z * (numOfSegmentsX + 1)) + x;
                int topRight = topLeft + 1;
                int bottomLeft = topLeft + (numOfSegmentsX + 1);
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



class StartingMenuEnvGenerator {

    public:

        BillBoardGenerator titleGenerator = BillBoardGenerator();
        BillBoardGenerator spacebarGenerator = BillBoardGenerator(5.0f, 0.577f, glm::vec3(0.0f, -0.8f, 0.0f));
        MenuFloorGenerator floorGenerator = MenuFloorGenerator(0.0f, 20.0f, 50.0f, 5, 10, 5, 10);

};


class GameOverEnvGenerator {

	public:

        BillBoardRepeatedGenerator gameOverWallpaperGenerator = BillBoardRepeatedGenerator(32.0f, 32.0f, glm::vec3(-2.0f, 0.0f, 0.0f), 32);
		BillBoardGenerator gameOverWriteGenerator = BillBoardGenerator(18.0f, 3.0f, glm::vec3(0.0f, 0.0f, 0.0f));

};

#endif