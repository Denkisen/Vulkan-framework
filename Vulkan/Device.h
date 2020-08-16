#ifndef __CPU_NW_VULKAN_DEVICES_H
#define __CPU_NW_VULKAN_DEVICES_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <optional>

#include "Supply.h"
#include "Instance.h"

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Vulkan
{
  enum class PhysicalDeviceType
  {
    Discrete = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    Integrated = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
    Virtual = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
    CPU = VK_PHYSICAL_DEVICE_TYPE_CPU
  };

  enum class QueueType
  {
    ComputeType = VK_QUEUE_COMPUTE_BIT,
    DrawingType = VK_QUEUE_GRAPHICS_BIT,
    DrawingAndComputeType = (VK_QUEUE_COMPUTE_BIT | VK_QUEUE_GRAPHICS_BIT)
  };

  enum class QueuePurpose
  {
    ComputePurpose,
    PresentationPurpose,
    GraphicPurpose,
    PresentationAndGraphicPurpose
  };

  struct PhysicalDevice
  {
    VkPhysicalDevice device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties device_properties = {};
    VkPhysicalDeviceFeatures device_features = {};
    uint32_t device_index = 0;
  };

  struct Surface
  {
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    GLFWwindow *window = nullptr;
  };

  struct Queue
  {
    VkQueueFamilyProperties props = {};
    std::optional<uint32_t> family;
    float queue_priority = 0.0f;
    Vulkan::QueuePurpose purpose = QueuePurpose::ComputePurpose;
  };

  class Device
  {
    private:
      Vulkan::Surface surface = {};
      Vulkan::PhysicalDevice p_device = {};
      VkDevice device = VK_NULL_HANDLE;
      Vulkan::QueueType queue_flag_bits = QueueType::ComputeType;
      std::vector<Queue> queues;
      void Create();
      void CreateSurface();
      std::vector<Queue> FindFamilyQueues();
    public:
      Device() = delete;
      Device(const Device &obj) = delete;
      Device& operator= (const Device &obj) = delete;
      explicit Device(uint32_t device_index, Vulkan::QueueType queue_flags = QueueType::ComputeType, GLFWwindow *window = nullptr);
      explicit Device(Vulkan::PhysicalDeviceType type, Vulkan::QueueType queue_flags = QueueType::ComputeType, GLFWwindow *window = nullptr);
      explicit Device(std::string device_name, Vulkan::QueueType queue_flags = QueueType::ComputeType, GLFWwindow *window = nullptr);
      ~Device();
      VkQueue GetGraphicQueue();
      VkQueue GetPresentQueue();
      VkQueue GetComputeQueue();
      std::optional<uint32_t> GetGraphicFamilyQueueIndex();
      std::optional<uint32_t> GetPresentFamilyQueueIndex();
      std::optional<uint32_t> GetComputeFamilyQueueIndex();
      std::pair<uint32_t, uint32_t> GetWindowSize();
      SwapChainDetails GetSwapChainDetails() { return Vulkan::Supply::GetSwapChainDetails(p_device.device, surface.surface); }
      static uint32_t AvaliableDevices() 
      { 
        Vulkan::Instance instance; 
        return Vulkan::Supply::GetPhisicalDevicesCount(instance.GetInstance()); 
      }
      VkPhysicalDeviceLimits GetLimits() { return p_device.device_properties.limits; }
      VkPhysicalDevice GetPhysicalDevice() { return p_device.device; }
      VkSurfaceKHR GetSurface() { return surface.surface; }
      VkDevice GetDevice() { return device; }
  };
}


#endif