
#include <iostream>
#include <vector>
#include <map>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>
#include <array>
#include <iostream>
#include <string>
#include <chrono>
#include <unordered_map>
#include <thread>
#include <chrono>
#include <future>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>


const uint32_t WIDTH = 1920;
const uint32_t HEIGHT = 1080;

const float pacmanHeight = 0.6f;

const std::string BASE_TEXTURE_PATH = "textures/test/BlackBrickedWall.png";

const std::vector<const char*> validationLayers = { "VK_LAYER_KHRONOS_validation" };

const int MAX_FRAMES_IN_FLIGHT = 2; // How many frames should be processed concurrently;

enum MATERIAL_TYPE {
    ENVIRONMENT_MAT,
    SKY_MAT,
    PELLET_MAT,
    POWER_PELLET_MAT,
    FROM_FILE_MAT,
    HUD_MAT,
};

// Queue families struct;
struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

// Extensions required struct (using strings);
const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

// Struct for properties needed for swapchain;
struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

// Vertex shader change from hardcoded to vertices passed as parameters;
struct Vertex {

    glm::vec3 pos;
    glm::vec3 color;
    glm::vec3 normCoord;
    glm::vec2 texCoord;
    MATERIAL_TYPE materialID;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, normCoord);

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, texCoord);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32_SINT;
        attributeDescriptions[4].offset = offsetof(Vertex, materialID);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const { return pos == other.pos && color == other.color && normCoord == other.normCoord && texCoord == other.texCoord && materialID == other.materialID; }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1)
                ^ (hash<glm::vec3>()(vertex.normCoord) << 2)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1)
                ^ (hash<int>()(vertex.materialID) << 3);
        }
    };
}

struct UniformBufferObject {
    glm::mat4 model;
    glm::mat4 modelNorm;
    glm::mat4 view;
    glm::mat4 proj;
};

struct GlobalUniformBufferObject {
    glm::vec3 viewerPos;

    glm::vec3 ambientLightDirection;
    glm::vec3 ambientLightColor;

    int pointLightCount = 12; // Uses only 12 point lights for the pellets to not overload the GPU;
    glm::vec3 pointLightPos[12];
    glm::vec4 pointLightColors[12];

    int ghostLightCount = 4;
    glm::vec3 ghostsLightsPos[4];
    glm::vec4 ghostsLightsColors[4];
};

static std::vector<char> readFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) { throw std::runtime_error("failed to open file!"); }

    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);
    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();

    return buffer;
}

// MY CODE _____________________________________________________________________________________________________________________________________________________________________________________________

#include "../core/ControlHandler.hpp"
#include "../core/GameEnvGenerator.hpp"
#include "../core/StartingMenuEnvGenerator.hpp"
#include "../core/GhostsBehaviour.hpp"
#include "../core/ModelHandler.hpp"
#include "../core/SoundManager.hpp"

#undef max
#undef min

glm::vec3 pacmanStartingPosition = glm::vec3(8.8f, pacmanHeight, 0.0f); // Set default Pacman starting position in world;

ViewCameraControl viewCamera = ViewCameraControl(pacmanStartingPosition, glm::vec3(0.0f, 1.0f, 0.0f), 180.0f, -55.0f, 5.0f); // Controller that handles view camera. Gets position, up, yaw, pitch and speed;

float deltaTime = 0.0f; // Time between current frame and last frame;
float lastFrame = 0.0f; // Time of last frame;

bool firstMouse = true; // Hold for first time the mouse is used;
float lastX = WIDTH / 2.0f, lastY = HEIGHT / 2.0f; // Window center;
bool isMousePressed = false; // Hold a boolean to know if the left mouse button is pressed;

bool closeApp = false; // Boolean determining wheter the app will be closed the next frame or not;

bool appInStartedScreen = false;
bool appInGameScreen = false;
bool appInGameOverScreen = false;

bool playerInHighView = false;
float playerHighViewY = pacmanHeight + 10.0f;


// Process the input received from keyboard using ViewCameraControl class;
void processInput(GLFWwindow* window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) { viewCamera.processKeyboardInput(FORWARD, deltaTime); }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) { viewCamera.processKeyboardInput(BACKWARD, deltaTime); }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) { viewCamera.processKeyboardInput(LEFT, deltaTime); }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) { viewCamera.processKeyboardInput(RIGHT, deltaTime); }
}

void closeAppOnEscPress(GLFWwindow* window) { if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) { closeApp = true; } } // If escape button is clicked exit the app;
bool checkSpaceBarPressed(GLFWwindow* window) {
    static bool spaceBarPressedLastFrame = false;
    bool isSpaceBarPressedNow = glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS;

    if (isSpaceBarPressedNow && !spaceBarPressedLastFrame) { spaceBarPressedLastFrame = isSpaceBarPressedNow; return true; }
    spaceBarPressedLastFrame = isSpaceBarPressedNow;
    return false;
}
void checkTABGotPressedPressedForViewChange(GLFWwindow* window) {
    static bool tabPressedLastFrame = false; // Tracks if TAB was pressed in the last frame;
    bool isTabPressedNow = glfwGetKey(window, GLFW_KEY_TAB) == GLFW_PRESS;

    auto currentSpeed = viewCamera.movementSpeed;

    if (isTabPressedNow && !tabPressedLastFrame) {
        playerInHighView = !playerInHighView;

        if (playerInHighView) {

            viewCamera.reInitializateAll(
                glm::vec3(viewCamera.position.x, playerHighViewY, viewCamera.position.z),
                viewCamera.front,
                glm::vec3(0.0f, 1.0f, 0.0f),
                - glm::degrees(atan2(viewCamera.front.x, viewCamera.front.z)) + 90.0f,
                -80.0f,
                currentSpeed
            );
            viewCamera.blockPitchMovement = true;
        }
        else {

            viewCamera.reInitializateAll(
                glm::vec3(viewCamera.position.x, pacmanHeight, viewCamera.position.z),
                viewCamera.front,
                glm::vec3(0.0f, 1.0f, 0.0f),
                - glm::degrees(atan2(viewCamera.front.x, viewCamera.front.z)) + 90.0f,
                0.0f,
                currentSpeed
            );
            viewCamera.blockPitchMovement = false;
        }
    }
    tabPressedLastFrame = isTabPressedNow;
}

// Mouse callback setted to handle mouse movements;
void mouse_callback(GLFWwindow* window, double xpos, double ypos) {
    static bool firstMouse = true;

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Reversed since y-coordinates go from bottom to top;

    lastX = xpos;
    lastY = ypos;

    viewCamera.processMouseMovement(xoffset, yoffset);

    // glfwSetCursorPos(window, WIDTH / 2.0, HEIGHT / 2.0);     // Center the mouse cursor after processing movement;
}

// Mouse button callback to make sure the mouse input is registered only when the left button gets clicked;
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) { isMousePressed = true; firstMouse = true; }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) { isMousePressed = false; }
}


// _____________________________________________________________________________________________________________________________________________________________________________________________________

// If applicaiton in debug mode then activate validation layers;
#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

class Pacman3D {

    public:

        void run() {

            initWindow();
            glfwInitialization();
            soundManagerInitialization();
            initVulkan();

            gameLoop(); // Start running the game logic and rendering;

            cleanup();
        }

    protected:

        GLFWwindow* window; // GLFW window;
        VkInstance instance; // Vulkan instance;
        VkPhysicalDevice physicalDevice = VK_NULL_HANDLE; // Physical device;
        VkDevice device; // Virtual device;
        VkQueue graphicsQueue; // Queue handler;
        VkSurfaceKHR surface; // Surface for integration with device OS;
        VkQueue presentQueue; // Handler for the current queue;
        VkSwapchainKHR swapChain; // Hold swapchain;
        std::vector<VkImage> swapChainImages; // Hold handles of swapchain;
        // Parameter chosen when creating the swapchain;
        VkFormat swapChainImageFormat;
        VkExtent2D swapChainExtent;
        std::vector<VkImageView> swapChainImageViews; // Image views;
        VkRenderPass renderPass; // Render Pass;
        VkDescriptorSetLayout descriptorSetLayout;
        VkPipelineLayout pipelineLayout; // Pipeline layout;
        VkPipeline graphicsPipeline; // PIPELINE;
        std::vector<VkFramebuffer> swapChainFramebuffers; // Framebuffer;
        VkCommandPool commandPool; // Command Pool;
        std::vector<VkCommandBuffer> commandBuffers; // Command Buffer;

        std::vector<VkSemaphore> imageAvailableSemaphores; // Semaphore to signal image is available to pick in swapchain;
        std::vector<VkSemaphore> renderFinishedSemaphores; // Semaphore to signal that rendering has finished and presentation can happen;
        std::vector<VkFence> inFlightFences; // Fence to make sure only one frame is rendering at a time;
        std::vector<VkFence> imagesInFlight;

        uint32_t currentFrame = 0; // Hold current frame;

        bool framebufferResized = false; // Has the window been resized?;

        VkDescriptorPool descriptorPool;

        std::vector<VkBuffer> globalUniformBuffers; // Global uniform buffers;
        std::vector<VkDeviceMemory> globalUniformBuffersMemory; // Global uniform buffers memory;
        std::vector<void*> globalUniformBuffersMapped; // Global uniform buffers mapped;

        VkImage depthImage;
        VkDeviceMemory depthImageMemory;
        VkImageView depthImageView;

        VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
        VkImage colorImage;
        VkDeviceMemory colorImageMemory;
        VkImageView colorImageView;

        // GAME LOGIC VARIABLES; ______________________________________________________________________________________________________________________________________________________________

        GameEnvGenerator envGenerator; // Environment generator;
        StartingMenuEnvGenerator startingMenuEnvGenerator; // Starting menu environment generator;
        GameOverEnvGenerator gameOverEnvGenerator; // Game over environment generator;

        GhostCollection ghosts = GhostCollection(envGenerator.mazeGenerator.getMaze()); // Enemy ghosts holder;

        std::vector<std::vector<std::tuple<std::shared_ptr<ModelHandler>, bool>>> pelletsInMaze; // Hold pellets in maze and their status;

        int powerPelletEaten = 0; // Power pellet eaten;
        float playerScore = 0.0f; // Player score;
        float playerLevelScore = 0.0f; // Player level score;
        int howManyPelletsLeft = 0; // How many pellets are left in the maze;
        int totalPellets = 0; // Total pellets in the maze;
        bool pacmanDefeated = false; // Has pacman been eaten?;
        bool pacmanWon = false; // Has pacman won the game?;
        bool isGameFinished = false; // Is the game finished?;
        bool hasPlayerWonCurrLevel = false; // Has the player won the game?;
        int livesMax = 3;
        int livesLeft; // Pacman lives;
        int lastLevel = 3; // Last level;
        int currentLevel = 1; // Current level;

        std::vector<std::shared_ptr<ModelHandler>> modelHandlers; // Hold the model handlers used in the current scene;
        std::vector<std::shared_ptr<ModelHandler>> startingMenuModelHandlers; // Hold all model handlers for the starting menu;
        std::vector<std::shared_ptr<ModelHandler>> gameModelHandlers; // Hold all model handlers for the game;
        std::vector<std::shared_ptr<ModelHandler>> gameOverModelHandlers; // Hold all model handlers for the game over screen;

        std::shared_ptr<CharacterMenuModelHandler> pacmanModelHandler; // Hold the pacman model handler;
        std::shared_ptr<CharacterMenuModelHandler> blinkyMenuHandler; // Hold the blinky ghost model handler;
        std::shared_ptr<CharacterMenuModelHandler> pinkyMenuHandler; // Hold the pinky ghost model handler;
        std::shared_ptr<CharacterMenuModelHandler> inkyMenuHandler; // Hold the inky ghost model handler;
        std::shared_ptr<CharacterMenuModelHandler> clydeMenuHandler; // Hold the clyde ghost model handler;

        std::shared_ptr<ModelHandler> gameOverWrite;
        std::shared_ptr<ModelHandler> youWonWrite;

        std::future<void> loadStartingMenuModelsFuture;
        std::future<void> loadGameModelsFuture;
        std::future<void> loadGameOverModelsFuture;


        // GAME LIFECYCLE LOGIC; ______________________________________________________________________________________________________________________________________________________________

        enum class GameState { STARTING_MENU, PACMAN_GAME, GAME_OVER };

        GameState currentGameState = GameState::STARTING_MENU;

        // Main game loop, contains starting menu screen, game screen and game over screen. When game over then back to the menu;
        void gameLoop() {
            appInStartedScreen = true;
            livesLeft = livesMax;

            loadAllModels(); // Make it run in parallel -> Why tf futures do not work??;

            while (!glfwWindowShouldClose(window) && !closeApp) {
                showStartingMenu();
                showPacmanGame();
                showGameOver();
            }
            
            // Remove the pacman model handler from the starting menu model handlers;
            startingMenuModelHandlers.erase(std::remove(startingMenuModelHandlers.begin(), startingMenuModelHandlers.end(), pacmanModelHandler), startingMenuModelHandlers.end());
            
            vkDeviceWaitIdle(device);
        }

