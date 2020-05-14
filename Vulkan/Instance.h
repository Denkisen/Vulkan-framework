#ifndef __CPU_NW_LIBS_VULKAN_INSTANCE_H
#define __CPU_NW_LIBS_VULKAN_INSTANCE_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

#define APP_VERSION VK_MAKE_VERSION(1, 0, 0)
#define ENGINE_VERSION VK_MAKE_VERSION(1, 0, 0)

namespace Vulkan 
{
  class Instance
  {
  private:
    static VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger = VK_NULL_HANDLE;
    std::string app_name = "Application";
    std::string engine_name = "MarisaCompute";
    friend class Device;
    template <typename T> friend class Offload;
  public:
    Instance();
    Instance(const Instance &obj) = delete;
    Instance& operator= (const Instance &obj) = delete;
    ~Instance();
  };
}

#endif