
#include "cameraObj.h"

// Default camera values

//All angle values in terms of float are converted into radians in the constructor funcs
const float YAW = -90.0f;
const float PITCH = 0.0f;
const float ROLL = 0.0f;
const float SPEED = 12.5f;
const float SENSITIVITY = 0.1f;
const float ZOOM = 45.0f;
const float PI = 3.1416;


//Camera constructor that takes front directly instead of euler angles
Camera::Camera(glm::vec3 position, glm::vec3 up, glm::vec3 front)
{
    movementSpeed = SPEED;
    mouseSensitivity = SENSITIVITY;
    zoom = ZOOM;

    Camera::position = position;
    worldUp = up;
    Camera::front = front;
    calcPitchYawRoll();
}

// returns the view matrix calculated using Euler Angles and the LookAt Matrix
glm::mat4 Camera::getViewMatrix()
{
    updateCameraVectors();
    return glm::lookAt(position, position + front, up);
}

void Camera::debugCamera() {
    front;right;up;worldUp;position;pitch;yaw;

}

// processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
void Camera::keyboardMovementProcess(Camera_Movement direction, float deltaTime)
{


    float velocity = movementSpeed * deltaTime;
    if (direction == FORWARD)
        position += front * velocity;
    if (direction == BACKWARD)
        position -= front * velocity;
    if (direction == LEFT)
        position -= right * velocity;
    if (direction == RIGHT)
        position += right * velocity;

    updateCameraVectors();
}

// processes input received from a mouse input system. Expects the offset value in both the x and y direction.
void Camera::mouseLookProcess(float xoffset, float yoffset, bool constrainpitch = true)
{
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // make sure that when pitch is out of bounds, screen doesn't get flipped
    if (constrainpitch)
    {
        if (pitch > 89.0f)
            pitch = 89.0f;
        if (pitch < -89.0f)
            pitch = -89.0f;
    }

    // update front, right and up Vectors using the updated Euler angles
    updateCameraVectors();
}

// processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
void Camera::processMouseScroll(float yoffset)
{
    zoom -= (float)yoffset;
    if (zoom < 1.0f)
        zoom = 1.0f;
    if (zoom > 45.0f)
        zoom = 45.0f;
}


void Camera::changeCameraFront(glm::vec3 newFront) {
    Camera::front = newFront;
    calcPitchYawRoll();
    updateCameraVectors();
}


// calculates the front vector from the Camera's (updated) Euler Angles
void Camera::updateCameraVectors()
{
    // calculate the new front vector
    float ryaw = glm::radians(yaw); float rpitch = glm::radians(pitch);
    Camera::front.x = cos(ryaw) * cos(rpitch);
    Camera::front.y = -sin(rpitch);
    Camera::front.z = sin(ryaw) * cos(rpitch);

    glm::vec3 frontnorm = glm::normalize(front);
    // also re-calculate the right and up vector
    right = glm::normalize(glm::cross(frontnorm, worldUp));  // normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    up = glm::normalize(glm::cross(right, frontnorm));
}

void Camera::calcPitchYawRoll() {
    yaw = acos((glm::dot(front, glm::vec3(front.x, front.y, 0.0f))));
    pitch = acos((glm::dot(front, glm::vec3(front.x, 0.0f, front.z))));
}




//Cubemap making Camera
CameraCubemap::CameraCubemap() {
    CameraCubemap::position = glm::vec3(0.0, 0.0, 3.0);
    CameraCubemap::front = glm::vec3(0.0f, 0.0f, 1.0f); //ogFront;
    CameraCubemap::right = glm::vec3(1.0f, 0.0f, 0.0f);
    CameraCubemap::worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    CameraCubemap::up = worldUp;
}

void CameraCubemap::changeCamDir(Camera_Movement direction) {
    if (direction == FORWARD) {
        front = forwardFront;
        right = glm::cross(front, worldUp);
    }
    if (direction == BACKWARD) {
        front = backwardFront;
        right = glm::cross(front, worldUp);
    }
    if (direction == LEFT) {
        front = leftFront;
        right = glm::cross(front, worldUp);
    }
    if (direction == RIGHT) {
        front = rightFront;
        right = glm::cross(front, worldUp);
    }
    if (direction == UP) {
        front = upFront;
        right = upRight;
    }
    if (direction == DOWN) {
        front = upFront;
        right = upRight;
    }

    up = glm::cross(right, front);
}

glm::mat4 CameraCubemap::getViewMatrix() {
    return glm::lookAt(position, position + front, up);
}

