
#pragma once
#include "engine/render/Mesh.hpp"
#include "engine/resources/Texture.hpp"
#include <memory>
#include <string>
#include <vulkan/vulkan.h>
class Model {
public:
  Model(VkDevice device, VkPhysicalDevice physicalDevice,
        VkCommandPool commandPool, VkQueue graphicsQueue,
        const std::string &path,
        Texture *texture); // <-- add Texture* here
  ~Model() = default;

  std::unique_ptr<Mesh> TakeMesh() { return std::move(mesh_); }

private:
  void loadOBJ(const std::string &path);
  void loadGLTF(const std::string &path);
  Texture *texture_;
  std::unique_ptr<Mesh> mesh_;
  VkDevice device_;
  VkPhysicalDevice physicalDevice_;
  VkCommandPool commandPool_;
  VkQueue graphicsQueue_;
};
