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
    static const std::vector<VkFormat> bit8;
    static const std::vector<VkFormat> bit16;
    static const std::vector<VkFormat> bit24;
    static const std::vector<VkFormat> bit32;
    static const std::vector<VkFormat> bit48;
    static const std::vector<VkFormat> bit64;
    static const std::vector<VkFormat> bit96;
    static const std::vector<VkFormat> bit128;
    static const std::vector<VkFormat> bit192;
    static const std::vector<VkFormat> bit256;

    static std::vector<char> LoadShaderFromFile(const std::string file_name) noexcept;
    static VkShaderModule CreateShaderModule(const VkDevice dev, const std::vector<char>& code) noexcept;
  public:
    Misc() = delete;
    Misc(const Misc &obj) = delete;
    Misc(Misc &&obj) = delete;
    Misc &operator=(Misc &&obj) = delete;
    Misc &operator=(const Misc &obj) = delete;
    ~Misc() = default;

    static std::vector<const char *> RequiredLayers;
    static std::vector<const char *> RequiredGraphicDeviceExtensions;

    static VkResult CreateDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger) noexcept;
    static void DestroyDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger) noexcept;

    static SwapChainDetails GetSwapChainDetails(const VkPhysicalDevice &device, const VkSurfaceKHR &surface) noexcept;
    static size_t SizeOfFormat(const VkFormat format) noexcept;

    static VkShaderModule LoadPrecompiledShaderFromFile(const VkDevice dev, const std::string file_name) noexcept;
    static VkPipelineLayout CreatePipelineLayout(const VkDevice dev, const std::vector<VkDescriptorSetLayout> desc_layouts) noexcept;
    static std::string GetExecDirectory(const std::string argc_path) noexcept;
    static std::string GetFileExtention(const std::string file) noexcept;
    static VkDeviceSize Align(const VkDeviceSize value, const VkDeviceSize align) noexcept;
  };
}

#endif