#include "Instance.h"
#include "Misc.h"
#include "Logger.h"

namespace Vulkan
{
  VkInstance Instance::instance = VK_NULL_HANDLE;
  VkDebugUtilsMessengerEXT Instance::debug_messenger = VK_NULL_HANDLE;
  std::string Instance::app_name = "Application";
  std::string Instance::engine_name = "Marisa";
  std::mutex Instance::instance_lock;

  std::vector<std::string> Instance::GetInstanceExtensions()
  {
    uint32_t ext_count = 0;
    std::vector<std::string> ret;
    if (vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr) == VK_SUCCESS)
    {
      std::vector<VkExtensionProperties> props(ext_count);
      if (vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, props.data()) == VK_SUCCESS)
      {
        for (auto &ext : props) 
        {
          ret.push_back(ext.extensionName);
        }
      }
    }

    return ret;
  }

  VkInstance& Instance::GetInstance() noexcept
  { 
    std::lock_guard<std::mutex> lock(instance_lock);

    if (instance == VK_NULL_HANDLE)
    {
      VkResult res = VK_SUCCESS;

      std::vector<const char *> ins;
      try
      {
        auto extensions = GetInstanceExtensions();
        ins.reserve(extensions.size());
        for (auto &s : extensions)
          ins.push_back(s.c_str());
      }
      catch (...)
      {

      }

      VkApplicationInfo app_info = {};
      app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
      app_info.pApplicationName = app_name.c_str();
      app_info.applicationVersion = APP_VERSION;
      app_info.pEngineName = engine_name.c_str();
      app_info.engineVersion = ENGINE_VERSION;
      app_info.apiVersion = VK_API_VERSION_1_1;

      VkInstanceCreateInfo create_info = {};
      create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

      if (Misc::RequiredLayers.size() > 0)
      {
        create_info.enabledLayerCount = (uint32_t) Misc::RequiredLayers.size();
        create_info.ppEnabledLayerNames = Misc::RequiredLayers.data();
      }

      create_info.pApplicationInfo = &app_info;
      create_info.ppEnabledExtensionNames = ins.size() > 0 ? ins.data() : nullptr;
      create_info.enabledExtensionCount = (uint32_t) ins.size();

      res = vkCreateInstance(&create_info, nullptr, &instance);

      if (res == VK_SUCCESS)
      {
#ifdef DEBUG
        Misc::CreateDebugerMessenger(instance, debug_messenger);
#endif
      }
      else
      {
        instance = VK_NULL_HANDLE;
      }
    }

    Logger::EchoDebug("instance handle: " + std::to_string((uint64_t) instance), __func__);

    return instance; 
  }

  Instance::~Instance() noexcept
  {
    std::lock_guard<std::mutex> lock(instance_lock);

    Logger::EchoDebug("", __func__);

    if (instance != VK_NULL_HANDLE)
    {
#ifdef DEBUG
      Misc::DestroyDebugerMessenger(instance, debug_messenger);
#endif
      vkDestroyInstance(instance, nullptr);
      instance = VK_NULL_HANDLE;
    }
  }
}