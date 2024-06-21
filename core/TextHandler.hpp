#include <ft2build.h>
#include FT_FREETYPE_H

#include <freetype/ftglyph.h>  // Glyph manipulation
#include <freetype/ftoutln.h> // Outline processing
#include <freetype/ftsystem.h> // Memory management, etc.

#include <vulkan/vulkan.hpp> // Vulkan library

class TextHandler {

    public:

        TextHandler(vk::Device device, vk::PhysicalDevice physicalDevice, vk::CommandPool commandPool, vk::Queue graphicsQueue) 
            : device(device), physicalDevice(physicalDevice), commandPool(commandPool), graphicsQueue(graphicsQueue) {
            // Initialize FreeType library
            FT_Init_FreeType(&ftLibrary);
        
            // Create Vulkan resources for text rendering
            createDescriptorSetLayout();
            createPipelineLayout();
            createPipeline();
            createVertexBuffer();
            createIndexBuffer();
            createUniformBuffers();
            createDescriptorPool();
            createDescriptorSets();
        }
    
        ~TextHandler() {
            // Cleanup FreeType library
            FT_Done_FreeType(ftLibrary);
        
            // Destroy Vulkan resources for text rendering
            device.destroyDescriptorSetLayout(descriptorSetLayout);
            device.destroyPipelineLayout(pipelineLayout);
            device.destroyPipeline(pipeline);
            device.destroyBuffer(vertexBuffer);
            device.freeMemory(vertexBufferMemory);
            device.destroyBuffer(indexBuffer);
            device.freeMemory(indexBufferMemory);
            for (size_t i = 0; i < uniformBuffers.size(); i++) {
                device.destroyBuffer(uniformBuffers[i]);
                device.freeMemory(uniformBuffersMemory[i]);
            }
            device.destroyDescriptorPool(descriptorPool);
        }
    
        void renderText(const std::string& text, float x, float y, float scale, const glm::vec3& color) {
            // Render the specified text using FreeType and Vulkan
            // ...
        }
    
    private:

        vk::Device device;
        vk::PhysicalDevice physicalDevice;
        vk::CommandPool commandPool;
        vk::Queue graphicsQueue;
    
        FT_Library ftLibrary;
    
        vk::DescriptorSetLayout descriptorSetLayout;
        vk::PipelineLayout pipelineLayout;
        vk::Pipeline pipeline;
    
        vk::Buffer vertexBuffer;
        vk::DeviceMemory vertexBufferMemory;
        vk::Buffer indexBuffer;
        vk::DeviceMemory indexBufferMemory;
    
        std::vector<vk::Buffer> uniformBuffers;
        std::vector<vk::DeviceMemory> uniformBuffersMemory;
    
        vk::DescriptorPool descriptorPool;
        std::vector<vk::DescriptorSet> descriptorSets;
    
        void createDescriptorSetLayout() {
            // Create descriptor set layout
            // ...
        }
    
        void createPipelineLayout() {
            // Create pipeline layout
            // ...
        }
    
        void createPipeline() {
            // Create pipeline
            // ...
        }
    
        void createVertexBuffer() {
            // Create vertex buffer
            // ...
        }
    
        void createIndexBuffer() {
            // Create index buffer
            // ...
        }
    
        void createUniformBuffers() {
            // Create uniform buffers
            // ...
        }
    
        void createDescriptorPool() {
            // Create descriptor pool
            // ...
        }
    
        void createDescriptorSets() {
            // Create descriptor sets
            // ...
        }
};
