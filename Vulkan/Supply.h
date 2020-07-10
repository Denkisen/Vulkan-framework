
#ifndef __CPU_NW_LIBS_VULKAN_SUPPLY_H
#define __CPU_NW_LIBS_VULKAN_SUPPLY_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

#include "IStorage.h"

namespace Vulkan
{
  class Supply
  {
  private:
    Supply() {}
    ~Supply() {}
    static std::vector<char> LoadShaderFromFile(std::string file_name);
    static VkShaderModule CreateShaderModule(VkDevice &dev, std::vector<char>& code);
  public:
    static std::vector<VkPhysicalDevice> GetPhysicalDevicesByType(VkInstance &instance, VkPhysicalDeviceType type);
    static std::vector<VkPhysicalDevice> GetAllPhysicalDevices(VkInstance &instance);
    static VkPhysicalDeviceFeatures GetPhysicalDeviceFeatures(VkPhysicalDevice &dev);
    static uint32_t GetPhisicalDevicesCount(VkInstance &instance);
    static uint32_t GetFamilyQueuesCount(VkPhysicalDevice &dev);
    static bool CheckQueueBit(VkPhysicalDevice &dev, uint32_t index, VkQueueFlagBits bit);  
    static VkResult LoadPrecompiledShaderFromFile(VkDevice &dev, std::string file_name, VkShaderModule &shader);
    static VkResult CreateDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger);
    static void DestroyDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger);
    static std::vector<std::string> GetInstanceExtensions();
    static std::vector<const char *> ValidationLayers;
    static bool IsDataVectorValid(const std::vector<IStorage*> &data);
    static VkQueue GetQueueFormFamilyIndex(const VkDevice &device, const uint32_t index);
  };
}


#endif