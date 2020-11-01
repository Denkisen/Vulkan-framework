#ifndef __VULKAN_PIPELINES_TYPES_H
#define __VULKAN_PIPELINES_TYPES_H

#include <vulkan/vulkan.h>
#include <string>
#include <filesystem>

namespace Vulkan
{
  enum class ShaderType
  {
    Vertex = VK_SHADER_STAGE_VERTEX_BIT,
    Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
    Compute = VK_SHADER_STAGE_COMPUTE_BIT,
  };

  struct ShaderInfo
  {
    std::string entry = "main";
    std::filesystem::path file_path;
    ShaderType type;
  };

  struct Shader
  {
    VkShaderModule shader = VK_NULL_HANDLE;
    std::string entry = "main";
  };
}
#endif