        // Used to load all models needed for the game since they are not many. If needed make this run in parallel;
        void loadAllModels() {
            loadStartingMenuModelHandlers(); // -> Loading screen? But how if the fuckers cannot be loaded in parallel?
            loadGameModelHandlers();
            loadGameOverModelHandlers();
        }
        

        // Show the starting menu;
        void showStartingMenu() {
            appInStartedScreen = true;

            viewCamera.reInitializateAll(
                glm::vec3(8.5f, pacmanHeight, 0.0f),
                glm::vec3(-1.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f),
                180.0f,
                0.0f,
                0.0f,
                0.0f
            );

            modelHandlers = startingMenuModelHandlers;
            startStartingMenuCoroutines();

            while (!glfwWindowShouldClose(window) && !closeApp && appInStartedScreen) {

                float currentFrame = glfwGetTime(); deltaTime = currentFrame - lastFrame; lastFrame = currentFrame;

                moveGhostsInStartingMenu(deltaTime);
                processInput(window);
                appInStartedScreen = !checkSpaceBarPressed(window);
                closeAppOnEscPress(window);
                drawFrame();
                glfwPollEvents();
            }
            
            pacmanModelHandler->scaleModelMatrix(0.36f);
            // pacmanModelHandler->rotateInitialModelMatrix(-125.0f, glm::vec3(0.0f, 0.0f, 1.0f)); // Kinda good, could be better;

            appInStartedScreen = false;
        }


        // Run the actual game loop;
        void showPacmanGame() {

            appInGameScreen = true;
            viewCamera.reInitializateAll(
                pacmanStartingPosition,
                glm::vec3(0.0f, 1.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f),
                180.0f,
                0.0f,
                5.0f
            );
            
            modelHandlers = gameModelHandlers;

            stopStartingMenuCoroutines(); // Stop the starting menu coroutines since the game is starting;
            startGameCoroutines(); // Start the game coroutines for the game to run properly;

            // mainGameLoop();
            while (!glfwWindowShouldClose(window) && !closeApp && appInGameScreen && !isGameFinished) {

                float currentFrame = glfwGetTime();
                deltaTime = currentFrame - lastFrame;
                lastFrame = currentFrame;

                // processInput(window); // Used to move camera with no wall collisions;
                movePlayerAndCheckCollisionsWithWalls(); // Move player and check for collisions;
                updatePlayerPositionForSound(); // Update player position for sound;
                movePacmanModel();
                checkForPlayerCollisionsWPellets(); // Check for player collisions with pellets;

                ghosts.moveAllGhosts(deltaTime, viewCamera.position, viewCamera.front); // Move all ghosts;
                checkForEndGame(); // Check if the game is over;

                if (pacmanDefeated and livesLeft > 0) { pacmanGotEaten(); } // If pacman got eaten then respawn him;
                else if (pacmanDefeated and livesLeft == 0) { isGameFinished = true; } // If pacman got eaten and has no lives left or player won then game is finished;
                else if (hasPlayerWonCurrLevel) { setUpNewLevel(); }

                checkTABGotPressedPressedForViewChange(window); // Check if the TAB button was pressed to change the view;

                closeAppOnEscPress(window);
                drawFrame();
                glfwPollEvents();
            }

            ghosts.stopAllGhostsSirens();
            if (SoundManager::isSoundPlaying("pacman_power-pellet-eaten")) { SoundManager::stopSound("pacman_power-pellet-eaten"); }

            playerInHighView = false;
            isGameFinished = false;
            appInGameScreen = false;
        }


        // Game is over, show game over screen;
        void showGameOver() {
            appInGameOverScreen = true;

            viewCamera.reInitializateAll(
                glm::vec3(8.5f, pacmanHeight, 0.0f),
                glm::vec3(-1.0f, 0.0f, 0.0f),
                glm::vec3(0.0f, 1.0f, 0.0f),
                180.0f,
                0.0f,
                0.0f,
                0.0f
            );

            modelHandlers = gameOverModelHandlers;

            // Modify positions of banners based on if the player won or lost;

            if (pacmanWon) { youWonWrite->modelMatrix = glm::translate(youWonWrite->modelMatrix, glm::vec3(10.0f, 0.0f, 0.0f)); }
            else { gameOverWrite->modelMatrix = glm::translate(gameOverWrite->modelMatrix, glm::vec3(10.0f, 0.0f, 0.0f)); }

            SoundManager::playSoundLooped("pacman_intermission");

            float time = glfwGetTime(); // Show it for 5.2 seconds;

            while (!glfwWindowShouldClose(window) && !closeApp && appInGameOverScreen && glfwGetTime() - time < 5.2f) {

                appInGameOverScreen = !checkSpaceBarPressed(window);
                closeAppOnEscPress(window);
                drawFrame();
                glfwPollEvents();
            }

            if(SoundManager::isSoundPlaying("pacman_intermission")) { SoundManager::stopSound("pacman_intermission"); }

            if (pacmanWon) { youWonWrite->modelMatrix = glm::translate(youWonWrite->modelMatrix, glm::vec3(-10.0f, 0.0f, 0.0f)); }
            else { gameOverWrite->modelMatrix = glm::translate(gameOverWrite->modelMatrix, glm::vec3(-10.0f, 0.0f, 0.0f)); }

            pacmanModelHandler->modelMatrix = generateModelMatrix(pacmanModelStartingPosition + glm::vec3(4.0f, 10.0f, 0.0f), 180.0f, 0.0f, 0.0f, 0.55f);
            pacmanModelHandler->scaleModelMatrix(1.0f / 0.36f);

            resetGameAfterGameOver();
            appInGameOverScreen = false;
        }

        // ____________________________________________________________________________________________________________________________________________________________________________________________


        // Initialize the window;
        void initWindow() {
            glfwInit();
            glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // Tell GLFW that this is not OpenGL;
            glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

            int monitorCount;
            GLFWmonitor** monitors = glfwGetMonitors(&monitorCount);
            GLFWmonitor* primaryMonitor = monitorCount == 1 ? monitors[0] : monitors[1];

            const GLFWvidmode* mode = glfwGetVideoMode(primaryMonitor);

            window = glfwCreateWindow(mode->width, mode->height, "Pacman3D", primaryMonitor, NULL); // Create the window (width, height, name, monitor, dc);
            glfwSetWindowUserPointer(window, this);
            glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
        }

        // Initialize the Vulkan instance;
        void initVulkan() {
            createInstance(); // Create a Vulkan instance;
            // setupDebugMessenger(); <- to understand why doesnt work :C;
            createSurface();
            pickPhysicalDevice(); // Add physical device;
            createLogicalDevice(); // Create the logical device;
            createSwapChain(); // Create swapchain;
            createImageViews(); // Create Image view;
            createRenderPass(); // Render pass object creation;
            createDescriptorSetLayout(); // DescriptorSetLayout;
            createGraphicsPipeline(); // Set up the pipeline;
            createCommandPool(); // Create command pool;
            createColorResources();
            createDepthResources();
            createFramebuffers(); // Create the framebuffers;
            createCommandBuffers(); // Create command buffer;
            createSyncObjects();
        }

        // Generate the models info for the game;
        void generateModels(std::vector<std::shared_ptr<ModelHandler>>& thisModelHandler) {

            createDescriptorPool(thisModelHandler); // Create descriptor pool;
            createGlobalUniformBuffers();

            for (const auto& handler : thisModelHandler) {

                createTextureImage(*handler); // LOAD IMAGES;
                createTextureImageView(*handler); // Create texture image view;
                createTextureSampler(*handler); // Create sampler;

                loadModel(*handler);

                createVertexBuffer(*handler); // Create vertex buffer;
                createIndexBuffer(*handler); // Create index buffer;
                createUniformBuffers(*handler); // Create uniform buffer;
                createDescriptorSets(*handler); // Create descriptor set;
            }
        }

        // Initialize other things such as callbacks;
        void glfwInitialization() {
            // glfwSetCursorPos(window, WIDTH / 2.0, HEIGHT / 2.0); // Put cursor at the center of the window;
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            glfwSetCursorPosCallback(window, mouse_callback); // Set the callback to handle the mouse updates;
            // glfwSetMouseButtonCallback(window, mouse_button_callback); // Set the callback for the mouse only if the left button gets clicked;
        }

        // Initialize the sounds;
        void soundManagerInitialization() {
            if (!SoundManager::initSoundManager()) { return; }

            const std::string initialMenuBGMusic = "initialMenuBGMusic";
            const std::string initialMenuBGMusicPath = "sounds/pacman_beginning.wav";

            std::unordered_map<std::string, std::string> soundsToLoad = {

                { "pacman_death", "sounds/pacman_death.wav" },
                { "pacman_eat-fruit", "sounds/pacman_eat-fruit.wav" },
                { "pacman_ghost-eaten", "sounds/pacman_ghost-eaten.wav" },
                { "pacman_ghost-siren", "sounds/pacman_ghost-siren.wav" },
                { "pacman_ghosts-to-hub", "sounds/pacman_ghosts-to-hub.wav" },
                { "pacman_intermission", "sounds/pacman_intermission.wav" },
                { "pacman_intro", "sounds/pacman_intro.wav" },
                { "pacman_new-highscore", "sounds/pacman_new-highscore.wav" },
                { "pacman_power-pellet-eaten", "sounds/pacman_power-pellet-eaten.wav" },
                { "pacman_respawn", "sounds/pacman_respawn.wav" },
                { "pacman_teleport", "sounds/pacman_teleport.wav" },
                { "pacman_wakawaka", "sounds/pacman_wakawaka-single.wav" },

            };

            for (const auto& sound : soundsToLoad) {
                if (!SoundManager::loadSound(sound.first, sound.second)) {
                    std::cerr << "Failed to load " << sound.first << "." << std::endl;
                    SoundManager::deInitSoundManager();
                    return;
                }
			}
        }


        // STARTING MENU LOGIC; ______________________________________________________________________________________________________________________________________________________________________

