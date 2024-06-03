
#ifndef VIEW_CAMERA_CONTROL
#define VIEW_CAMERA_CONTROL


#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <iostream>


// Enum to make direction more understandable in code instead of using numbers;
enum Direction {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Class to handle both mouse and keyboard inputs for movement;
class ViewCameraControl {

    public:
        glm::vec3 position; // Current player position;
        glm::vec3 front; // Vector that holde the current front direction;
        glm::vec3 up; // Vector that holde the up direction. This won't ever change in the current configuration so it is just as worldUp;
        glm::vec3 right; // Vector that holde the current right direction;
        glm::vec3 worldUp; // Vector that hold the fixed up direction in the world (not relative to the player);

        float yaw; // Current yaw angle;
        float pitch; // Current pitch angle;

        float movementSpeed; // Speed of the player;
        float mouseSensitivity; // Mouse sensitivity, could be make changeable in settings if they will ever be created;
        float fixedHeight; // Height at which the player camera is. It is fixed so to have a "walking" movement;

        // Constructor;
        ViewCameraControl(glm::vec3 startPosition, glm::vec3 startUp, float startYaw, float startPitch, float movementSpeed = 2.5f)
            : front(glm::vec3(0.0f, 0.0f, 1.0f)), movementSpeed(movementSpeed), mouseSensitivity(0.08f), fixedHeight(startPosition.y) {

            position = startPosition;
            worldUp = startUp;
            yaw = startYaw;
            pitch = startPitch;

            updateCameraVectors();
        }

        // Used in the Starter.hpp to update the view matrix in the UBO;
        glm::mat4 getViewMatrix() { return glm::lookAt(position, position + front, up); }

        // Process keyboard inputs;
        void processKeyboardInput(Direction direction, float deltaTime) {
            float velocity = movementSpeed * deltaTime;

            // Use this new movement so speed is not affected by watching direction;
            glm::vec3 movement = glm::normalize(glm::vec3(front.x, 0.0f, front.z));

            if (direction == FORWARD) { position += movement * velocity; }
            if (direction == BACKWARD) { position -= movement * velocity; }
            if (direction == LEFT) { position -= right * velocity; }
            if (direction == RIGHT) { position += right * velocity; }

            position.y = fixedHeight;
        }

        // Process mouse inputs;
        void processMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
            xoffset *= mouseSensitivity;
            yoffset *= mouseSensitivity;

            yaw += xoffset;
            pitch += yoffset;

            if (constrainPitch) {
                if (pitch > 89.0f) { pitch = 89.0f; }
                if (pitch < -89.0f) { pitch = -89.0f; }
            }

            updateCameraVectors();
        }

    private:

        // Used to update the variables vectors to the inputs received;
        void updateCameraVectors() {
            // Calculate the new front vector
            glm::vec3 newFront;
            newFront.x = cos(glm::radians(pitch)) * cos(glm::radians(yaw));
            newFront.y = sin(glm::radians(pitch));
            newFront.z = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
            front = glm::normalize(newFront);

            // Recalculate right and up vectors
            right = glm::normalize(glm::cross(front, worldUp)); // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
            up = glm::normalize(glm::cross(right, front));
        }
};

#endif