#include "Device.h"
 
Vulkan::Device::~Device()
{
#ifdef DEBUG
    std::cout << __func__ << std::endl;
#endif
  if (device != VK_NULL_HANDLE)
    vkDestroyDevice(device, nullptr);
  device = VK_NULL_HANDLE;
}

void Vulkan::Device::Create(Vulkan::Instance &instance, uint32_t device_index)
{
  if (instance.instance == VK_NULL_HANDLE)
    throw std::runtime_error("Instance is nullptr.");

  VkResult res = VK_SUCCESS;

  uint32_t devices_count = Vulkan::Supply::GetPhisicalDevicesCount(instance.instance);
  std::vector<VkPhysicalDevice> devices(devices_count);

  if (vkEnumeratePhysicalDevices(instance.instance, &devices_count, devices.data()) != VK_SUCCESS) 
    throw std::runtime_error(" No Physical devices found.");

  if (device_index >= devices_count)
    throw std::runtime_error(" Invalid device index.");

  p_device = devices[device_index];
  
  res = p_device == VK_NULL_HANDLE ? VK_ERROR_DEVICE_LOST : VK_SUCCESS;

  if (res == VK_SUCCESS)
  {
    if (int family_queue_tmp = Vulkan::Supply::GetFamilyQueue(p_device, queue_flag_bits); family_queue_tmp >= 0)
    {
      family_queue = (uint32_t) family_queue_tmp;
    }
    else
      res = VK_ERROR_FEATURE_NOT_PRESENT;
  }

  if (res != VK_SUCCESS)
    throw std::runtime_error(" No FamilyQueue found.");

  VkDeviceCreateInfo device_create_info = {};
  VkDeviceQueueCreateInfo queue_create_info = {};
  VkPhysicalDeviceFeatures device_features = Vulkan::Supply::GetPhysicalDeviceFeatures(p_device);

  queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
  queue_create_info.queueFamilyIndex = family_queue;
  queue_create_info.queueCount = 1;
  float queue_priority = 1.0f;
  queue_create_info.pQueuePriorities = &queue_priority;

  device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  device_create_info.pQueueCreateInfos = &queue_create_info;
  device_create_info.queueCreateInfoCount = 1;
  device_create_info.pEnabledFeatures = &device_features;
  device_create_info.enabledLayerCount = (uint32_t) Vulkan::Supply::ValidationLayers.size();
  device_create_info.ppEnabledLayerNames = Vulkan::Supply::ValidationLayers.data();

  res = vkCreateDevice(p_device, &device_create_info, nullptr, &device);
  vkGetDeviceQueue(device, family_queue, 0, &queue);

#ifdef DEBUG
    std::cout << __func__ << "() return " << res << std::endl;
#endif
  if (res != VK_SUCCESS)
    throw std::runtime_error("Can't create Device");
}

void Vulkan::Device::Create(Vulkan::Instance &instance, Vulkan::PhysicalDeviceType type)
{
  uint32_t devices_count = Vulkan::Supply::GetPhisicalDevicesCount(instance.instance);
  std::vector<VkPhysicalDevice> devices(devices_count);

  if (vkEnumeratePhysicalDevices(instance.instance, &devices_count, devices.data()) != VK_SUCCESS) 
    throw std::runtime_error(" No Physical devices found.");

  for (uint32_t i = 0; i < devices.size(); ++i)
  {
    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(devices[i], &device_properties);
    if (device_properties.deviceType == (VkPhysicalDeviceType) type)
    {
      device_limits = device_properties.limits;
      Create(instance, i);
      return;
    }
  }
  throw std::runtime_error(" No Physical devices with given type found.");
}