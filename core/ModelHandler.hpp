

class ModelHandler {

    public:

        ModelHandler(glm::mat4 model, std::string modelPath = "", std::string texturePath = "") : modelMatrix(model), modelPath(modelPath), texturePath(texturePath) { }

        std::string modelPath;
        std::string texturePath;

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

        void generateModel(std::string modelPath) { }

        void generateTexture(std::string texturePath) { }

};