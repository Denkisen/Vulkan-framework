#ifndef __VULKAN_INSTANCE_H
#define __VULKAN_INSTANCE_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <mutex>

#define APP_VERSION VK_MAKE_VERSION(1, 0, 0)
#define ENGINE_VERSION VK_MAKE_VERSION(1, 0, 0)

namespace Vulkan 
{
  class Instance
  {
  private:
    static VkInstance instance;
    static VkDebugUtilsMessengerEXT debug_messenger;
    static std::string app_name;
    static std::string engine_name;
    static std::mutex instance_lock;
    static std::vector<std::string> GetInstanceExtensions();
  public:
    Instance() = delete;
    Instance(const Instance &obj) = delete;
    Instance& operator= (const Instance &obj) = delete;
    static std::string AppName() { return app_name; }
    static VkInstance& GetInstance();
    ~Instance();
  };
}

#endif