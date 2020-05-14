#ifndef __CPU_NW_LIBS_VULKAN_DEVICES_H
#define __CPU_NW_LIBS_VULKAN_DEVICES_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

#include "Supply.h"
#include "Instance.h"

namespace Vulkan
{
  enum PhysicalDeviceType
  {
    Discrete = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    Integrated = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
    Virtual = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
    CPU = VK_PHYSICAL_DEVICE_TYPE_CPU
  };

  class Device
  {
  private:
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice p_device = VK_NULL_HANDLE;
    VkQueueFlagBits queue_flag_bits = VK_QUEUE_COMPUTE_BIT;
    VkQueue queue = VK_NULL_HANDLE;
    VkPhysicalDeviceLimits device_limits = {};
    uint32_t family_queue = 0;
    void Create(Vulkan::Instance &instance, uint32_t device_index);
    void Create(Vulkan::Instance &instance, Vulkan::PhysicalDeviceType type);
    template <typename T> friend class Array;
    template <typename T> friend class Offload;
    friend class UniformBuffer;
  public:
    Device() = default;
    Device(const Device &obj) = delete;
    Device& operator= (const Device &obj) = delete;
    Device(Vulkan::Instance &instance, uint32_t device_index) { Create(instance, device_index); }
    Device(Vulkan::Instance &instance, Vulkan::PhysicalDeviceType type) { Create(instance, type); }
    static uint32_t AvaliableDevices(Vulkan::Instance &instance) { return Vulkan::Supply::GetPhisicalDevicesCount(instance.instance); }
    VkPhysicalDeviceLimits GetLimits() { return device_limits; }
    ~Device();
  };
}

#endif