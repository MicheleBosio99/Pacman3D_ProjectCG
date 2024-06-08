
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


class EnvironmentModelHandler : public ModelHandler {

	public:

		EnvironmentModelHandler(glm::mat4 modelMatrix, std::string texturePath) : ModelHandler(modelMatrix, texturePath) { }

        glm::mat4 getModelMatrix() override { return modelMatrix; }
		
};


class GhostModelHandler : public ModelHandler {

    std::shared_ptr<Ghost> ghost;

	public:

		GhostModelHandler(std::shared_ptr<Ghost> ghost, glm::mat4 modelMatrix, std::string modelPath, std::string texturePath) : ghost(ghost), ModelHandler(modelMatrix, modelPath, texturePath) {
            ghost->setInitialModelMatrix(modelMatrix);
        }

        glm::mat4 getModelMatrix() override { return ghost->getModelMatrix(); }

};


class PelletModelHandler : public ModelHandler {

    public:

        int i, j; // Coordinates to the pellet in the grid system;

        PelletModelHandler(glm::mat4 modelMatrix, std::string texturePath, int i, int j) : ModelHandler(modelMatrix, texturePath), i(i), j(j) { }

        glm::mat4 getModelMatrix() override { return modelMatrix; }

};


#endif