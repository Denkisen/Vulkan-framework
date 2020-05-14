#include "Instance.h"
#include "Supply.h"

VkInstance Vulkan::Instance::instance = VK_NULL_HANDLE;

Vulkan::Instance::Instance()
{
  if (instance != VK_NULL_HANDLE) return;

  VkResult res = VK_SUCCESS;
  VkApplicationInfo app_info = {};
  VkInstanceCreateInfo create_info = {};

  app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  app_info.pApplicationName = app_name.c_str();
  app_info.applicationVersion = APP_VERSION;
  app_info.pEngineName = engine_name.c_str();
  app_info.engineVersion = ENGINE_VERSION;
  app_info.apiVersion = VK_API_VERSION_1_0;

  create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  if (Vulkan::Supply::ValidationLayers.size() > 0)
  {
    create_info.enabledLayerCount = (uint32_t) Vulkan::Supply::ValidationLayers.size();
    create_info.ppEnabledLayerNames = Vulkan::Supply::ValidationLayers.data();
  }

  create_info.pApplicationInfo = &app_info;

  auto instance_extentions = Supply::GetInstanceExtensions();
  std::vector<const char *> ins;
  for (auto &s : instance_extentions)
  {
    ins.push_back(s.c_str());
  }
  create_info.ppEnabledExtensionNames = ins.data();
  create_info.enabledExtensionCount = (uint32_t) ins.size();

  res = vkCreateInstance(&create_info, nullptr, &instance);
  
#ifdef DEBUG
    std::cout << "Using debug layer"<< std::endl;
    if (res == VK_SUCCESS)
      Supply::CreateDebugerMessenger(instance, debug_messenger);
    std::cout << __func__ << "() return " << res << std::endl;
#endif

  if (res != VK_SUCCESS)
    throw std::runtime_error("Can't create Instance");
}
  
Vulkan::Instance::~Instance()
{
#ifdef DEBUG
    std::cout << __func__ << std::endl;
#endif
  if (instance != VK_NULL_HANDLE)
  {
#ifdef DEBUG
    Supply::DestroyDebugerMessenger(instance, debug_messenger);
#endif
    vkDestroyInstance(instance, nullptr);
  }
  instance = VK_NULL_HANDLE;
}