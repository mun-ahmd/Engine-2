#pragma once
#include "glm/fwd.hpp"
#include "glm/glm.hpp"

class PointLight {
private:
  glm::vec3 position;
  float radius;
  glm::vec3 color;
  float intensity;
  // bool hasShadows;

public:
  PointLight() = default;
  PointLight(glm::vec3 position, float radius, glm::vec3 color, float intensity)
      : position(position), radius(radius), color(color), intensity(intensity) {
  }
};

class SpotLight {
private:
  glm::vec3 position;
  glm::vec3 color;
  float intensity;
  float radius;

public:
  SpotLight() = default;
  SpotLight(glm::vec3 position, glm::vec3 color, float intensity, float radius)
      : position(position), color(color), intensity(intensity), radius(radius) {
  }
};

class DirectionalLight {
private:
  glm::vec3 direction;
  glm::vec3 color;
  float intensity;

public:
  DirectionalLight() = default;
  DirectionalLight(glm::vec3 direction, glm::vec3 color, float intensity)
      : direction(direction), color(color), intensity(intensity) {}
};