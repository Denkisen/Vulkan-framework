#ifndef __VULKAN_DEVICE_H
#define __VULKAN_DEVICE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <optional>
#include <vector>
#include <iostream>

#include "Surface.h"

namespace Vulkan
{ 
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

  enum class PhysicalDeviceType
  {
    Discrete = VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU,
    Integrated = VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU,
    Virtual = VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU,
    CPU = VK_PHYSICAL_DEVICE_TYPE_CPU
  };

  struct PhysicalDevice
  {
    VkPhysicalDevice device = VK_NULL_HANDLE;
    VkPhysicalDeviceProperties device_properties = {};
    VkPhysicalDeviceFeatures device_features = {};
    VkDeviceSize device_index = 0;
  };

  struct Queue
  {
    VkQueueFamilyProperties props = {};
    std::optional<VkDeviceSize> family;
    float queue_priority = 0.0f;
    Vulkan::QueuePurpose purpose = QueuePurpose::ComputePurpose;
  };

  class DeviceConfig
  {
  private:
    friend class Device_impl;
    std::optional<VkDeviceSize> device_index;
    QueueType queue_flags = QueueType::DrawingAndComputeType;
    std::shared_ptr<Surface> surface;
    PhysicalDeviceType p_device_type = PhysicalDeviceType::Discrete;
    VkPhysicalDeviceFeatures p_device_features = {};
    std::string device_name = "";
  public:
    DeviceConfig() = default;
    ~DeviceConfig() = default;
    DeviceConfig &SetDeviceIndex(const VkDeviceSize index) { device_index = index; return *this; }
    DeviceConfig &SetQueueType(const QueueType type) { queue_flags = type; return *this; }
    DeviceConfig &SetSurface(const std::shared_ptr<Surface> surf) { surface = surf; return *this; }
    DeviceConfig &SetDeviceType(const PhysicalDeviceType type) { p_device_type = type; return *this; }
    DeviceConfig &SetDeviceName(const std::string name) { device_name = name; return *this; }
    DeviceConfig &SetRequiredDeviceFeatures(const VkPhysicalDeviceFeatures features) { p_device_features = features; return *this; }
  };

  class Device_impl
  {
  public:
    ~Device_impl();
    Device_impl() = delete;
    Device_impl(const Device_impl &obj) = delete;
    Device_impl(Device_impl &&obj) = delete;
    Device_impl &operator=(const Device_impl &obj) = delete; 
    Device_impl &operator=(Device_impl &&obj) = delete; 
  private:
    std::shared_ptr<Surface> surface;
    PhysicalDevice p_device = {};
    VkPhysicalDeviceFeatures req_p_device_features = {};
    VkDevice device = VK_NULL_HANDLE;
    QueueType queue_flag_bits = QueueType::ComputeType;
    std::vector<Queue> queues;

    friend class Device;
    // public
    Device_impl(const DeviceConfig params);
    static std::vector<VkPhysicalDevice> GetAllPhysicalDevices();
    static VkDeviceSize GetPhisicalDevicesCount();
    static std::vector<std::string> GetPhysicalDeviceExtensions(VkPhysicalDevice &device);
    VkQueue GetGraphicQueue();
    VkQueue GetPresentQueue();
    VkQueue GetComputeQueue();
    std::optional<VkDeviceSize> GetGraphicFamilyQueueIndex();
    std::optional<VkDeviceSize> GetPresentFamilyQueueIndex();
    std::optional<VkDeviceSize> GetComputeFamilyQueueIndex();
    VkPhysicalDeviceProperties GetPhysicalDeviceProperties() { return p_device.device_properties; }
    VkPhysicalDevice GetPhysicalDevice() { return p_device.device; }
    VkSurfaceKHR GetSurface() { return surface->GetSurface(); }
    VkDevice GetDevice() { return device; }
    VkFormatProperties GetFormatProperties(const VkFormat format);
    bool CheckMultisampling(VkSampleCountFlagBits x);

    // private
    VkDevice Create(const VkPhysicalDeviceFeatures features);
    std::vector<Queue> FindFamilyQueues();
  };

  class Device
  {
  private:
    std::unique_ptr<Device_impl> impl;
  public:
    Device() = delete;
    Device(const Device &obj);
    Device(Device &&obj) = delete;
    Device(const DeviceConfig &params) : impl(std::unique_ptr<Device_impl>(new Device_impl(params))) {};
    Device &operator=(const Device &obj);
    Device &operator=(Device &&obj) = delete;
    static std::vector<VkPhysicalDevice> EnumPhysicalDevices() { return Device_impl::GetAllPhysicalDevices(); };
    static VkDeviceSize AvailablePhisicalDevicesCount() { return Device_impl::GetPhisicalDevicesCount(); }
    VkQueue GetGraphicQueue() { return impl->GetGraphicQueue(); }
    VkQueue GetPresentQueue() { return impl->GetPresentQueue(); }
    VkQueue GetComputeQueue() { return impl->GetComputeQueue(); }
    std::optional<VkDeviceSize> GetGraphicFamilyQueueIndex() { return impl->GetGraphicFamilyQueueIndex(); }
    std::optional<VkDeviceSize> GetPresentFamilyQueueIndex() { return impl->GetPresentFamilyQueueIndex(); }
    std::optional<VkDeviceSize> GetComputeFamilyQueueIndex() { return impl ->GetComputeFamilyQueueIndex(); }
    VkPhysicalDeviceProperties GetPhysicalDeviceProperties() { return impl->GetPhysicalDeviceProperties(); }
    VkPhysicalDevice GetPhysicalDevice() { return impl->GetPhysicalDevice(); }
    VkSurfaceKHR GetSurface() { return impl->GetSurface(); }
    VkDevice GetDevice() { return impl->GetDevice(); }
    VkFormatProperties GetFormatProperties(const VkFormat format) { return impl->GetFormatProperties(format); }
    VkBool32 CheckSampleCountSupport(VkSampleCountFlagBits x) { return impl->CheckMultisampling(x); }
    ~Device() = default;
  };
}

#endif