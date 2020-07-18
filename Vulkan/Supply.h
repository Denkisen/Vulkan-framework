
#ifndef __CPU_NW_LIBS_VULKAN_SUPPLY_H
#define __CPU_NW_LIBS_VULKAN_SUPPLY_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

namespace Vulkan
{
  struct SwapChainDetails
  {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
  };

  enum class ShaderType
  {
    Vertex,
    Fragment,
    Compute,
  };

  struct ShaderInfo
  {
    std::string entry = "main";
    std::string file_path;
    Vulkan::ShaderType type;
  };

  class Supply
  {
  private:
    Supply() {}
    ~Supply() {}
    static std::vector<char> LoadShaderFromFile(std::string file_name);
    static VkShaderModule CreateShaderModule(VkDevice &dev, std::vector<char>& code);
  public:
    static std::vector<const char *> ValidationLayers;
    static std::vector<const char *> RequiredGraphicDeviceExtensions;
    static std::vector<std::string> GetInstanceExtensions();
    static std::vector<std::string> GetPhysicalDeviceExtensions(VkPhysicalDevice &device);
    
    static std::vector<VkPhysicalDevice> GetPhysicalDevicesByType(VkInstance &instance, VkPhysicalDeviceType type);
    static std::vector<VkPhysicalDevice> GetAllPhysicalDevices(VkInstance &instance);
    static VkPhysicalDeviceFeatures GetPhysicalDeviceFeatures(VkPhysicalDevice &dev);
    static uint32_t GetPhisicalDevicesCount(VkInstance &instance);
    
    static VkResult LoadPrecompiledShaderFromFile(VkDevice dev, std::string file_name, VkShaderModule &shader);
    static VkResult CreateShaderStageInfo(VkDevice device, Vulkan::ShaderInfo &info, VkShaderModule &module, VkShaderStageFlagBits stage, VkPipelineShaderStageCreateInfo &out);

    static VkResult CreateDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger);
    static void DestroyDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger);

    static VkQueue GetQueueFormFamilyIndex(const VkDevice &device, const uint32_t index);
    static uint32_t GetFamilyQueuesCount(VkPhysicalDevice &dev);
    static bool CheckQueueBit(VkPhysicalDevice &dev, uint32_t index, VkQueueFlagBits bit); 

    static SwapChainDetails GetSwapChainDetails(VkPhysicalDevice &device, VkSurfaceKHR &surface);

    static VkCommandBuffer CreateCommandBuffer(VkDevice device, const VkCommandPool pool, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    static std::vector<VkCommandBuffer> CreateCommandBuffers(VkDevice device, const VkCommandPool pool, const uint32_t count, VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    static VkCommandPool CreateCommandPool(VkDevice device, const uint32_t family_queue);

    static VkPipelineLayout CreatePipelineLayout(VkDevice device, std::vector<VkDescriptorSetLayout> layouts);
  };
}


#endif