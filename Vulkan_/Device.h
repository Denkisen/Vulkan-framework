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
    std::optional<uint32_t> family;
    float queue_priority = 0.0f;
    QueuePurpose purpose = QueuePurpose::ComputePurpose;
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
    DeviceConfig() noexcept = default;
    ~DeviceConfig() noexcept = default;
    auto &SetDeviceIndex(const VkDeviceSize index) noexcept { device_index = index; return *this; }
    auto &SetQueueType(const QueueType type) noexcept { queue_flags = type; return *this; }
    auto &SetSurface(const std::shared_ptr<Surface> surf) noexcept { surface = surf; return *this; }
    auto &SetDeviceType(const PhysicalDeviceType type) noexcept { p_device_type = type; return *this; }
    auto &SetDeviceName(const std::string name) { device_name = name; return *this; }
    auto &SetRequiredDeviceFeatures(const VkPhysicalDeviceFeatures features) noexcept { p_device_features = features; return *this; }
  };

  class Device_impl
  {
  public:
    ~Device_impl() noexcept;
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
    Device_impl(const DeviceConfig params) noexcept;
    static std::vector<VkPhysicalDevice> GetAllPhysicalDevices() noexcept;
    static VkDeviceSize GetPhisicalDevicesCount() noexcept;
    static std::vector<std::string> GetPhysicalDeviceExtensions(const VkPhysicalDevice &device) noexcept;
    VkQueue GetGraphicQueue() const noexcept;
    VkQueue GetPresentQueue() const noexcept;
    VkQueue GetComputeQueue() const noexcept;
    std::optional<uint32_t> GetGraphicFamilyQueueIndex() const noexcept;
    std::optional<uint32_t> GetPresentFamilyQueueIndex() const noexcept;
    std::optional<uint32_t> GetComputeFamilyQueueIndex() const noexcept;
    VkQueue GetQueueFormFamilyIndex(const uint32_t index) const noexcept;
    VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const noexcept { return p_device.device_properties; }
    VkPhysicalDevice GetPhysicalDevice() const noexcept { return p_device.device; }
    std::shared_ptr<Surface> GetSurface() const noexcept { return surface; }
    VkDevice GetDevice() const noexcept { return device; }
    VkFormatProperties GetFormatProperties(const VkFormat format) const noexcept;
    bool CheckMultisampling(VkSampleCountFlagBits x) const noexcept;

    // private
    VkDevice Create(const VkPhysicalDeviceFeatures features) noexcept;
    std::vector<Queue> FindFamilyQueues() const noexcept;
  };

  class Device
  {
  private:
    std::unique_ptr<Device_impl> impl;
  public:
    Device() = delete;
    Device(const Device &obj) noexcept;
    Device(Device &&obj) noexcept : impl(std::move(obj.impl)) {};;
    Device(const DeviceConfig &params) noexcept : impl(std::unique_ptr<Device_impl>(new Device_impl(params))) {};
    Device &operator=(const Device &obj) noexcept;
    Device &operator=(Device &&obj) noexcept;
    void swap(Device &obj) noexcept;
    static std::vector<VkPhysicalDevice> EnumPhysicalDevices() noexcept { return Device_impl::GetAllPhysicalDevices(); };
    static VkDeviceSize AvailablePhisicalDevicesCount() noexcept { return Device_impl::GetPhisicalDevicesCount(); }
    VkQueue GetGraphicQueue() const noexcept { if (impl.get()) return impl->GetGraphicQueue(); return VK_NULL_HANDLE; }
    VkQueue GetPresentQueue() const noexcept { if (impl.get()) return impl->GetPresentQueue(); return VK_NULL_HANDLE; }
    VkQueue GetComputeQueue() const noexcept { if (impl.get()) return impl->GetComputeQueue(); return VK_NULL_HANDLE; }
    std::optional<uint32_t> GetGraphicFamilyQueueIndex() const noexcept { if (impl.get()) return impl->GetGraphicFamilyQueueIndex(); return {}; }
    std::optional<uint32_t> GetPresentFamilyQueueIndex() const noexcept { if (impl.get()) return impl->GetPresentFamilyQueueIndex(); return {}; }
    std::optional<uint32_t> GetComputeFamilyQueueIndex() const noexcept { if (impl.get()) return impl->GetComputeFamilyQueueIndex(); return {}; }
    VkQueue GetQueueFormFamilyIndex(const uint32_t index) const noexcept { if (impl.get()) return impl->GetQueueFormFamilyIndex(index); return VK_NULL_HANDLE; }
    VkPhysicalDeviceProperties GetPhysicalDeviceProperties() const noexcept { if (impl.get()) return impl->GetPhysicalDeviceProperties(); return {}; }
    VkPhysicalDevice GetPhysicalDevice() const noexcept { if (impl.get()) return impl->GetPhysicalDevice(); return VK_NULL_HANDLE; }
    std::shared_ptr<Surface> GetSurface() const noexcept { if (impl.get()) return impl->GetSurface(); return {}; }
    VkDevice GetDevice() const noexcept { if (impl.get()) return impl->GetDevice(); return VK_NULL_HANDLE; }
    VkFormatProperties GetFormatProperties(const VkFormat format) const noexcept { if (impl.get()) return impl->GetFormatProperties(format); return {}; }
    VkBool32 CheckSampleCountSupport(VkSampleCountFlagBits x) const noexcept { if (impl.get()) return impl->CheckMultisampling(x); return false; }
    bool IsValid() const noexcept { return impl.get() && impl->device != VK_NULL_HANDLE; }
    ~Device() noexcept = default;
  };

  void swap(Device &lhs, Device &rhs) noexcept;
}

#endif