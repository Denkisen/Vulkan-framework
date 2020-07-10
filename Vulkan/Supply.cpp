#include "Supply.h"
#include <fstream>
#include <string.h>

std::vector<const char *> Vulkan::Supply::ValidationLayers = 
{
  "VK_LAYER_KHRONOS_validation"
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
                                                    VkDebugUtilsMessageTypeFlagsEXT messageType,
                                                    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
                                                    void* pUserData) 
{
  std::ofstream f;
  f.open("log.txt", std::ios::app);
  std::cout << "validation layer: " << pCallbackData->pMessage << std::endl;
  f << pCallbackData->pMessage << std::endl;
  f.close();

  return VK_FALSE;
}

VkResult Vulkan::Supply::CreateDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger)
{
  VkDebugUtilsMessengerCreateInfoEXT create_d_info = {};
  create_d_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  create_d_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  create_d_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  create_d_info.pfnUserCallback = DebugCallback;
  create_d_info.pUserData = nullptr;
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  return func(instance, &create_d_info, nullptr, &debug_messenger);
}

void Vulkan::Supply::DestroyDebugerMessenger(VkInstance &instance, VkDebugUtilsMessengerEXT &debug_messenger)
{
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
  func(instance, debug_messenger, nullptr);
}

VkPhysicalDeviceFeatures Vulkan::Supply::GetPhysicalDeviceFeatures(VkPhysicalDevice &dev)
{
  VkPhysicalDeviceFeatures result = {};
  if (dev != VK_NULL_HANDLE)
    vkGetPhysicalDeviceFeatures(dev, &result);
  return result;
}

std::vector<VkPhysicalDevice> Vulkan::Supply::GetAllPhysicalDevices(VkInstance &instance)
{
  std::vector<VkPhysicalDevice> ret(GetPhisicalDevicesCount(instance), VK_NULL_HANDLE);
  uint32_t count = (uint32_t) ret.size();
  if (ret.empty() || vkEnumeratePhysicalDevices(instance, &count, ret.data()) != VK_SUCCESS) 
    ret.clear();

  return ret;
}

std::vector<VkPhysicalDevice> Vulkan::Supply::GetPhysicalDevicesByType(VkInstance &instance, VkPhysicalDeviceType type)
{
  std::vector<VkPhysicalDevice> ret;
  std::vector<VkPhysicalDevice> devices = GetAllPhysicalDevices(instance);

  for (auto &dev : devices)
  {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(dev, &device_properties);
    if (device_properties.deviceType == type)
    {
      ret.push_back(dev);
    }
  }

  return ret;
}

uint32_t Vulkan::Supply::GetPhisicalDevicesCount(VkInstance &instance)
{
  if (instance == VK_NULL_HANDLE) return 0;

  uint32_t device_count = 0;
  vkEnumeratePhysicalDevices(instance, &device_count, nullptr);
  return device_count;
}

uint32_t Vulkan::Supply::GetFamilyQueuesCount(VkPhysicalDevice &dev)
{
  uint32_t queue_family_count = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, nullptr);
  return queue_family_count;
}

bool Vulkan::Supply::CheckQueueBit(VkPhysicalDevice &dev, uint32_t index, VkQueueFlagBits bit)
{
  if (dev == VK_NULL_HANDLE) return false;

  uint32_t queue_family_count = GetFamilyQueuesCount(dev);
  if (index >= queue_family_count) return false;

  std::vector<VkQueueFamilyProperties> queue_families(queue_family_count);
  vkGetPhysicalDeviceQueueFamilyProperties(dev, &queue_family_count, queue_families.data());

  return queue_families[index].queueFlags & bit;
}

std::vector<char> Vulkan::Supply::LoadShaderFromFile(std::string file_name)
{
  std::ifstream f(file_name, std::ios::ate | std::ios::binary);
  std::vector<char> res;
  if (!f.is_open()) return res;

  res.resize((uint32_t) f.tellg());
  f.seekg(0);
  f.read(res.data(), res.size());
  f.close();
  return res;
}

VkShaderModule Vulkan::Supply::CreateShaderModule(VkDevice &dev, std::vector<char> &code)
{
  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.codeSize = code.size();
  create_info.pCode = reinterpret_cast<const uint32_t*>(code.data());
  VkShaderModule shader_module = VK_NULL_HANDLE;
  VkResult res = vkCreateShaderModule(dev, &create_info, nullptr, &shader_module);
  if (res != VK_SUCCESS) return VK_NULL_HANDLE;
  
  return shader_module;
}

VkResult Vulkan::Supply::LoadPrecompiledShaderFromFile(VkDevice &dev, std::string file_name, VkShaderModule &shader)
{
  if (shader != VK_NULL_HANDLE)
    vkDestroyShaderModule(dev, shader, nullptr);
  shader = VK_NULL_HANDLE;
  
  auto shader_c = LoadShaderFromFile(file_name);
  shader = CreateShaderModule(dev, shader_c);
  if (shader != VK_NULL_HANDLE) return VK_SUCCESS;

  return VK_ERROR_INVALID_SHADER_NV;
}

std::vector<std::string> Vulkan::Supply::GetInstanceExtensions()
{
  std::vector<std::string> res;

  uint32_t ext_count = 0;
  if (vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, nullptr) == VK_SUCCESS)
  {
    std::vector<VkExtensionProperties> props(ext_count);
    if (vkEnumerateInstanceExtensionProperties(nullptr, &ext_count, props.data()) == VK_SUCCESS)
    {
      for (auto &ext : props) 
      {
        res.push_back(ext.extensionName);
      }
    }
  }

  return res;
}

bool Vulkan::Supply::IsDataVectorValid(const std::vector<IStorage*> &data)
{
  bool result = true;
  if (data.size() == 0) return false;
  VkDevice dev = data[0]->device;

  for (std::size_t i = 0; i < data.size(); ++i)
  {
    if (data[i]->device != dev)
      return false;
  }

  return result;
}

VkQueue Vulkan::Supply::GetQueueFormFamilyIndex(const VkDevice &device, const uint32_t index)
{
  VkQueue q;
  vkGetDeviceQueue(device, index, 0, &q);
  return q;
}