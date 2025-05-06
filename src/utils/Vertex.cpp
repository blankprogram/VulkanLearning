#include "engine/utils/Vertex.hpp"
#include <cstddef>
namespace engine {

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
  return vk::VertexInputBindingDescription{}
      .setBinding(0)
      .setStride(sizeof(Vertex))
      .setInputRate(vk::VertexInputRate::eVertex);
}

std::array<vk::VertexInputAttributeDescription, 2>
Vertex::getAttributeDescriptions() {
  std::array<vk::VertexInputAttributeDescription, 2> desc{};
  desc[0] = vk::VertexInputAttributeDescription{}
                .setBinding(0)
                .setLocation(0)
                .setFormat(vk::Format::eR32G32B32Sfloat)
                .setOffset(offsetof(Vertex, pos));
  desc[1] = vk::VertexInputAttributeDescription{}
                .setBinding(0)
                .setLocation(1)
                .setFormat(vk::Format::eR32G32B32Sfloat)
                .setOffset(offsetof(Vertex, color));
  return desc;
}

} // namespace engine
