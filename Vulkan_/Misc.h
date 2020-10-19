#ifndef __VULKAN_MISC_H
#define __VULKAN_MISC_H

#include <vector>
#include <vulkan/vulkan.h>

namespace Vulkan
{
  class Misc
  {
  public:
    Misc() = default;
    ~Misc() = default;

    static std::vector<const char *> RequiredLayers;
    static std::vector<const char *> RequiredGraphicDeviceExtensions;

    static VkResult CreateDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger);
    static void DestroyDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger);
  };
}

#endif