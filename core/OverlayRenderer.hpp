
/*


/ START 20 - OVERLAY ____________________________________________________________________________________________________________________________________________________________


// New vk variables for the overlay;
VkRenderPass overlayRenderPass;
VkPipelineLayout overlayPipelineLayout;
VkPipeline overlayGraphicsPipeline;
VkDescriptorPool overlayDescriptorPool;
VkDescriptorSetLayout overlayDescriptorSetLayout;
std::vector<VkDescriptorSet> overlayDescriptorSets;
VkCommandBuffer overlayCommandBuffer;
std::vector<VkCommandBuffer> overlayCommandBuffers;
float blendConstants[4] = { 1.0f, 1.0f, 1.0f, 1.0f };


std::vector<std::shared_ptr<ModelHandler>> overlayModelHandlers;


void createOverlay() {
    loadOverlayModelHandlers();
    initOverlay();
    // mainGameLoop();
    // overlayCleanup();
}

// Initialize the overlay;
void initOverlay() {

    createOverlayRenderPass();
    createOverlayDescriptorSetLayout();
    createOverlayGraphicsPipeline();
    //createOverlayFramebuffers();
    createOverlayDescriptorPool();

    for (const auto& handler : overlayModelHandlers) {

        createTextureImage(*handler); // LOAD IMAGES;
        createTextureImageView(*handler); // Create texture image view;
        createTextureSampler(*handler); // Create sampler;

        loadModel(*handler); // Load model;

        createVertexBuffer(*handler); // Create vertex buffer;
        createIndexBuffer(*handler); // Create index buffer;
        createUniformBuffers(*handler); // Create uniform buffer;
        createOverlayDescriptorSets(*handler); // Create descriptor set;
    }

    createOverlayCommandBuffers();
}

// Create the render pass for the overlay;
void createOverlayRenderPass() {

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Load the existing contents of the attachment
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the rendered content
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Assuming overlay is rendered last
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    // Subpass dependency to ensure proper image layout transition
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &overlayRenderPass) != VK_SUCCESS) { throw std::runtime_error("failed to create render pass!"); }
}

// Create the overlay descriptor set layout;
void createOverlayDescriptorSetLayout() {

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.binding = 0;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.pImmutableSamplers = nullptr; // Optional;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &samplerLayoutBinding;

    if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &overlayDescriptorSetLayout) != VK_SUCCESS) { throw std::runtime_error("failed to create descriptor set layout!"); }
}

// Create the overlay framebuffers;
void createOverlayGraphicsPipeline() {

    // Load shaders
    auto vertShaderCode = readFile("shaders/ShaderVert.spv");
    auto fragShaderCode = readFile("shaders/ShaderFrag.spv");

    // Create shader modules for shaders loaded;
    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);

    // Load vert shaders in the pipeline;
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // Tell pipeline where to use this shader;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main"; // Where to start in the shader code to execute (means can be combined multiple shaders);
    vertShaderStageInfo.pSpecializationInfo = nullptr; // Here is null, but can be used to pass to the shader different parameters for the constants used in the shader code;

    // Load frag shaders in the pipeline;
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";
    vertShaderStageInfo.pSpecializationInfo = nullptr;

    // Array with the shaders structs;
    VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

    // Describe format of vertex data that will be passed to the shader;
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    auto bindingDescription = Vertex::getBindingDescription();
    auto attributeDescriptions = Vertex::getAttributeDescriptions();

    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


    // Describes what kind of geometry will be drawn from the vertices and if primitive restart should be enabled;
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    // Define viewport;
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)swapChainExtent.width;
    viewport.height = (float)swapChainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor rectangle to draw the entire framebuffer;
    VkRect2D scissor{};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    // Dynamic state setup (ignore configuration of some values when creating the pipeline);
    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    // Create viewport state;
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_NONE;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;

    // Multisampling (disabled)
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    // Color blending (enable blending for transparency)
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 1.0f; // R
    colorBlending.blendConstants[1] = 1.0f; // G
    colorBlending.blendConstants[2] = 1.0f; // B
    colorBlending.blendConstants[3] = 1.0f; // A

    // Depth and stencil state (disabled for overlays)
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_FALSE;
    depthStencil.depthWriteEnable = VK_FALSE;

    // Create pipelineLayout;
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1; // Optional
    pipelineLayoutInfo.pSetLayouts = &overlayDescriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &overlayPipelineLayout) != VK_SUCCESS) { throw std::runtime_error("failed to create pipeline layout!"); }

    // CREATE PIPELINE FINALLY;
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.layout = overlayPipelineLayout;
    pipelineInfo.renderPass = overlayRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional

    // Create pipeline;
    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &overlayGraphicsPipeline) != VK_SUCCESS) {
        throw std::runtime_error("failed to create graphics pipeline!");
    }

    // Destroy shader modules;
    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
}

// Create the overlay descriptor pool;
void createOverlayDescriptorPool() {
    size_t totalDescriptorSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * overlayModelHandlers.size();

    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = totalDescriptorSets;

    std::array<VkDescriptorPoolSize, 2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = totalDescriptorSets;

    if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &overlayDescriptorPool) != VK_SUCCESS) { throw std::runtime_error("failed to create descriptor pool!"); }
}

// Record overlay command buffer;
void recordOverlayCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) { throw std::runtime_error("failed to begin recording command buffer!"); }

    // Start the render pass;
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = overlayRenderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex]; // Pick right framebuffer for current swapchain image;
    renderPassInfo.renderArea.offset = { 0, 0 }; // Size of render area
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    // clearValues[1].depthStencil = { 1.0f, 0 };

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // Being render pass: VK_SUBPASS_CONTENTS_INLINE used to use only first level framebuffers;
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, overlayGraphicsPipeline); // Bind overlay graphics pipeline;

    for (const auto& handlerPtr : overlayModelHandlers) {

        ModelHandler& handler = *handlerPtr;

        VkBuffer vertexBuffers[] = { handler.vertexBuffer };
        VkDeviceSize offsets[] = { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(commandBuffer, handler.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
            &handler.descriptorSets[currentFrame], 0, nullptr);
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(handler.indices.size()), 1, 0, 0, 0);
    }

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapChainExtent.width);
    viewport.height = static_cast<float>(swapChainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = { 0, 0 };
    scissor.extent = swapChainExtent;

    vkCmdSetDepthTestEnable(commandBuffer, VK_FALSE);

    // Enable alpha blending
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    vkCmdSetBlendConstants(commandBuffer, blendConstants);

    vkCmdEndRenderPass(commandBuffer);
    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) { throw std::runtime_error("failed to record command buffer!"); }
}

// Create overlay command buffers;
void createOverlayCommandBuffers() {
    overlayCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)overlayCommandBuffers.size();

    if (vkAllocateCommandBuffers(device, &allocInfo, overlayCommandBuffers.data()) != VK_SUCCESS) { throw std::runtime_error("failed to allocate overlay command buffers!"); }
}

// Create overlay descriptor sets;
void createOverlayDescriptorSets(ModelHandler& handler) {

    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, overlayDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = overlayDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();

    handler.descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    if (vkAllocateDescriptorSets(device, &allocInfo, handler.descriptorSets.data()) != VK_SUCCESS) { throw std::runtime_error("failed to allocate descriptor sets!"); }

    // Config each descriptor;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = handler.uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = handler.textureImageView;
        imageInfo.sampler = handler.textureSampler;

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = handler.descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = handler.descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

// Update overlay uniform buffer;
void updateOverlayUniformBuffer(ModelHandler& handler, uint32_t currentImage) {
    UniformBufferObject ubo{ };

    ubo.model = glm::mat4(1.0f);
    ubo.view = glm::mat4(1.0f);

    ubo.proj = glm::ortho(
        static_cast<float>(-swapChainExtent.width / 2),
        static_cast<float>(swapChainExtent.width / 2),
        static_cast<float>(-swapChainExtent.height / 2),
        static_cast<float>(swapChainExtent.height / 2),
        -1.0f,
        1.0f
    );

    ubo.proj[1][1] *= -1; // OpenGL standard to Vulkan;

    memcpy(handler.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

// Load overlay model handlers;
void loadOverlayModelHandlers() {

    std::vector<Vertex> overlayVertices1 = {
        { {-0.45f, 0.0f, -0.45f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0},
        { {0.45f, 0.0f, -0.45f}, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, 0},
        { {0.45f, 0.0f, 0.45f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0},
        { {-0.45f, 0.0f, 0.45f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0}
    };

    std::vector<uint32_t> overlayIndices1 = { 0, 1, 2, 2, 3, 0 };

    std::vector<Vertex> overlayVertices2 = {
        { {-0.45f, -0.45f, 0.0f }, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f}, 0},
        { {0.45f, -0.45f, 0.0f }, {1.0f, 1.0f, 1.0f}, {1.0f, 0.0f}, 0},
        { {0.45f, 0.45f, 0.0f }, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}, 0},
        { {-0.45f, 0.45f, 0.0f }, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}, 0}
    };

    std::vector<uint32_t> overlayIndices2 = { 0, 1, 2, 2, 3, 0 };

    auto rectangle1Handler = std::make_shared<EnvironmentModelHandler>(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)), "textures/test/Gold.png");
    rectangle1Handler->vertices = overlayVertices1;
    rectangle1Handler->indices = overlayIndices1;

    auto rectangle2Handler = std::make_shared<EnvironmentModelHandler>(glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)), "textures/test/Gold.png");
    rectangle2Handler->vertices = overlayVertices2;
    rectangle2Handler->indices = overlayIndices2;

    overlayModelHandlers.push_back(rectangle1Handler);
    overlayModelHandlers.push_back(rectangle2Handler);
}


// createVertexBuffer already defined;



In drawFrame:

for (const auto& handler : overlayModelHandlers) { updateOverlayUniformBuffer(*handler, currentFrame); } // Update overlay uniform buffer;

// RENDER OVERLAY;

vkResetCommandBuffer(overlayCommandBuffers[currentFrame], 0); // Assuming you have a separate command buffer array for the overlay
recordOverlayCommandBuffer(overlayCommandBuffers[currentFrame], imageIndex); // This function should be defined by you to record overlay commands

VkSubmitInfo overlaySubmitInfo{};
overlaySubmitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
overlaySubmitInfo.waitSemaphoreCount = 1;
overlaySubmitInfo.pWaitSemaphores = signalSemaphores; // Wait for the main scene to finish rendering
VkPipelineStageFlags overlayWaitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
overlaySubmitInfo.pWaitDstStageMask = overlayWaitStages;
overlaySubmitInfo.commandBufferCount = 1;
overlaySubmitInfo.pCommandBuffers = &overlayCommandBuffers[currentFrame];
overlaySubmitInfo.signalSemaphoreCount = 1;
overlaySubmitInfo.pSignalSemaphores = signalSemaphores; // Re-use the same semaphore for presentation

if (vkQueueSubmit(graphicsQueue, 1, &overlaySubmitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) { throw std::runtime_error("failed to submit overlay command buffer!"); }

// END RENDER OVERLAY;




*/



