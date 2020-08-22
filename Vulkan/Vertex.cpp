#include "Vertex.h"
#include "Supply.h"

namespace Vulkan
{
  std::pair<VkVertexInputBindingDescription, std::vector<VkVertexInputAttributeDescription>> GetVertexDescription(uint32_t binding)
  {
    std::pair<VkVertexInputBindingDescription, std::vector<VkVertexInputAttributeDescription>> result;
    std::vector<Vulkan::VertexDescription> vertex_descriptions = 
    {
      {offsetof(Vertex, pos), VK_FORMAT_R32G32B32_SFLOAT},
      {offsetof(Vertex, color), VK_FORMAT_R32G32B32_SFLOAT},
      {offsetof(Vertex, texCoord), VK_FORMAT_R32G32_SFLOAT}
    };
    Vulkan::Supply::GetVertexInputBindingDescription<Vulkan::Vertex>(binding, vertex_descriptions, result.first, result.second);

    return result;
  }
}