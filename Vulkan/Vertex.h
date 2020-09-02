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
  struct alignas(16) Vertex 
  {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;
    glm::vec3 normal;

    bool operator==(const Vertex& other) const 
    {
      return pos == other.pos && color == other.color && texCoord == other.texCoord && normal == other.normal;
    }
  };

  std::pair<VkVertexInputBindingDescription, std::vector<VkVertexInputAttributeDescription>> GetVertexDescription(uint32_t binding);
}

template <class T>
inline void hash_combine(std::size_t & s, const T & v)
{
  std::hash<T> h;
  s^= h(v) + 0x9e3779b9 + (s<< 6) + (s>> 2);
}

namespace std 
{
  template<> struct hash<Vulkan::Vertex> 
  {
    size_t operator()(Vulkan::Vertex const& vertex) const 
    {
      size_t res = 0;
      hash_combine(res, hash<glm::vec3>()(vertex.pos));
      hash_combine(res, hash<glm::vec3>()(vertex.color));
      hash_combine(res, hash<glm::vec2>()(vertex.texCoord));
      hash_combine(res, hash<glm::vec3>()(vertex.normal));

      return res;
    }
  };
}

#endif