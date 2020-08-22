#ifndef __CPU_NW_VULKAN_VERTEX_H
#define __CPU_NW_VULKAN_VERTEX_H

#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vector>

namespace Vulkan
{
  struct Vertex
  {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    bool operator==(const Vertex& other) const 
    {
      return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
  };

  std::pair<VkVertexInputBindingDescription, std::vector<VkVertexInputAttributeDescription>> GetVertexDescription(uint32_t binding);
}

namespace std 
{
  template<> struct hash<Vulkan::Vertex> 
  {
    size_t operator()(Vulkan::Vertex const& vertex) const 
    {
      auto p = hash<glm::vec3>()(vertex.pos);
      auto c = hash<glm::vec3>()(vertex.color);
      auto t = hash<glm::vec2>()(vertex.texCoord);
      return ((p ^ (c << 1)) >> 1) ^ (t << 1);
    }
  };
}

#endif