        // Load the models used in the starting menu;
        void loadStartingMenuModelHandlers() {

            startingMenuModelHandlers.clear();

            auto titleHandler = std::make_shared<GameModelHandler>(
                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f)),
                "textures/menus/PacmanLogo5.png"
            );
            titleHandler->vertices = startingMenuEnvGenerator.titleGenerator.getBillboardVertices();
            titleHandler->indices = startingMenuEnvGenerator.titleGenerator.getBillboardIndices();

            auto spacebarHandler = std::make_shared<GameModelHandler>(
                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.0f, 0.0f)),
                "textures/menus/PressSpaceBarText.png"
            );
            spacebarHandler->vertices = startingMenuEnvGenerator.spacebarGenerator.getBillboardVertices();
            spacebarHandler->indices = startingMenuEnvGenerator.spacebarGenerator.getBillboardIndices();

            auto floorHandler = std::make_shared<GameModelHandler>(
                glm::translate(glm::mat4(1.0f), glm::vec3(5.0f, -3.0f, 0.0f)),
                "textures/filter_forge/PacmanFloor2.png"
            );
            floorHandler->vertices = startingMenuEnvGenerator.floorGenerator.getFloorVertices();
            floorHandler->indices = startingMenuEnvGenerator.floorGenerator.getFloorIndices();


            pacmanModelHandler = std::make_shared<CharacterMenuModelHandler>(
                generateModelMatrix(pacmanModelStartingPosition + glm::vec3(4.0f, 10.0f, 0.0f), 180.0f, 0.0f, 0.0f, 0.55f),
                "models/pacman.obj",
                "textures/pacman/pacman.png",
                -10.0f
            );

            blinkyMenuHandler = std::make_shared<CharacterMenuModelHandler>(
                generateModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f), 180.0f, 0.0f, 0.0f, 0.64f),
				"models/ghostModel.obj",
				"textures/ghosts/BlinkyTex.png"
			);

            pinkyMenuHandler = std::make_shared<CharacterMenuModelHandler>(
                generateModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f), 180.0f, 0.0f, 0.0f, 0.64f),
                "models/ghostModel.obj",
                "textures/ghosts/PinkyTex.png"
            );

            inkyMenuHandler = std::make_shared<CharacterMenuModelHandler>(
                generateModelMatrix(glm::vec3(0.0f, 0.0f, 0.0f), 180.0f, 0.0f, 0.0f, 0.64f),
                "models/ghostModel.obj",
                "textures/ghosts/InkyTex.png"
            );

            clydeMenuHandler = std::make_shared<CharacterMenuModelHandler>(
                generateModelMatrix(glm::vec3(0.0, 0.0f, 0.0f), 180.0f, 0.0f, 0.0f, 0.64f),
                "models/ghostModel.obj",
                "textures/ghosts/ClydeTex.png"
            );

            startingMenuModelHandlers.push_back(titleHandler);
            startingMenuModelHandlers.push_back(spacebarHandler);
            startingMenuModelHandlers.push_back(floorHandler);

            startingMenuModelHandlers.push_back(pacmanModelHandler);
            
            startingMenuModelHandlers.push_back(blinkyMenuHandler);
            startingMenuModelHandlers.push_back(pinkyMenuHandler);
            startingMenuModelHandlers.push_back(inkyMenuHandler);
            startingMenuModelHandlers.push_back(clydeMenuHandler);

            generateModels(startingMenuModelHandlers);
        }

        bool moveGhosts = false;

        // Start the coroutines for the starting menu;
        void startStartingMenuCoroutines() {
            SoundManager::playSoundLooped("pacman_intro", 0.75f);
            SoundManager::playSoundLooped("pacman_ghost-siren", 0.4f);
            moveGhosts = true;
        }

        void stopStartingMenuCoroutines() {
            if(SoundManager::isSoundPlaying("pacman_intro")) { SoundManager::stopSound("pacman_intro"); }
            if (SoundManager::isSoundPlaying("pacman_ghost-siren")) { SoundManager::stopSound("pacman_ghost-siren"); }
            if (SoundManager::isSoundPlaying("pacman_power-pellet-eaten")) { SoundManager::stopSound("pacman_power-pellet-eaten"); }
            moveGhosts = false;
        }

        // Variables used to move the ghosts in the starting menu; 
        glm::vec3 pacmanOffset = glm::vec3(0.0f, 7.8f, -7.2f);
        glm::vec3 blinkyOffset = glm::vec3(0.0f, -1.0f, -12.0f);
        glm::vec3 pinkyOffset = glm::vec3(-0.4f, -1.2f, -14.4f);
        glm::vec3 inkyOffset = glm::vec3(1.0f, -1.0f, -16.0f);
        glm::vec3 clydeOffset = glm::vec3(0.4f, -1.2f, -20.5f);

        glm::vec3 pacmanCurrentPos = pacmanOffset;
        glm::vec3 blinkyCurrentPos = blinkyOffset;
        glm::vec3 pinkyCurrentPos = pinkyOffset;
        glm::vec3 inkyCurrentPos = inkyOffset;
        glm::vec3 clydeCurrentPos = clydeOffset;

        float elapsedTime = 0.0f;
        float horSpeed = 2.5f;
        float vertSpeed = 2.5f;

        float direction = 1.0f;

        float pacmanVertLength = 0.0f;
        float blinkyVertLength = 0.0f;
        float pinkyVertLength = 1.2f;
        float inkyVertLength = 0.2f;
        float clydeVertLength = -0.5f;

        float pacmanVertOffset = 0.0f;
        float blinkyVertOffset = 0.5f;
        float pinkyVertOffset = 0.35f;
        float inkyVertOffset = 0.6f;
        float clydeVertOffset = 0.4f;

        // Move the ghosts in the starting menu;
        void moveGhostsInStartingMenu(float deltaTime) {

            if(!moveGhosts || deltaTime > 0.5f) { return; }
            elapsedTime += deltaTime;

            bool soundPlayed = false;

            if (elapsedTime > 9.2f) { direction *= -1.0f; elapsedTime = 0.0f; soundPlayed = false; }
            else if (elapsedTime > 7.2f) {

                if (!soundPlayed) {
                    if (direction > 0.0f) {
                        if (!SoundManager::isSoundPlaying("pacman_power-pellet-eaten")) { SoundManager::playSoundLooped("pacman_power-pellet-eaten", 0.6f); }
                        soundPlayed = true;
                    }
                    else { if (SoundManager::isSoundPlaying("pacman_power-pellet-eaten")) { SoundManager::stopSound("pacman_power-pellet-eaten"); } }
                }

                pacmanModelHandler->modifyModelMatrix(pacmanCurrentPos, glm::vec3(1.0f, 0.0f, 0.75f));

                blinkyMenuHandler->modifyModelMatrix(blinkyCurrentPos, glm::vec3(cos(elapsedTime) * 0.4f + blinkyVertOffset + 1.0f, 0.0f, direction));

                pinkyMenuHandler->modifyModelMatrix(pinkyCurrentPos, glm::vec3(cos(elapsedTime) * 0.4f + pinkyVertOffset + 1.0f, 0.0f, direction));

                inkyMenuHandler->modifyModelMatrix(inkyCurrentPos, glm::vec3(cos(elapsedTime) * 0.4f + inkyVertOffset + 1.0f, 0.0f, direction));

                clydeMenuHandler->modifyModelMatrix(clydeCurrentPos, glm::vec3(cos(elapsedTime) * 0.4f + clydeVertOffset + 1.0f, 0.0f, direction));
            }
            else {

                pacmanCurrentPos.y = pacmanCurrentPos.y;
                blinkyCurrentPos.y = sin(elapsedTime * vertSpeed + blinkyVertLength) * blinkyVertOffset + blinkyOffset.y;
                pinkyCurrentPos.y = sin(elapsedTime * vertSpeed + pinkyVertLength) * pinkyVertOffset + pinkyOffset.y;
                inkyCurrentPos.y = sin(elapsedTime * vertSpeed + inkyVertLength) * inkyVertOffset + inkyOffset.y;
                clydeCurrentPos.y = sin(elapsedTime * vertSpeed + clydeVertLength) * clydeVertOffset + clydeOffset.y;

                pacmanCurrentPos.z += horSpeed * deltaTime * direction;
                blinkyCurrentPos.z += horSpeed * deltaTime * direction;
                pinkyCurrentPos.z += horSpeed * deltaTime * direction;
                inkyCurrentPos.z += horSpeed * deltaTime * direction;
                clydeCurrentPos.z += horSpeed * deltaTime * direction;

                pacmanModelHandler->modifyModelMatrix(pacmanCurrentPos, glm::vec3(cos(elapsedTime) * 0.6f + 0.4f, 0.0f, direction));

                blinkyMenuHandler->modifyModelMatrix(blinkyCurrentPos, glm::vec3(cos(elapsedTime) * 0.4f + blinkyVertOffset, 0.0f, direction));

                pinkyMenuHandler->modifyModelMatrix(pinkyCurrentPos, glm::vec3(cos(elapsedTime) * 0.4f + pinkyVertOffset, 0.0f, direction));

                inkyMenuHandler->modifyModelMatrix(inkyCurrentPos, glm::vec3(cos(elapsedTime) * 0.4f + inkyVertOffset, 0.0f, direction));

                clydeMenuHandler->modifyModelMatrix(clydeCurrentPos, glm::vec3(cos(elapsedTime) * 0.4f + clydeVertOffset, 0.0f, direction));
            }
        }

        // ____________________________________________________________________________________________________________________________________________________________________________________________


        // GAME OVER LOGIC; ___________________________________________________________________________________________________________________________________________________________________________

        // TODO;
        void loadGameOverModelHandlers() {

			gameOverModelHandlers.clear();

            auto wallpaper = std::make_shared<GameModelHandler>(
                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
                "textures/filter_forge/GameOverWallSmile.png"
            );
            wallpaper->vertices = gameOverEnvGenerator.gameOverWallpaperGenerator.getBillboardVertices();
            wallpaper->indices = gameOverEnvGenerator.gameOverWallpaperGenerator.getBillboardIndices();

            gameOverWrite = std::make_shared<GameModelHandler>(
                glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 0.92f, 0.0f)),
                "textures/menus/GameOver_LargeAndPellets.png"
            );
            gameOverWrite->vertices = gameOverEnvGenerator.gameOverWriteGenerator.getBillboardVertices();
            gameOverWrite->indices = gameOverEnvGenerator.gameOverWriteGenerator.getBillboardIndices();

            youWonWrite = std::make_shared<GameModelHandler>(
                glm::translate(glm::mat4(1.0f), glm::vec3(-10.0f, 0.92f, 0.0f)),
                "textures/menus/YouWon_LargeAndPellets.png"
            );
            youWonWrite->vertices = gameOverEnvGenerator.youWonWriteGenerator.getBillboardVertices();
            youWonWrite->indices = gameOverEnvGenerator.youWonWriteGenerator.getBillboardIndices();

            gameOverModelHandlers.push_back(wallpaper);
            gameOverModelHandlers.push_back(gameOverWrite);
            gameOverModelHandlers.push_back(youWonWrite);

			generateModels(gameOverModelHandlers);
		}

        // Reset the game after game over;
        void resetGameAfterGameOver() {
            playerLevelScore = 0.0f;
            playerScore = 0.0f;
            currentLevel = 1;
            ghosts = GhostCollection(envGenerator.mazeGenerator.getMaze());
            livesLeft = livesMax;
            powerPelletEaten = 0;

            for (auto& modelRow : pelletsInMaze) {
                for (auto& [p, eaten] : modelRow) {
                    if (p != nullptr && eaten == true) { std::dynamic_pointer_cast<PelletModelHandler>(p)->translatePellet(glm::vec3(0.0f, 10.0f, 0.0f)); }
                    eaten = false;
                }
            }

            auto positions = livesPosition[livesLeft];
            for (int i = 0; i < livesLeft; i ++) { livesModels[i]->modelMatrix = generateModelMatrix(positions[i], 90.0, 0.0f, 0.0f, 0.035f); }

            viewCamera.blockPitchMovement = false;
            howManyPelletsLeft = totalPellets;
            pacmanDefeated = false;
            isGameFinished = false;
            hasPlayerWonCurrLevel = false;
            pacmanDefeated = false;
            pacmanWon = false;
            // modelHandlers.clear();
        }


        // PACMAN GAME LOGIC; _________________________________________________________________________________________________________________________________________________________________________

        glm::vec3 pacmanModelStartingPosition = glm::vec3(0.0f, -10.0f, 0.0f); // Set default Pacman MODEL starting position in world;

        // Create the model handlers for needed models;
        void loadGameModelHandlers() {

            gameModelHandlers.clear();

            auto labirinthHandler = std::make_shared<GameModelHandler>(
                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
                "textures/wall/PacmanWall1.png" // "textures/filter_forge/PacmanWall14_EpilecticAttackEdition.png"
            );
            labirinthHandler->vertices = envGenerator.mazeGenerator.getMazeVertices();
            labirinthHandler->indices = envGenerator.mazeGenerator.getMazeIndices();

            auto floorHandler = std::make_shared<GameModelHandler>(
                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
                "textures/floor/PacmanFloor.png"
            );
            floorHandler->vertices = envGenerator.floorGenerator.getFloorVertices();
            floorHandler->indices = envGenerator.floorGenerator.getFloorIndices();

            auto skyHandler = std::make_shared<GameModelHandler>(
                glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)),
                "textures/test/Sky.png"
            );
            skyHandler->vertices = envGenerator.skyGenerator.geSkyVertices();
            skyHandler->indices = envGenerator.skyGenerator.getSkyIndices();

            auto leftPortalHandler = std::make_shared<GameModelHandler>(
                glm::translate(glm::mat4(1.0f), glm::vec3(-0.5f, 0.0f, -14.0f)),
                "textures/test/Portal.png"
            );
            leftPortalHandler->vertices = envGenerator.teleporterGenerator.getTeleporterVertices();
            leftPortalHandler->indices = envGenerator.teleporterGenerator.getTeleporterIndices();

            auto rightPortalHandler = std::make_shared<GameModelHandler>(
                generateModelMatrix(glm::vec3(-0.5f, 0.0f, 14.0f), 180.0f, 0.0f, 0.0f), // Rotate of 180 to have normCoords correctly setted towards origin;
                "textures/test/Portal.png"
            );
            rightPortalHandler->vertices = envGenerator.teleporterGenerator.getTeleporterVertices();
            rightPortalHandler->indices = envGenerator.teleporterGenerator.getTeleporterIndices();

            auto gateHandler = std::make_shared<GameModelHandler>(
                glm::translate(glm::mat4(1.0f), glm::vec3(-2.5f, 0.0f, 0.0f)),
                "textures/test/Gate.png"
            );
            gateHandler->vertices = envGenerator.gateGenerator.getGateVertices();
            gateHandler->indices = envGenerator.gateGenerator.getGateIndices();


            pacmanModelHandler->modifyModelMatrix(pacmanStartingPosition, glm::vec3(0.0f, 1.0f, 0.0f));

            auto blinkyHandler = std::make_shared<GhostGameModelHandler>(
                ghosts.blinky,
                generateModelMatrix(ghosts.blinky->getCurrentPosition(), 0.0f, 0.0f, 0.0f, ghostsScale),
                "models/ghostModel.obj",
                "textures/ghosts/BlinkyTex.png"
            );

            auto pinkyHandler = std::make_shared<GhostGameModelHandler>(
                ghosts.pinky,
                generateModelMatrix(ghosts.pinky->getCurrentPosition(), 0.0f, 0.0f, 0.0f, ghostsScale),
                "models/ghostModel.obj",
                "textures/ghosts/PinkyTex.png"
            );

            auto inkyHandler = std::make_shared<GhostGameModelHandler>(
                ghosts.inky,
                generateModelMatrix(ghosts.inky->getCurrentPosition(), 0.0f, 0.0f, 0.0f, ghostsScale),
                "models/ghostModel.obj",
                "textures/ghosts/InkyTex.png"
            );

            auto clydeHandler = std::make_shared<GhostGameModelHandler>(
                ghosts.clyde,
                generateModelMatrix(ghosts.clyde->getCurrentPosition(), 0.0f, 0.0f, 0.0f, ghostsScale),
                "models/ghostModel.obj",
                "textures/ghosts/ClydeTex.png"
            );


            gameModelHandlers.push_back(labirinthHandler);
            gameModelHandlers.push_back(floorHandler);
            gameModelHandlers.push_back(skyHandler);
            gameModelHandlers.push_back(leftPortalHandler);
            gameModelHandlers.push_back(rightPortalHandler);
            gameModelHandlers.push_back(gateHandler);

            gameModelHandlers.push_back(pacmanModelHandler);

            gameModelHandlers.push_back(blinkyHandler);
            gameModelHandlers.push_back(pinkyHandler);
            gameModelHandlers.push_back(inkyHandler);
            gameModelHandlers.push_back(clydeHandler);

            addPelletsModelsToScene();
            addLivesModelToScene();

            generateModels(gameModelHandlers); // 4.73907 seconds to load these fuckers;
        }

        // Generate model matrix with translation rotation and scale using yaw pitch and roll for rotation and single float per uniform scaling;
        glm::mat4 generateModelMatrix(glm::vec3 position, float yaw, float pitch, float roll, float scale = 1.0f) {

			glm::mat4 model = glm::mat4(1.0f);

			model = glm::translate(model, position);
			model = glm::rotate(model, glm::radians(yaw), glm::vec3(0.0f, 1.0f, 0.0f));
			model = glm::rotate(model, glm::radians(pitch), glm::vec3(1.0f, 0.0f, 0.0f));
			model = glm::rotate(model, glm::radians(roll), glm::vec3(0.0f, 0.0f, 1.0f));
			model = glm::scale(model, glm::vec3(scale));

			return model;
		}

        // Add pellets to the scene;
        void addPelletsModelsToScene() {
            auto maze = envGenerator.mazeGenerator.getMaze();
            glm::vec3 centeredOffset = glm::vec3(maze.size() / 2.0f - 0.5f, 0.0f, maze[0].size() / 2.0f - 0.5f);
            pelletsInMaze.resize(maze.size());
            
            for (int i = 0; i < maze.size(); i++) {

                pelletsInMaze[i].resize(maze[0].size());
                for (int j = 0; j < maze[i].size(); j++) {

                    glm::vec3 pelletPosition = glm::vec3(i, 0.4f, j) - centeredOffset;
                    if (maze[i][j] == PELLET) {
                        PelletGenerator pelletGenerator = PelletGenerator(pelletPosition);

                        auto pelletHandler = std::make_shared<PelletModelHandler>(
                            glm::translate(glm::mat4(1.0f), pelletPosition),
                            "textures/test/GalvanisedSteel.png", i, j
                        );
                        pelletHandler->vertices = pelletGenerator.getPelletVertices();
                        pelletHandler->indices = pelletGenerator.getPelletIndices();
                        pelletHandler->pointsWhenEaten = 10.0f;

                        gameModelHandlers.push_back(pelletHandler);
                        pelletsInMaze[i][j] = std::make_tuple(pelletHandler, false);
                        howManyPelletsLeft++; totalPellets++;
                    }
                    else if (maze[i][j] == POWER_PELLET) {
                        PelletGenerator pelletGenerator = PelletGenerator(pelletPosition, true, 0.4f, 50.0f, POWER_PELLET_MAT);

                        auto pelletHandler = std::make_shared<PelletModelHandler>(
                            glm::translate(glm::mat4(1.0f), pelletPosition),
                            "textures/test/Gold.png", i, j
                        );
                        pelletHandler->vertices = pelletGenerator.getPelletVertices();
                        pelletHandler->indices = pelletGenerator.getPelletIndices();
                        pelletHandler->pointsWhenEaten = 50.0f;

                        gameModelHandlers.push_back(pelletHandler);
                        pelletsInMaze[i][j] = std::make_tuple(pelletHandler, false);
                    }
                    else { pelletsInMaze[i][j] = std::make_tuple(nullptr, true); }
                }
            }
        }

        // Variable with lives positions;
        std::unordered_map<int, std::vector<glm::vec3>> livesPosition = {
			{ 0, {} },
            { 1, { glm::vec3(7.85f, 0.55f, 0.0f) } },
			{ 2, { glm::vec3(7.85f, 0.55f, -0.1f), glm::vec3(7.85f, 0.55f, 0.1f) } },
			{ 3, { glm::vec3(7.85f, 0.55f, -0.2f), glm::vec3(7.85f, 0.55f, 0.0f), glm::vec3(7.85f, 0.55f, 0.2f) } },
            { 4, { glm::vec3(7.85f, 0.55f, -0.3f), glm::vec3(7.85f, 0.55f, -0.1f), glm::vec3(7.85f, 0.55f, 0.1f), glm::vec3(7.85f, 0.55f, 0.3f) } },
            { 5, { glm::vec3(7.85f, 0.55f, -0.4f), glm::vec3(7.85f, 0.55f, -0.2f), glm::vec3(7.85f, 0.55f, 0.0f), glm::vec3(7.85f, 0.55f, 0.2f), glm::vec3(7.85f, 0.55f, 0.4f) } },
            { 6, { glm::vec3(7.85f, 0.55f, -0.5f), glm::vec3(7.85f, 0.55f, -0.3f), glm::vec3(7.85f, 0.55f, -0.1f), glm::vec3(7.85f, 0.55f, 0.1f), glm::vec3(7.85f, 0.55f, 0.3f), glm::vec3(7.85f, 0.55f, 0.5f) } }
        };
        std::vector<std::shared_ptr<ModelHandler>> livesModels;
        
        // Add lives models to the scene;
        void addLivesModelToScene() {
            auto positions = livesPosition[livesLeft];
            for (const auto& position : positions) {
				auto lifeHandler = std::make_shared<CharacterMenuModelHandler>(
                    generateModelMatrix(position, 90.0, 0.0f, 0.0f, 0.035f),
                    "models/lowPolyHeart.obj",
					"textures/details/LowPolyHeart.png"
				);
				gameModelHandlers.push_back(lifeHandler);
                livesModels.push_back(lifeHandler);
			}

            auto ppSignLives = ParallelepGenerator(0.75f, 0.2596f, 0.1f, HUD_MAT);
            auto lifeSignHandler = std::make_shared<GameModelHandler>(
                glm::translate(glm::mat4(1.0f), glm::vec3(7.82f, 0.72f, 0.0f)),
                "textures/menus/LivesLeft4.png"
            );
            lifeSignHandler->vertices = ppSignLives.gateVertices;
            lifeSignHandler->indices = ppSignLives.gateIndices;
            gameModelHandlers.push_back(lifeSignHandler);
        }

        // Check if player collided with walls;
        void movePlayerAndCheckCollisionsWithWalls() {
            auto maze = envGenerator.mazeGenerator.getMaze();

            glm::vec3 currentPosition = viewCamera.position;
            glm::vec3 frontDirection = viewCamera.front;
            processInput(window); // Use the input received from keyboard this frame to update the view matrix;
            glm::vec3 nextPosition = viewCamera.position;

            glm::ivec2 playerPosInMaze = toGridCoordinates(nextPosition, maze.size(), maze[0].size());

            if ((playerPosInMaze.x == 13 || playerPosInMaze.x == 14 || playerPosInMaze.x == 15) && playerPosInMaze.y == 28) {
                SoundManager::playSound("pacman_teleport");
                viewCamera.setPosition(glm::vec3(-0.5f, pacmanHeight, 13.5f));
            }
            else if ((playerPosInMaze.x == 13 || playerPosInMaze.x == 14 || playerPosInMaze.x == 15) && playerPosInMaze.y == -1) {
                SoundManager::playSound("pacman_teleport");
                viewCamera.setPosition(glm::vec3(-0.5f, pacmanHeight, -13.5f));
            }
            else if (playerPosInMaze.x < 0 || playerPosInMaze.x > maze.size() || playerPosInMaze.y < 0 || playerPosInMaze.y > maze[0].size()) { return; }
            else if (maze[playerPosInMaze.x][playerPosInMaze.y] == WALL || maze[playerPosInMaze.x][playerPosInMaze.y] == GHOSTS_HUB) {

                glm::vec3 movement = nextPosition - currentPosition;
                glm::vec3 newPosition = currentPosition;

                newPosition.x = nextPosition.x;
                glm::ivec2 newPosInMaze = toGridCoordinates(newPosition, maze.size(), maze[0].size());
                if (isValidPosition(newPosInMaze, maze)) { viewCamera.setPosition(newPosition); return; }

                newPosition = currentPosition;
                newPosition.z = nextPosition.z;
                newPosInMaze = toGridCoordinates(newPosition, maze.size(), maze[0].size());
                if (isValidPosition(newPosInMaze, maze)) { viewCamera.setPosition(newPosition); return; }

                // If both horizontal and vertical failed, check diagonal (corner case);
                newPosition = currentPosition;
                glm::ivec2 cornerPosInMaze;
                if (movement.x > 0 && movement.z > 0) { cornerPosInMaze = playerPosInMaze + glm::ivec2(1, 1); }
                else if (movement.x > 0 && movement.z < 0) { cornerPosInMaze = playerPosInMaze + glm::ivec2(1, -1); }
                else if (movement.x < 0 && movement.z > 0) { cornerPosInMaze = playerPosInMaze + glm::ivec2(-1, 1); }
                else { cornerPosInMaze = playerPosInMaze + glm::ivec2(-1, -1); }

                if (!isValidPosition(cornerPosInMaze, maze)) { viewCamera.setPosition(currentPosition); }
            }
        }

		// Move the pacman model;
        void movePacmanModel() {
            pacmanModelHandler->modifyModelMatrix(viewCamera.position, viewCamera.front);
            // pacmanModelHandler->rotateModelMatrix(25.0f, glm::normalize(glm::cross(glm::normalize(viewCamera.front), glm::vec3(0.0f, 1.0f, 0.0f))));
        }

        // Update player position for sound;
        void updatePlayerPositionForSound() {
            SoundManager::engine->setListenerPosition(
                { viewCamera.position.x, viewCamera.position.y, viewCamera.position.z },
                { -viewCamera.front.x, -viewCamera.front.y, -viewCamera.front.z },
                { 0.0f, 0.0f, 0.0f },
                { viewCamera.up.x, viewCamera.up.y, viewCamera.up.z }
            );
        }

        // Check if player position is a valid position;
        bool isValidPosition(glm::ivec2 pos, const std::vector<std::vector<int>>& maze) {
            return !outOfBounds(pos, maze) && maze[pos.x][pos.y] != WALL && maze[pos.x][pos.y] != GHOSTS_HUB;
        }

        // Check if position is out of bounds;
        bool outOfBounds(glm::ivec2 pos, const std::vector<std::vector<int>>& maze) { return pos.x < 0 || pos.x >= maze.size() || pos.y < 0 || pos.y >= maze[0].size(); }

        // Convert world coordinates to grid coordinates;
        glm::ivec2 toGridCoordinates(const glm::vec3& worldPos, int height, int width) { return glm::ivec2(floor(worldPos.x) + height / 2, floor(- worldPos.z) + width / 2); }

        // Check if player collided with pellets;
        void checkForPlayerCollisionsWPellets() {

            auto maze = envGenerator.mazeGenerator.getMaze();
			glm::vec3 playerPosition = viewCamera.position;
			glm::ivec2 playerPosInMaze = glm::ivec2(floor(playerPosition.x) + maze.size() / 2, floor(playerPosition.z) + maze[0].size() / 2);

			if (maze[playerPosInMaze.x][playerPosInMaze.y] == PELLET) {
				if (!std::get<1>(pelletsInMaze[playerPosInMaze.x][playerPosInMaze.y])) {
                    SoundManager::queueSound("pacman_wakawaka");

					std::get<1>(pelletsInMaze[playerPosInMaze.x][playerPosInMaze.y]) = true;

                    playerLevelScore += std::dynamic_pointer_cast<PelletModelHandler>(std::get<0>(pelletsInMaze[playerPosInMaze.x][playerPosInMaze.y]))
                        ->pointsWhenEaten;

                    std::dynamic_pointer_cast<PelletModelHandler>(std::get<0>(pelletsInMaze[playerPosInMaze.x][playerPosInMaze.y]))
                        ->translatePellet(glm::vec3(0.0f, -10.0f, 0.0f));

                    howManyPelletsLeft --;
				}
			}
            else if (maze[playerPosInMaze.x][playerPosInMaze.y] == POWER_PELLET) {
                if (!std::get<1>(pelletsInMaze[playerPosInMaze.x][playerPosInMaze.y])) {

                    std::get<1>(pelletsInMaze[playerPosInMaze.x][playerPosInMaze.y]) = true;

                    playerLevelScore += std::dynamic_pointer_cast<PelletModelHandler>(std::get<0>(pelletsInMaze[playerPosInMaze.x][playerPosInMaze.y]))
                        ->pointsWhenEaten;

                    std::dynamic_pointer_cast<PelletModelHandler>(std::get<0>(pelletsInMaze[playerPosInMaze.x][playerPosInMaze.y]))
                        ->translatePellet(glm::vec3(0.0f, -10.0f, 0.0f));

                    powerPelletGotEaten();
                }
            }
        }

        // Change ghosts behaviour when power pellet gets eaten;
        void powerPelletGotEaten() {
            std::thread timerThread([this] {
                powerPelletEaten ++;
                if (!SoundManager::isSoundPlaying("pacman_power-pellet-eaten")) { SoundManager::playSoundLooped("pacman_power-pellet-eaten"); }

                float s = ghosts.speedModifier;
                ghosts.changeGhostsState(FRIGHTENED);
                ghosts.changeGhostsSpeedModifier(s * 0.92f);
                std::this_thread::sleep_for(std::chrono::seconds(static_cast<uint32_t>(ghosts.modeDuration)));

                powerPelletEaten --;
                if (powerPelletEaten == 0) {
                    ghosts.changeGhostsState(NORMAL);
                    ghosts.changeGhostsSpeedModifier(s);
                    if (SoundManager::isSoundPlaying("pacman_power-pellet-eaten")) { SoundManager::stopSound("pacman_power-pellet-eaten"); }
                }
            });

            timerThread.detach(); // Threads runs independently;
        }

        // Check if player collided with ghosts or finished pellets;
        void checkForEndGame() {
            checkForPlayerCollisionsWGhosts(); // Check for player collisions with ghosts;
            checkIfPlayerGotAllPellets(); // Check if player got all pellets;
        }

        // The ghosts radius of reach;
        float ghostsReach = 1.2f;

        // Check if player collided with ghosts;
        void checkForPlayerCollisionsWGhosts() { if (ghosts.checkIfGhostsGotPacman()) { SoundManager::playSound("pacman_death"); pacmanDefeated = true; } }

        // Check if player got all pellets;
        void checkIfPlayerGotAllPellets() { if (howManyPelletsLeft == 0) { hasPlayerWonCurrLevel = true; } }

        // Handle the game coroutines;
        void startGameCoroutines() {
            std::thread timerThread([this] { startGhostHandlerCoroutines(); });

            timerThread.detach();
        }

        // Handles ghosts release and scatter timings;
        void startGhostHandlerCoroutines() {
            ghosts.blinky->setSpeedModifier(ghosts.speedModifier);
            std::this_thread::sleep_for(std::chrono::seconds(4));

            ghosts.pinky->setSpeedModifier(ghosts.speedModifier);
            ghosts.inky->setSpeedModifier(ghosts.speedModifier);
            std::this_thread::sleep_for(std::chrono::seconds(6));

            ghosts.clyde->setSpeedModifier(ghosts.speedModifier);

            // All ghosts are now released, start scatter behaviour at random times;

            std::random_device rd; std::mt19937 gen(rd()); // Seed random number generator;
            std::uniform_int_distribution<> distrib(18, 24); // Random time between 18 and 24 seconds;

            while (!pacmanDefeated) {
                int randomTime = distrib(gen);
                std::this_thread::sleep_for(std::chrono::seconds(randomTime));

                if (ghosts.generalGhostState != FRIGHTENED) { ghosts.changeGhostsState(SCATTER); }
                std::this_thread::sleep_for(std::chrono::seconds(static_cast<uint32_t>(ghosts.modeDuration)));
                ghosts.changeGhostsState(NORMAL);
            }
        }

        // Handle pacman defeat;
        void pacmanGotEaten() {
            livesLeft--;

            if (playerInHighView) { playerInHighView = false; }

            if (livesLeft == 0) { isGameFinished = true; }
            else {
                SoundManager::playSound("pacman_respawn");

                viewCamera.reInitializateAll(
                    pacmanStartingPosition,
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    180.0f,
                    0.0f,
                    5.0f
                );
                std::thread makePlayerStayStill([] { viewCamera.pacmanGotEatenBehaviour(3); });
                makePlayerStayStill.detach();

                ghosts.resetAfterEatingPacman(3);
                pacmanDefeated = false;

                moveLivesModels();
            }
        }

        // Repositions lifes models when their number changes;
        void moveLivesModels() {
            auto lifeModel = livesModels[livesLeft];

            lifeModel->modelMatrix = glm::translate(lifeModel->modelMatrix, glm::vec3(0.0f, -100.0f, 0.0f));

            auto positions = livesPosition[livesLeft];
            for (int i = 0; i < livesLeft; i ++) {
                livesModels[i]->modelMatrix = generateModelMatrix(positions[i], 90.0, 0.0f, 0.0f, 0.035f);
            }
        }

        // Handle finishing a level and starting a new one;
        void setUpNewLevel() {
			currentLevel++;

            if (currentLevel == lastLevel) { isGameFinished = true; pacmanWon = true; }
			else {
				SoundManager::playSound("pacman_respawn");
                viewCamera.reInitializateAll(
                    pacmanStartingPosition,
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    glm::vec3(0.0f, 1.0f, 0.0f),
                    180.0f,
                    0.0f,
                    5.0f
                );

                std::thread makePlayerStayStill([] { viewCamera.pacmanGotEatenBehaviour(3); });
                makePlayerStayStill.detach();
                ghosts.resetAfterEatingPacman(8);

                for (auto& modelRow : pelletsInMaze) {
                    for (auto& [p, eaten] : modelRow) {
                        eaten = false;
                        if (p != nullptr) {
                            auto pp = std::dynamic_pointer_cast<PelletModelHandler>(p);
                            if (pp->modelMatrix[3].y < 0.0f) { pp->translatePellet(glm::vec3(0.0f, 10.0f, 0.0f)); }
                        }
					}
                }

                playerScore += playerLevelScore;
                playerLevelScore = 0.0f;

                ghosts.changeGhostsSpeedMod(ghosts.blinky->getSpeedModifier() + currentLevel * 1.0f);
                ghosts.changeGhostsModeDuration(ghosts.blinky->getModeDuration() - 0.5f);

                ghosts.changeGhostsState(NORMAL);
                if (SoundManager::isSoundPlaying("pacman_power-pellet-eaten")) { SoundManager::stopSound("pacman_power-pellet-eaten"); }

				startGameCoroutines();

                hasPlayerWonCurrLevel = false;
                playerInHighView = false;
				howManyPelletsLeft = totalPellets;
			}
        }
        


        // Cleanup all vulkan and non resources;
        void cleanup() {
            vkDestroyImageView(device, colorImageView, nullptr);
            vkDestroyImage(device, colorImage, nullptr);
            vkFreeMemory(device, colorImageMemory, nullptr);

            vkDestroyImageView(device, depthImageView, nullptr);
            vkDestroyImage(device, depthImage, nullptr);
            vkFreeMemory(device, depthImageMemory, nullptr);

            cleanupSwapChain();

            std::vector<std::vector<std::shared_ptr<ModelHandler>>> handlers = {
                startingMenuModelHandlers,
                gameModelHandlers,
                gameOverModelHandlers
            };

            // For each model, cleanup their variables;
            for (auto handler : handlers) {
                for (const auto& handlerPtr : handler) {

                    ModelHandler& handler = *handlerPtr;

                    vkDestroySampler(device, handler.textureSampler, nullptr);
                    vkDestroyImageView(device, handler.textureImageView, nullptr);
                    vkDestroyImage(device, handler.textureImage, nullptr);
                    vkFreeMemory(device, handler.textureImageMemory, nullptr);

                    // Clear uniform buffers;
                    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                        vkDestroyBuffer(device, handler.uniformBuffers[i], nullptr);
                        vkFreeMemory(device, handler.uniformBuffersMemory[i], nullptr);
                    }

                    // Old descriptorPool and descriptorSetLayout cleanup -- Check if still works

                    vkDestroyBuffer(device, handler.indexBuffer, nullptr); // Destroy vertex buffer;
                    vkFreeMemory(device, handler.indexBufferMemory, nullptr);
                    vkDestroyBuffer(device, handler.vertexBuffer, nullptr); // Destroy index buffer;
                    vkFreeMemory(device, handler.vertexBufferMemory, nullptr);
                }
            }

            vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr); // Clear descriptor set;
            vkDestroyDescriptorPool(device, descriptorPool, nullptr); // Destroy descriptor pool and sets with them;


            vkDestroyPipeline(device, graphicsPipeline, nullptr); // Destroy Pipeline;
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr); // Destroy PipelineLayout;
            vkDestroyRenderPass(device, renderPass, nullptr); // Destroy RenderPass;

            // Clear semaphores and fence;
            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(device, inFlightFences[i], nullptr);
            }

            vkDestroyCommandPool(device, commandPool, nullptr); // Destroy Command pool;
            vkDestroyDevice(device, nullptr); // Clear the virtual device created in createLogicalDevice();

            vkDestroySurfaceKHR(instance, surface, nullptr); // Destroy surfaces created;
            vkDestroyInstance(instance, nullptr); // Destroy Vulkan instance;

            glfwDestroyWindow(window); // Close window;
            glfwTerminate(); // Terminate GLFW;
        }

        // Create the vulkan instance;
        void createInstance() {

            // Check the presence of the needed validation layers with the function defined below;
            if (enableValidationLayers && !checkValidationLayerSupport()) { throw std::runtime_error("validation layers requested, but not available!"); }

            // OPTIONAL - Struct with informations about the application (many parameters in Vulkan are passed via structs and not as function parameters);
            VkApplicationInfo appInfo{};
            appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
            appInfo.pApplicationName = "Pacman3D";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "No Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_0;

            // REQUIRED - tell Vulkan which global extensions and validation layers we want to use;
            VkInstanceCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
            createInfo.pApplicationInfo = &appInfo;

            // Include the validation layers if they are enabled;
            if (enableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();
            }
            else { createInfo.enabledLayerCount = 0; }

            // Specify the wanted global extensions;
            uint32_t glfwExtensionCount = 0;
            const char** glfwExtensions;

            glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
            createInfo.enabledExtensionCount = glfwExtensionCount;
            createInfo.ppEnabledExtensionNames = glfwExtensions;

            // OPTIONAL - Retrieve a list of supported extension;
            uint32_t extensionCount = 0;
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
            std::vector<VkExtensionProperties> extensions(extensionCount);
            vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

            // Create the Vulkan instance and check for positive result;
            if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
                throw std::runtime_error("failed to create instance!");
            }
        }

        // Are validation layers requested available?;
        bool checkValidationLayerSupport() {
            uint32_t layerCount;
            vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
            std::vector<VkLayerProperties> availableLayers(layerCount);
            vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

            for (const char* layerName : validationLayers) {
                bool layerFound = false;
                for (const auto& layerProperties : availableLayers) {
                    if (strcmp(layerName, layerProperties.layerName) == 0) {
                        layerFound = true;
                        break;
                    }
                }
                if (!layerFound) { return false; }
            }
            return true;
        }

        static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
            const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {

            std::cerr << "Validation layer: " << pCallbackData->pMessage << std::endl;

            return VK_FALSE;
        }

        // Create the debug messenger
        void setupDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT& debugMessenger) {
            if (!enableValidationLayers) return;

            VkDebugUtilsMessengerCreateInfoEXT createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
            createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
            createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
            createInfo.pfnUserCallback = debugCallback;

            if (vkCreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
                throw std::runtime_error("failed to set up debug messenger!");
            }
        }



        // START1 - Physical Device: put this section in a separate area to cleanuo the code (another class maybe);

        // Check for presence of physical devices in the pc, then choosing one
        void pickPhysicalDevice() {
            uint32_t deviceCount = 0;
            vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
            if (deviceCount == 0) { throw std::runtime_error("failed to find GPUs with Vulkan support!"); }

            std::vector<VkPhysicalDevice> devices(deviceCount);
            vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

            if (deviceCount == 0) { throw std::runtime_error("failed to find GPUs with Vulkan support!"); }
            for (const auto& device : devices) {
                if (isDeviceSuitable(device)) {
                    physicalDevice = device;
                    msaaSamples = getMaxUsableSampleCount();
                    break;
                }
            }
            if (physicalDevice == VK_NULL_HANDLE) { throw std::runtime_error("failed to find a suitable GPU!"); }
        }

        // Check if device is suitable based on the queue families it supports;
        bool isDeviceSuitable(VkPhysicalDevice device) {
            QueueFamilyIndices indices = findQueueFamilies(device);
            bool extensionsSupported = checkDeviceExtensionSupport(device);
            bool swapChainAdequate = false;
            if (extensionsSupported) {
                SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
                swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
            }
            VkPhysicalDeviceFeatures supportedFeatures;
            vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

            return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy;
        }

        // Checking if swapchain are supported on this device;
        bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
            uint32_t extensionCount;
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);
            std::vector<VkExtensionProperties> availableExtensions(extensionCount);
            vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

            std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());
            for (const auto& extension : availableExtensions) { requiredExtensions.erase(extension.extensionName); }

            return requiredExtensions.empty();
        }

        // Query for device properties to handle swapchain;
        SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
            SwapChainSupportDetails details;
            vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

            uint32_t formatCount;
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
            if (formatCount != 0) {
                details.formats.resize(formatCount);
                vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
            }

            uint32_t presentModeCount;
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
            if (presentModeCount != 0) {
                details.presentModes.resize(presentModeCount);
                vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
            }

            return details;
        }

        // Find which queue families are supported by the device and which supports the command we want to use. (For now it finds the graphics queue family);
        QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
            QueueFamilyIndices indices;

            uint32_t queueFamilyCount = 0;
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

            std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
            vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

            int i = 0;
            for (const auto& queueFamily : queueFamilies) {
                if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) { indices.graphicsFamily = i; }
                if (indices.isComplete()) { break; }

                // Search for queue families that have the capability of presenting to our window surface;
                VkBool32 presentSupport = false;
                vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
                if (presentSupport) { indices.presentFamily = i; }

                i++;
            }
            return indices;
        }

        // END1



        // START2 - Virtual Device

        // Create the logical device;
        void createLogicalDevice() {
            QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

            std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
            std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

            float queuePriority = 1.0f;
            for (uint32_t queueFamily : uniqueQueueFamilies) {
                VkDeviceQueueCreateInfo queueCreateInfo{};
                queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
                queueCreateInfo.queueFamilyIndex = queueFamily;
                queueCreateInfo.queueCount = 1;
                queueCreateInfo.pQueuePriorities = &queuePriority;
                queueCreateInfos.push_back(queueCreateInfo);
            }

            VkPhysicalDeviceFeatures deviceFeatures{};
            deviceFeatures.samplerAnisotropy = VK_TRUE;
            deviceFeatures.sampleRateShading = VK_TRUE;

            VkDeviceCreateInfo createInfo{};

            createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
            createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
            createInfo.pQueueCreateInfos = queueCreateInfos.data();
            createInfo.pEnabledFeatures = &deviceFeatures;
            createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
            createInfo.ppEnabledExtensionNames = deviceExtensions.data();

            // Set this even if ignored in up-to-date versions (Retrocompatibility of what?);
            if (enableValidationLayers) {
                createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
                createInfo.ppEnabledLayerNames = validationLayers.data();
            }
            else { createInfo.enabledLayerCount = 0; }

            // vkCreateDevice(physical_device, queue and usage info, allocation pointer optional and pointer to variable where to store the device);
            if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) { throw std::runtime_error("failed to create logical device!"); }

            // Retrieve the queue handles for both the graphics and presentation queues;
            vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
            vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
        }

        // END2 _________________________________________________________________________________________________________________________________________________________



        // START3 - Surfaces

        // Use glfwCreateWindowSurface() to create the right surface for the current OS where app is running;
        void createSurface() {
            if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) { throw std::runtime_error("failed to create window surface!"); }
        }

        // END3 ________________________________________________________________________________________________________________________________________________________



        // START4 - Swapchain

        void createSwapChain() {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

            VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
            VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
            VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

            // How many images in the swapchain;
            uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;

            // Make sure to not exceed max num of images;
            if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
                imageCount = swapChainSupport.capabilities.maxImageCount;
            }

            // Create the swapchain with all its details;
            VkSwapchainCreateInfoKHR createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
            createInfo.surface = surface;
            createInfo.minImageCount = imageCount;
            createInfo.imageFormat = surfaceFormat.format;
            createInfo.imageColorSpace = surfaceFormat.colorSpace;
            createInfo.imageExtent = extent;
            createInfo.imageArrayLayers = 1;
            createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

            QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
            uint32_t queueFamilyIndices[] = { indices.graphicsFamily.value(), indices.presentFamily.value() };

            if (indices.graphicsFamily != indices.presentFamily) {
                createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
                createInfo.queueFamilyIndexCount = 2;
                createInfo.pQueueFamilyIndices = queueFamilyIndices;
            }
            else {
                createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
                createInfo.queueFamilyIndexCount = 0; // Optional
                createInfo.pQueueFamilyIndices = nullptr; // Optional
            }

            createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
            createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
            createInfo.presentMode = presentMode;
            createInfo.clipped = VK_TRUE;
            createInfo.oldSwapchain = VK_NULL_HANDLE;

            if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) { throw std::runtime_error("failed to create swap chain!"); }

            vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
            swapChainImages.resize(imageCount);
            vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

            // Save parameters chosen;
            swapChainImageFormat = surfaceFormat.format;
            swapChainExtent = extent;
        }

        // Pick the color space we want to use (and check if it is available in this device);
        VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
            for (const auto& availableFormat : availableFormats) {
                if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                    return availableFormat;
                }
            }
            return availableFormats[0];
        }

        // Pick best possible presentation mode: VK_PRESENT_MODE_MAILBOX_KHR is triple buffering;
        VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
            for (const auto& availablePresentMode : availablePresentModes) {
                if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) { return availablePresentMode; }
            }
            return VK_PRESENT_MODE_FIFO_KHR;
        }

        // Set the choosed swap extent, if currentExtent.width != max then we use the current one, otherwise we pick the best one for the current window size;
        VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
            if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) { return capabilities.currentExtent; }
            else {
                int width, height;
                glfwGetFramebufferSize(window, &width, &height);

                VkExtent2D actualExtent = {
                    static_cast<uint32_t>(width),
                    static_cast<uint32_t>(height)
                };

                // Clamp to limit the result to the minimum and maximum values supported by the implementation;
                actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
                actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

                return actualExtent;
            }
        }

        // END4 __________________________________________________________________________________________________________________________________



        // START5 - Image views

        void createImageViews() {
            swapChainImageViews.resize(swapChainImages.size());
            for (uint32_t i = 0; i < swapChainImages.size(); i++) { swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1); }
        }

        // END5 _________________________________________________________________________________________________________________



        // START6 - Pipeline

        // Create the graphics pipeline;
        void createGraphicsPipeline() {

            // Load the shaders;
            auto vertShaderCode = readFile("shaders/Test/ShaderVert.spv");
            auto fragShaderCode = readFile("shaders/Test/ShaderFrag.spv");

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


            // Rasterizer, creates the fragments from the geometry created by the vertex shader;
            VkPipelineRasterizationStateCreateInfo rasterizer{};
            rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
            rasterizer.depthClampEnable = VK_FALSE; // Enable clamping? If yes a GPU feature is required;
            rasterizer.rasterizerDiscardEnable = VK_FALSE; // If set to true then jump rasterization phase;
            rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // Define how to generate fragments from the geometry (any other mode not FILL requires GPU feature);
            rasterizer.lineWidth = 1.0f; // As the others, anything wider than 1.0f requires GPU feature calles "wideLines";
            rasterizer.cullMode = VK_CULL_MODE_NONE; // What type of culling to use;
            rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; // What direction to use to determine if the faces is front or back in culling;
            // Can be used to alter the depth values, won't be using it in the tutorial;
            rasterizer.depthBiasEnable = VK_FALSE;
            rasterizer.depthBiasConstantFactor = 0.0f; // Optional
            rasterizer.depthBiasClamp = 0.0f; // Optional
            rasterizer.depthBiasSlopeFactor = 0.0f; // Optional


            // Enable multisampling, one of the ways to perform anti-aliasing (here is disabled, we'll see it later);
            VkPipelineMultisampleStateCreateInfo multisampling{};
            multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
            multisampling.sampleShadingEnable = VK_TRUE;
            multisampling.rasterizationSamples = msaaSamples;
            multisampling.minSampleShading = 0.2f;
            multisampling.pSampleMask = nullptr; // Optional
            multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
            multisampling.alphaToOneEnable = VK_FALSE; // Optional


            // Color bending configuration;
            VkPipelineColorBlendAttachmentState colorBlendAttachment{};
            colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            colorBlendAttachment.blendEnable = VK_FALSE;
            colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA; // Optional
            colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA; // Optional
            colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
            colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
            colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
            colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

            VkPipelineColorBlendStateCreateInfo colorBlending{};
            colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
            colorBlending.logicOpEnable = VK_FALSE;
            colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
            colorBlending.attachmentCount = 1;
            colorBlending.pAttachments = &colorBlendAttachment;
            colorBlending.blendConstants[0] = 0.0f; // Optional
            colorBlending.blendConstants[1] = 0.0f; // Optional
            colorBlending.blendConstants[2] = 0.0f; // Optional
            colorBlending.blendConstants[3] = 0.0f; // Optional

            VkPipelineDepthStencilStateCreateInfo depthStencil{};
            depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
            depthStencil.depthTestEnable = VK_TRUE;
            depthStencil.depthWriteEnable = VK_TRUE;
            depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
            depthStencil.depthBoundsTestEnable = VK_FALSE;
            depthStencil.minDepthBounds = 0.0f; // Optional
            depthStencil.maxDepthBounds = 1.0f; // Optional
            depthStencil.stencilTestEnable = VK_FALSE;
            depthStencil.front = {}; // Optional
            depthStencil.back = {}; // Optional

            // Create pipelineLayout;
            VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
            pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
            pipelineLayoutInfo.setLayoutCount = 1; // Optional
            pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
            pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
            pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

            if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) { throw std::runtime_error("failed to create pipeline layout!"); }

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
            pipelineInfo.layout = pipelineLayout;
            pipelineInfo.renderPass = renderPass;
            pipelineInfo.subpass = 0;
            pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
            pipelineInfo.basePipelineIndex = -1; // Optional

            // Create pipeline;
            if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics pipeline!");
            }


            // Destroy shader modules;
            vkDestroyShaderModule(device, fragShaderModule, nullptr);
            vkDestroyShaderModule(device, vertShaderModule, nullptr);
        }

        // Create shader module, used to pass the shader to the pipeline;
        VkShaderModule createShaderModule(const std::vector<char>& code) {
            VkShaderModuleCreateInfo createInfo{};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.codeSize = code.size();
            createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

            VkShaderModule shaderModule;
            if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) { throw std::runtime_error("failed to create shader module!"); }

            return shaderModule;
        }

        // Render pass used to set how many color and depth buffers there will be, how many samples to use for each of them and how their contents should be handled throughout the rendering operations;
        void createRenderPass() {
            // Describe a single color buffer attachment;
            VkAttachmentDescription colorAttachment{};
            colorAttachment.format = swapChainImageFormat; // Match the format of the swap chain images;
            colorAttachment.samples = msaaSamples; // No multisampling;
            colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Clear the attachment at the start of the render pass;
            colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store the rendered contents for later reading;
            colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // We don't care about stencil data;
            colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // We don't care about stencil data;
            colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Don't care about initial layout (contents won't be preserved);
            colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout for presenting to the swap chain;

            // Reference to the color attachment in a subpass;
            VkAttachmentReference colorAttachmentRef{};
            colorAttachmentRef.attachment = 0; // Index of the attachment in the attachment descriptions array;
            colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Optimal layout for a color attachment;

            // Describe the depth of the image;
            VkAttachmentDescription depthAttachment{};
            depthAttachment.format = findDepthFormat();
            depthAttachment.samples = msaaSamples;
            depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            // Reference to the depth attachment;
            VkAttachmentReference depthAttachmentRef{};
            depthAttachmentRef.attachment = 1;
            depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

            // Color attachment;
            VkAttachmentDescription colorAttachmentResolve{};
            colorAttachmentResolve.format = swapChainImageFormat;
            colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
            colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
            colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

            VkAttachmentReference colorAttachmentResolveRef{};
            colorAttachmentResolveRef.attachment = 2;
            colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

            // Describe a single subpass;
            VkSubpassDescription subpass{};
            subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS; // This is a graphics subpass;
            subpass.colorAttachmentCount = 1; // We have one color attachment;
            subpass.pColorAttachments = &colorAttachmentRef; // Reference to the color attachment;
            subpass.pDepthStencilAttachment = &depthAttachmentRef; // Reference to the depth attachment;
            subpass.pResolveAttachments = &colorAttachmentResolveRef;


            // Create subpass dependencies;
            VkSubpassDependency dependency{};
            dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
            dependency.dstSubpass = 0;
            dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.srcAccessMask = 0;
            dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


            // Create the render pass with one attachment and one subpass;
            std::array<VkAttachmentDescription, 3> attachments = { colorAttachment, depthAttachment, colorAttachmentResolve };

            VkRenderPassCreateInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
            renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size()); // Number of attachments;
            renderPassInfo.pAttachments = attachments.data(); // Pointer to the attachment description;
            renderPassInfo.subpassCount = 1; // Number of subpasses;
            renderPassInfo.pSubpasses = &subpass; // Pointer to the subpass description;
            renderPassInfo.dependencyCount = 1; // How many dependencies;
            renderPassInfo.pDependencies = &dependency; // Pointer to dependencies;

            // Create the render pass
            if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) { throw std::runtime_error("failed to create render pass!"); }
        }

        // END6 ______________________________________________________________________________________________________________



        // START7 - FrameBuffer

        void createFramebuffers() {
            swapChainFramebuffers.resize(swapChainImageViews.size()); // Size to hold all framebuffers;

            // Create framebuffers from image views;
            for (size_t i = 0; i < swapChainImageViews.size(); i++) {
                std::array<VkImageView, 3> attachments = {
                    colorImageView,
                    depthImageView,
                    swapChainImageViews[i]
                };

                VkFramebufferCreateInfo framebufferInfo{};
                framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
                framebufferInfo.renderPass = renderPass;
                framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
                framebufferInfo.pAttachments = attachments.data();
                framebufferInfo.width = swapChainExtent.width;
                framebufferInfo.height = swapChainExtent.height;
                framebufferInfo.layers = 1;

                if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) { throw std::runtime_error("failed to create framebuffer!"); }
            }
        }

        // END7 _______________________________________________________________________________________________________________________



        // START8 - Command pool & Command Buffer;

        void createCommandPool() {
            QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

            VkCommandPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
            poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT; // Allow command buffers to be rerecorder individually;
            poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

            if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) { throw std::runtime_error("failed to create command pool!"); }
        }


        void createCommandBuffers() {
            commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.commandPool = commandPool;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

            if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) { throw std::runtime_error("failed to allocate command buffers!"); }
        }

        // Writes command we want inside the buffer;
        void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = 0; // Optional - How are we going to use the command buffer;
            beginInfo.pInheritanceInfo = nullptr; // Optional - Only for second level buffers;

            if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) { throw std::runtime_error("failed to begin recording command buffer!"); }

            // Start the render pass;
            VkRenderPassBeginInfo renderPassInfo{};
            renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            renderPassInfo.renderPass = renderPass;
            renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex]; // Pick right framebuffer for current swapchain image;
            renderPassInfo.renderArea.offset = { 0, 0 }; // Size of render area;
            renderPassInfo.renderArea.extent = swapChainExtent;

            std::array<VkClearValue, 2> clearValues{};
            clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
            clearValues[1].depthStencil = { 1.0f, 0 };

            renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
            renderPassInfo.pClearValues = clearValues.data();

            vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE); // Being render pass: VK_SUBPASS_CONTENTS_INLINE used to use only first level framebuffers;
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline); // Bind graphics pipeline;

            for (const auto& handlerPtr : modelHandlers) {

                ModelHandler& handler = *handlerPtr;

                VkBuffer vertexBuffers[] = { handler.vertexBuffer };
                VkDeviceSize offsets[] = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
                vkCmdBindIndexBuffer(commandBuffer, handler.indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1,
                    &handler.descriptorSets[currentFrame], 0, nullptr);
                vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(handler.indices.size()), 1, 0, 0, 0);
            }



            // Set dynamic viewport;
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = static_cast<float>(swapChainExtent.width);
            viewport.height = static_cast<float>(swapChainExtent.height);
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            // Set dynamic scissor;
            VkRect2D scissor{};
            scissor.offset = { 0, 0 };
            scissor.extent = swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

            vkCmdEndRenderPass(commandBuffer);
            if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) { throw std::runtime_error("failed to record command buffer!"); }
        }

        // END8 ________________________________________________________________________________________________________________________________



        // START9 - Draw Frame;

        // Draw the frames of our app;
        void drawFrame() {

            // Used to not go in a deadlock using fences;
            vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

            uint32_t imageIndex;
            VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex); // Get image from swapchain;

            if (result == VK_ERROR_OUT_OF_DATE_KHR) { recreateSwapChain(); return; }
            else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) { throw std::runtime_error("failed to acquire swap chain image!"); }

            // Ensure that the previous frame using this image (if any) has finished executing
            if (imagesInFlight[imageIndex] != VK_NULL_HANDLE) { vkWaitForFences(device, 1, &imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX); }
            imagesInFlight[imageIndex] = inFlightFences[currentFrame];

            // Update uniform buffers;
            updateGlobalUniformBuffer(currentFrame);
            for (const auto& handler : modelHandlers) { updateUniformBuffer(*handler, currentFrame); } // Update uniform buffer;
            

            // Only reset the fence if we are submitting work;
            vkResetFences(device, 1, &inFlightFences[currentFrame]);
            vkResetCommandBuffer(commandBuffers[currentFrame], 0); // Reset the command buffer for recording new commands;


            // RENDER WORLD;
            recordCommandBuffer(commandBuffers[currentFrame], imageIndex); // Record commandsBuffer once for each model. This also calls the Draw function;

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

            // Semaphores to wait on before execution (Image available);
            VkSemaphore waitSemaphores[] = { imageAvailableSemaphores[currentFrame] };
            VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
            submitInfo.waitSemaphoreCount = 1;
            submitInfo.pWaitSemaphores = waitSemaphores;
            submitInfo.pWaitDstStageMask = waitStages;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

            // Semaphores to signal after execution (Render finished);
            VkSemaphore signalSemaphores[] = { renderFinishedSemaphores[currentFrame] };
            submitInfo.signalSemaphoreCount = 1;
            submitInfo.pSignalSemaphores = signalSemaphores;

            // Only one vkQueueSubmit needed;
            if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) { throw std::runtime_error("failed to submit draw command buffers!"); }



            // Present the image to the swap chain for display;
            VkPresentInfoKHR presentInfo{};
            presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
            presentInfo.waitSemaphoreCount = 1;
            presentInfo.pWaitSemaphores = signalSemaphores;

            VkSwapchainKHR swapChains[] = { swapChain };
            presentInfo.swapchainCount = 1;
            presentInfo.pSwapchains = swapChains;
            presentInfo.pImageIndices = &imageIndex;
            presentInfo.pResults = nullptr;

            result = vkQueuePresentKHR(presentQueue, &presentInfo); // Submit request to present image to swapchain;

            if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) { framebufferResized = false; recreateSwapChain(); } // Recreate swapchain;
            else if (result != VK_SUCCESS) { throw std::runtime_error("failed to present swap chain image!"); }

            currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
        }


        void createSyncObjects() {
            imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
            inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
            imagesInFlight.resize(swapChainImages.size(), VK_NULL_HANDLE);

            VkSemaphoreCreateInfo semaphoreInfo{};
            semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

            VkFenceCreateInfo fenceInfo{};
            fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
            fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                    vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                    vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {

                    throw std::runtime_error("failed to create synchronization objects for a frame!");
                }
            }
        }

        // END9 ________________________________________________________________________________________________________________________________



        // START10 - Recreate swapchain;
        void recreateSwapChain() {
            int width = 0, height = 0;
            while (width == 0 || height == 0) {
                glfwGetFramebufferSize(window, &width, &height);
                glfwWaitEvents();
            }

            vkDeviceWaitIdle(device);

            cleanupSwapChain();

            createSwapChain();
            createImageViews();
            createColorResources();
            createDepthResources();
            createFramebuffers();
        }


        void cleanupSwapChain() {
            for (size_t i = 0; i < swapChainFramebuffers.size(); i++) { vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr); }
            for (size_t i = 0; i < swapChainImageViews.size(); i++) { vkDestroyImageView(device, swapChainImageViews[i], nullptr); }
            vkDestroySwapchainKHR(device, swapChain, nullptr);
        }

        // END10 __________________________________________________________________________________________________________________________________________________________________________________



        // START11 - Resize Window;

        static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
            auto app = reinterpret_cast<Pacman3D*>(glfwGetWindowUserPointer(window));
            app->framebufferResized = true;
        }

        // END11 ___________________________________________________________________________________________________________________________________________________________________________________



        // START12 - Vertex Buffer & Index Buffer;

        // Create vertex buffer;
        void createVertexBuffer(ModelHandler& handler) {
            VkDeviceSize bufferSize = sizeof(handler.vertices[0]) * handler.vertices.size();

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            // Transfer data from the const we defined to the buffer;
            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, handler.vertices.data(), (size_t)bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);

            createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, handler.vertexBuffer, handler.vertexBufferMemory);
            copyBuffer(stagingBuffer, handler.vertexBuffer, bufferSize);

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }

        // Create index buffer;
        void createIndexBuffer(ModelHandler& handler) {
            VkDeviceSize bufferSize = sizeof(handler.indices[0]) * handler.indices.size();

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
            memcpy(data, handler.indices.data(), (size_t)bufferSize);
            vkUnmapMemory(device, stagingBufferMemory);

            createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, handler.indexBuffer, handler.indexBufferMemory);

            copyBuffer(stagingBuffer, handler.indexBuffer, bufferSize);

            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }

        // Create generic buffer;
        void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
            VkBufferCreateInfo bufferInfo{};
            bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
            bufferInfo.size = size;
            bufferInfo.usage = usage;
            bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

            if (vkCreateBuffer(device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) { throw std::runtime_error("failed to create buffer!"); }

            VkMemoryRequirements memRequirements;
            vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) { throw std::runtime_error("failed to allocate buffer memory!"); }

            vkBindBufferMemory(device, buffer, bufferMemory, 0);
        }

        // Use to copy data between buffers;
        void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
            VkCommandBuffer commandBuffer = beginSingleTimeCommands();

            VkBufferCopy copyRegion{};
            copyRegion.size = size;
            vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

            endSingleTimeCommands(commandBuffer);
        }

        // Handle the memory request to the GPU by finding the right type of memory to use;
        uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
            VkPhysicalDeviceMemoryProperties memProperties;
            vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

            for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) { if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) { return i; } }
            throw std::runtime_error("failed to find suitable memory type!");
        }

        // END12 ______________________________________________________________________________________________________________________________________________________________________________



        // START13 - Matrices for vertices;

        void createDescriptorSetLayout() {

            VkDescriptorSetLayoutBinding uboLayoutBinding { };
            uboLayoutBinding.binding = 0;
            uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            uboLayoutBinding.descriptorCount = 1; // Values in array of uniform buffer objects;
            uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT; // What stage is it referenced in;
            uboLayoutBinding.pImmutableSamplers = nullptr; // Optional - only for image sampling;

            VkDescriptorSetLayoutBinding guboLayoutBinding { };
            guboLayoutBinding.binding = 1;
            guboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            guboLayoutBinding.descriptorCount = 1;
            guboLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            guboLayoutBinding.pImmutableSamplers = nullptr;

            VkDescriptorSetLayoutBinding samplerLayoutBinding { };
            samplerLayoutBinding.binding = 2;
            samplerLayoutBinding.descriptorCount = 1;
            samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            samplerLayoutBinding.pImmutableSamplers = nullptr;
            samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

            std::array<VkDescriptorSetLayoutBinding, 3> bindings = { uboLayoutBinding, guboLayoutBinding, samplerLayoutBinding };

            VkDescriptorSetLayoutCreateInfo layoutInfo{};
            layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
            layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
            layoutInfo.pBindings = bindings.data();

            if (vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout) != VK_SUCCESS) { throw std::runtime_error("failed to create descriptor set layout!"); }
        }

        // Create uniform buffers;
        void createUniformBuffers(ModelHandler& handler) {
            VkDeviceSize bufferSize = sizeof(UniformBufferObject);

            handler.uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
            handler.uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
            handler.uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, handler.uniformBuffers[i], handler.uniformBuffersMemory[i]); // Create buffer;
                vkMapMemory(device, handler.uniformBuffersMemory[i], 0, bufferSize, 0, &handler.uniformBuffersMapped[i]); // Map buffer just after creation to get pointer to it (persistent mapping);
            }
        }

        // Create global uniform buffers;
        void createGlobalUniformBuffers() {
            VkDeviceSize bufferSize = sizeof(GlobalUniformBufferObject);

            globalUniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
            globalUniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
            globalUniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                    VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, globalUniformBuffers[i], globalUniformBuffersMemory[i]);
                vkMapMemory(device, globalUniformBuffersMemory[i], 0, bufferSize, 0, &globalUniformBuffersMapped[i]);
            }
        }

        // Update uniform buffer;
        void updateUniformBuffer(ModelHandler& handler, uint32_t currentImage) {

            UniformBufferObject ubo { };

            ubo.model = handler.getModelMatrix(); // Model matrix;
            ubo.modelNorm = glm::inverse(glm::transpose(ubo.model)); // Normal model matrix;

            ubo.view = viewCamera.getViewMatrix(); // View matrix;

            ubo.proj = glm::perspective(glm::radians(60.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 100.0f); // Perspective proj;
            ubo.proj[1][1] *= -1; // OpenGL standard to Vulkan;

            memcpy(handler.uniformBuffersMapped[currentImage], &ubo, sizeof(ubo)); // Copy the UBO in the uniform buffer;
        }

        // Update uniform buffer;
        void updateGlobalUniformBuffer(uint32_t currentImage) {

            GlobalUniformBufferObject gubo { };

            gubo.viewerPos = viewCamera.position; // Viewer eyes position;

            gubo.ambientLightDirection = glm::vec3(glm::sin(glm::dot(viewCamera.position, glm::vec3(1.0f))));
            gubo.ambientLightColor = glm::vec3(1.0f, 1.0f, 1.0f); // Ambient light color, right now just white;

            for (int i = 0; i < gubo.pointLightCount; i++) {
                gubo.pointLightPos[i] = glm::vec3(8.5f, 0.0f, i + 0.5f);
                gubo.pointLightColors[i] = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            } // Lights do not *** work;

            auto ghostsPosTuples = ghosts.getGhostsPositions();
            std::vector<glm::vec3> ghostsPos;
            for (const auto& [first, _] : ghostsPosTuples) { ghostsPos.push_back(first); }

            std::vector<glm::vec3> ghostsColors = ghosts.getGhostsColors();

            for (int i = 0; i < gubo.ghostLightCount; i++) {
                gubo.ghostsLightsPos[i] = ghostsPos[i];
                gubo.ghostsLightsColors[i] = glm::vec4(ghostsColors[i], 1.0f);
            }

            memcpy(globalUniformBuffersMapped[currentImage], &gubo, sizeof(gubo));
        }


        // END13 ______________________________________________________________________________________________________________________________________________________________________________



        // START14 - Descriptor pool and sets;

        void createDescriptorPool(std::vector<std::shared_ptr<ModelHandler>>& thisModelHandler) {

            size_t totalDescriptorSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * thisModelHandler.size();

            VkDescriptorPoolSize poolSize{};
            poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSize.descriptorCount = totalDescriptorSets;

            std::array<VkDescriptorPoolSize, 3> poolSizes{};
            poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
            poolSizes[2].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSizes[2].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

            VkDescriptorPoolCreateInfo poolInfo{};
            poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
            poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
            poolInfo.pPoolSizes = poolSizes.data();
            poolInfo.maxSets = totalDescriptorSets;

            if (vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) { throw std::runtime_error("failed to create descriptor pool!"); }
        }


        void createDescriptorSets(ModelHandler& handler) {

            std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
            VkDescriptorSetAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
            allocInfo.descriptorPool = descriptorPool;
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

                VkDescriptorBufferInfo globalBufferInfo{};
                globalBufferInfo.buffer = globalUniformBuffers[i];
                globalBufferInfo.offset = 0;
                globalBufferInfo.range = sizeof(GlobalUniformBufferObject);

                VkDescriptorImageInfo imageInfo{};
                imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                imageInfo.imageView = handler.textureImageView;
                imageInfo.sampler = handler.textureSampler;

                std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

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
                descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                descriptorWrites[1].descriptorCount = 1;
                descriptorWrites[1].pBufferInfo = &globalBufferInfo;

                descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrites[2].dstSet = handler.descriptorSets[i];
                descriptorWrites[2].dstBinding = 2;
                descriptorWrites[2].dstArrayElement = 0;
                descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrites[2].descriptorCount = 1;
                descriptorWrites[2].pImageInfo = &imageInfo;

                vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
            }
        }


        // END14 ______________________________________________________________________________________________________________________________________________________________________________



        // START15 - Load images with stb;

        void createTextureImage(ModelHandler& handler) {

            std::string texturePath = handler.texturePath != "" ? handler.texturePath : BASE_TEXTURE_PATH;

            int texWidth, texHeight, texChannels;
            stbi_uc* pixels = stbi_load(texturePath.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
            handler.mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
            VkDeviceSize imageSize = texWidth * texHeight * 4;

            if (!pixels) { throw std::runtime_error("failed to load texture image!"); }

            VkBuffer stagingBuffer;
            VkDeviceMemory stagingBufferMemory;
            createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

            void* data;
            vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
            memcpy(data, pixels, static_cast<size_t>(imageSize));
            vkUnmapMemory(device, stagingBufferMemory);

            stbi_image_free(pixels);

            createImage(texWidth, texHeight, handler.mipLevels, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, handler.textureImage, handler.textureImageMemory);

            transitionImageLayout(handler.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, handler.mipLevels);
            copyBufferToImage(stagingBuffer, handler.textureImage, static_cast<uint32_t>(texWidth), static_cast<uint32_t>(texHeight));
            generateMipmaps(handler.textureImage, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, handler.mipLevels);

            // Cleanup
            vkDestroyBuffer(device, stagingBuffer, nullptr);
            vkFreeMemory(device, stagingBufferMemory, nullptr);
        }


        void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling,
            VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {

            VkImageCreateInfo imageInfo{};
            imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
            imageInfo.imageType = VK_IMAGE_TYPE_2D;
            imageInfo.extent.width = width;
            imageInfo.extent.height = height;
            imageInfo.extent.depth = 1;
            imageInfo.mipLevels = 1;
            imageInfo.arrayLayers = 1;
            imageInfo.format = format;
            imageInfo.tiling = tiling;
            imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            imageInfo.usage = usage;
            imageInfo.samples = numSamples;
            imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
            imageInfo.mipLevels = mipLevels;

            if (vkCreateImage(device, &imageInfo, nullptr, &image) != VK_SUCCESS) { throw std::runtime_error("failed to create image!"); }

            VkMemoryRequirements memRequirements;
            vkGetImageMemoryRequirements(device, image, &memRequirements);

            VkMemoryAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
            allocInfo.allocationSize = memRequirements.size;
            allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

            if (vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) { throw std::runtime_error("failed to allocate image memory!"); }

            vkBindImageMemory(device, image, imageMemory, 0);
        }


        VkCommandBuffer beginSingleTimeCommands() {
            VkCommandBufferAllocateInfo allocInfo{};
            allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
            allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
            allocInfo.commandPool = commandPool;
            allocInfo.commandBufferCount = 1;

            VkCommandBuffer commandBuffer;
            vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

            VkCommandBufferBeginInfo beginInfo{};
            beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

            vkBeginCommandBuffer(commandBuffer, &beginInfo);

            return commandBuffer;
        }


        void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
            vkEndCommandBuffer(commandBuffer);

            VkSubmitInfo submitInfo{};
            submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            submitInfo.commandBufferCount = 1;
            submitInfo.pCommandBuffers = &commandBuffer;

            vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
            vkQueueWaitIdle(graphicsQueue);

            vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
        }


        void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
            VkCommandBuffer commandBuffer = beginSingleTimeCommands();

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = oldLayout;
            barrier.newLayout = newLayout;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = image;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = mipLevels;

            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
                if (hasStencilComponent(format)) { barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT; }
            }
            else { barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT; }

            if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
                destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
            }
            else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
                barrier.srcAccessMask = 0;
                barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

                sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
                destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            }
            else { throw std::invalid_argument("unsupported layout transition!"); }

            vkCmdPipelineBarrier(
                commandBuffer,
                sourceStage,
                destinationStage,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            endSingleTimeCommands(commandBuffer);
        }


        void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
            VkCommandBuffer commandBuffer = beginSingleTimeCommands();

            VkBufferImageCopy region{};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = { 0, 0, 0 };
            region.imageExtent = { width, height, 1 };

            vkCmdCopyBufferToImage(
                commandBuffer,
                buffer,
                image,
                VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1,
                &region
            );

            endSingleTimeCommands(commandBuffer);
        }


        VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
            VkImageViewCreateInfo viewInfo{};
            viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            viewInfo.image = image;
            viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            viewInfo.format = format;
            viewInfo.subresourceRange.aspectMask = aspectFlags;
            viewInfo.subresourceRange.baseMipLevel = 0;
            viewInfo.subresourceRange.baseArrayLayer = 0;
            viewInfo.subresourceRange.levelCount = mipLevels;

            VkImageView imageView;
            if (vkCreateImageView(device, &viewInfo, nullptr, &imageView) != VK_SUCCESS) { throw std::runtime_error("failed to create texture image view!"); }

            return imageView;
        }


        void createTextureImageView(ModelHandler& handler) { handler.textureImageView = createImageView(handler.textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, handler.mipLevels); }


        void createTextureSampler(ModelHandler& handler) {
            VkPhysicalDeviceProperties properties{};
            vkGetPhysicalDeviceProperties(physicalDevice, &properties);

            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_TRUE;
            samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // If anisotropyEnable disabled then this to 1.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.minLod = 0.0f; // Optional
            samplerInfo.maxLod = static_cast<float>(handler.mipLevels);
            samplerInfo.mipLodBias = 0.0f; // Optional

            if (vkCreateSampler(device, &samplerInfo, nullptr, &handler.textureSampler) != VK_SUCCESS) { throw std::runtime_error("failed to create texture sampler!"); }
        }

        // END15 _______________________________________________________________________________________________________________________________________________________________________________



        // START16 - Add image depth

        void createDepthResources() {
            VkFormat depthFormat = findDepthFormat();
            createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, depthFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);

            depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

            transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
        }


        bool hasStencilComponent(VkFormat format) { return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT; }


        VkFormat findDepthFormat() {
            return findSupportedFormat(
                { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
                VK_IMAGE_TILING_OPTIMAL,
                VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
            );
        }


        VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
            for (VkFormat format : candidates) {
                VkFormatProperties props;
                vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

                if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) { return format; }
                else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) { return format; }
            }

            throw std::runtime_error("failed to find supported format!");
        }

        // END16 ______________________________________________________________________________________________________________________________________________________________________



        // START17 - Load model

        void loadModel(ModelHandler& handler) {

            if (handler.modelPath != "") {

                glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), 0.0f, glm::vec3(1.0f));


                tinyobj::attrib_t attrib;
                std::vector<tinyobj::shape_t> shapes;
                std::vector<tinyobj::material_t> materials;
                std::string warn, err;

                if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, handler.modelPath.c_str())) { throw std::runtime_error("Error: " + warn + err); }

                std::unordered_map<Vertex, uint32_t> uniqueVertices{};

                for (const auto& shape : shapes) {
                    for (const auto& index : shape.mesh.indices) {
                        Vertex vertex{};

                        vertex.pos = {
                            attrib.vertices[3 * index.vertex_index + 0],
                            attrib.vertices[3 * index.vertex_index + 1],
                            attrib.vertices[3 * index.vertex_index + 2]
                        };

                        glm::vec4 rotatedPosition = rotationMatrix * glm::vec4(vertex.pos, 1.0f);
                        vertex.pos = glm::vec3(rotatedPosition);

                        vertex.texCoord = {
                            attrib.texcoords[2 * index.texcoord_index + 0],
                            1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                        };

                        vertex.color = { 1.0f, 1.0f, 1.0f };

                        if (index.normal_index >= 0) {
                            vertex.normCoord = {
                                attrib.normals[3 * index.normal_index + 0],
                                attrib.normals[3 * index.normal_index + 1],
                                attrib.normals[3 * index.normal_index + 2]
                            };
                        }
                        else {  vertex.normCoord = glm::vec3(0.0f, 0.0f, 0.0f); }

                        vertex.materialID = FROM_FILE_MAT;

                        if (uniqueVertices.count(vertex) == 0) {
                            uniqueVertices[vertex] = static_cast<uint32_t>(uniqueVertices.size());
                            handler.vertices.push_back(vertex);
                        }

                        handler.indices.push_back(uniqueVertices[vertex]);
                    }
                }
            }

            if (handler.vertices.empty() || handler.indices.empty()) { printf("Error, it seems you have not loaded correctly the model vertices or indices;\n"); }
        }

        // END17 ______________________________________________________________________________________________________________________________________________________________________
        


        // START18 - Mipmaps

        void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
            VkFormatProperties formatProperties;
            vkGetPhysicalDeviceFormatProperties(physicalDevice, imageFormat, &formatProperties);

            if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) { throw std::runtime_error("texture image format does not support linear blitting!"); }

            VkCommandBuffer commandBuffer = beginSingleTimeCommands();

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.image = image;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;
            barrier.subresourceRange.levelCount = 1;

            int32_t mipWidth = texWidth;
            int32_t mipHeight = texHeight;

            for (uint32_t i = 1; i < mipLevels; i++) {
                barrier.subresourceRange.baseMipLevel = i - 1;
                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                VkImageBlit blit{};
                blit.srcOffsets[0] = { 0, 0, 0 };
                blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
                blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.srcSubresource.mipLevel = i - 1;
                blit.srcSubresource.baseArrayLayer = 0;
                blit.srcSubresource.layerCount = 1;
                blit.dstOffsets[0] = { 0, 0, 0 };
                blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
                blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
                blit.dstSubresource.mipLevel = i;
                blit.dstSubresource.baseArrayLayer = 0;
                blit.dstSubresource.layerCount = 1;

                vkCmdBlitImage(commandBuffer,
                    image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                    image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                    1, &blit,
                    VK_FILTER_LINEAR);

                barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
                barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

                vkCmdPipelineBarrier(commandBuffer,
                    VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                    0, nullptr,
                    0, nullptr,
                    1, &barrier);

                if (mipWidth > 1) mipWidth /= 2;
                if (mipHeight > 1) mipHeight /= 2;
            }

            barrier.subresourceRange.baseMipLevel = mipLevels - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier);

            endSingleTimeCommands(commandBuffer);
        }

        // END18 _____________________________________________________________________________________________________________________________________________________________________



        // START19 - Multisampling

        VkSampleCountFlagBits getMaxUsableSampleCount() {
            VkPhysicalDeviceProperties physicalDeviceProperties;
            vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

            VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
            if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
            if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
            if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
            if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
            if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
            if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

            return VK_SAMPLE_COUNT_1_BIT;
        }


        void createColorResources() {
            VkFormat colorFormat = swapChainImageFormat;

            createImage(swapChainExtent.width, swapChainExtent.height, 1, msaaSamples, colorFormat, VK_IMAGE_TILING_OPTIMAL,
                VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);

            colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
        }

        // END19 ___________________________________________________________________________________________________________________________________________________________________________

};

