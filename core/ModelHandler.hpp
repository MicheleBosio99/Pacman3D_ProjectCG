
#ifndef MODEL_HANDLER_HPP
#define MODEL_HANDLER_HPP


class ModelHandler {

    public:

        ModelHandler(glm::mat4 modelMatrix, std::string modelPath, std::string texturePath) : modelMatrix(modelMatrix), modelPath(modelPath), texturePath(texturePath) { }

        ModelHandler(glm::mat4 modelMatrix, std::string texturePath) : modelMatrix(modelMatrix), texturePath(texturePath) { }

        ModelHandler(glm::mat4 modelMatrix) : modelMatrix(modelMatrix) { }


        std::string modelPath = "";
        std::string texturePath = "";

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

        std::vector<VkBuffer> uniformBuffers;
        std::vector<VkDeviceMemory> uniformBuffersMemory;
        std::vector<void*> uniformBuffersMapped;

        std::vector<VkDescriptorSet> descriptorSets;
        uint32_t mipLevels;
        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        VkImageView textureImageView;
        VkSampler textureSampler;

        glm::mat4 modelMatrix;

        virtual glm::mat4 getModelMatrix() { return glm::mat4(1.0f); };

};


class GameModelHandler : public ModelHandler {

	public:

		GameModelHandler(glm::mat4 modelMatrix, std::string texturePath) : ModelHandler(modelMatrix, texturePath) { }

        glm::mat4 getModelMatrix() override { return modelMatrix; }
		
};


class PacmanModelHandler : public ModelHandler {

    glm::mat4 initialMatrixTransf;

    public:

        PacmanModelHandler(glm::mat4 modelMatrix, std::string modelPath, std::string texturePath) : ModelHandler(modelMatrix, modelPath, texturePath), initialMatrixTransf(modelMatrix) { }

        glm::mat4 getModelMatrix() override { return modelMatrix; }

        void modifyModelMatrix(glm::vec3 position, glm::vec3 front) {
            glm::vec3 initialTranslation = glm::vec3(initialMatrixTransf[3]);
            glm::vec3 translationDifference = position - initialTranslation + glm::vec3(0.0f, -10.0f, 0.0f);
            glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), translationDifference);

            glm::vec3 currentForward = glm::normalize(glm::vec3(initialMatrixTransf[2]));
            float currentAngle = atan2(currentForward.z, currentForward.x);
            float targetAngle = atan2(front.z, front.x);
            float angleDifference = targetAngle - currentAngle - glm::radians(90.0f);
            glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), -angleDifference, glm::vec3(0.0f, 1.0f, 0.0f));

            modelMatrix = translationMatrix * initialMatrixTransf * rotationMatrix;
        }
};


class GhostGameModelHandler : public ModelHandler {

    std::shared_ptr<Ghost> ghost;

	public:

		GhostGameModelHandler(std::shared_ptr<Ghost> ghost, glm::mat4 modelMatrix, std::string modelPath, std::string texturePath) : ghost(ghost), ModelHandler(modelMatrix, modelPath, texturePath) {
            ghost->setInitialModelMatrix(modelMatrix);
        }

        glm::mat4 getModelMatrix() override { return ghost->getModelMatrix(); }

};


class GhostMenuModelHandler : public ModelHandler {

    glm::mat4* modelMatrix;

    public:

        GhostMenuModelHandler(glm::mat4* modelMatrix, std::string modelPath, std::string texturePath) : modelMatrix(modelMatrix), ModelHandler(*modelMatrix, modelPath, texturePath) { }

        glm::mat4 getModelMatrix() override { return *modelMatrix; }

};


class PelletModelHandler : public ModelHandler {

    public:

        int i, j; // Coordinates to the pellet in the grid system;
        float pointsWhenEaten;

        PelletModelHandler(glm::mat4 modelMatrix, std::string texturePath, int i, int j) : ModelHandler(modelMatrix, texturePath), i(i), j(j), pointsWhenEaten(10.0f) { }

        glm::mat4 getModelMatrix() override { return modelMatrix; }

};



#endif