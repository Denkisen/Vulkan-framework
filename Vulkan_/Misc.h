#ifndef __VULKAN_MISC_H
#define __VULKAN_MISC_H

#include <vector>
#include <vulkan/vulkan.h>
#include <string>

namespace Vulkan
{
  struct SwapChainDetails
  {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
  };

  class Misc
  {
  private:
    static std::vector<char> LoadShaderFromFile(const std::string file_name);
    static VkShaderModule CreateShaderModule(const VkDevice dev, const std::vector<char>& code);
  public:
    Misc() = default;
    ~Misc() = default;

    static std::vector<const char *> RequiredLayers;
    static std::vector<const char *> RequiredGraphicDeviceExtensions;

    static VkResult CreateDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger);
    static void DestroyDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger);

    static SwapChainDetails GetSwapChainDetails(const VkPhysicalDevice &device, const VkSurfaceKHR &surface);
    static size_t SizeOfFormat(const VkFormat format);

    static VkShaderModule LoadPrecompiledShaderFromFile(const VkDevice dev, const std::string file_name);
    static VkPipelineLayout CreatePipelineLayout(const VkDevice dev, const std::vector<VkDescriptorSetLayout> desc_layouts);
    static std::string GetExecDirectory(const std::string argc_path);
    static std::string GetFileExtention(const std::string file);
  };
}

#endif