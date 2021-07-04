#ifndef __VULKAN_FENCE_H
#define __VULKAN_FENCE_H

#include "Logger.h"
#include "Device.h"

#include <memory>
#include <optional>
#include <vulkan/vulkan.h>

namespace Vulkan
{
  class Fence_impl
  {
  public:
    Fence_impl() = delete;
    Fence_impl(const Fence_impl &obj) = delete;
    Fence_impl(Fence_impl &&obj) = delete;
    Fence_impl &operator=(const Fence_impl &obj) = delete;
    Fence_impl &operator=(Fence_impl &&obj) = delete;
    ~Fence_impl() noexcept;
  private:
    friend class Fence;
    friend class FenceArray;

    std::shared_ptr<Device> device;
    VkFence fence = VK_NULL_HANDLE;
    VkFenceCreateFlags flags = 0;

    Fence_impl(const std::shared_ptr<Device> dev, const VkFenceCreateFlags flags);
    std::shared_ptr<Device> GetDevice() const noexcept { return device; }
    VkFence GetFence() const noexcept { return fence; }
    std::optional<VkBool32> GetState() const noexcept { return fence != VK_NULL_HANDLE ? vkGetFenceStatus(device->GetDevice(), fence) : std::optional<VkBool32>(); }
    VkResult Wait(const uint64_t timeout) const noexcept { return vkWaitForFences(device->GetDevice(), 1, &fence, VK_TRUE, timeout); }
    VkResult Reset() noexcept { return vkResetFences(device->GetDevice(), 1, &fence); }
  };

  class Fence
  {
  private:
    friend class FenceArray;
    std::unique_ptr<Fence_impl> impl;
  public:
    Fence() = delete;
    Fence(const Fence &obj);
    Fence(Fence &&obj) noexcept : impl(std::move(obj.impl)) {};
    Fence(const std::shared_ptr<Device> dev, const VkFenceCreateFlags flags = 0) : impl(std::unique_ptr<Fence_impl>(new Fence_impl(dev, flags))) {};
    Fence &operator=(const Fence &obj);
    Fence &operator=(Fence &&obj) noexcept;
    void swap(Fence &obj) noexcept;
    bool IsValid() const noexcept { return impl.get() && impl->fence != VK_NULL_HANDLE; }
    std::shared_ptr<Device> GetDevice() const noexcept { if (impl.get()) return impl->GetDevice(); return nullptr; }
    VkFence GetFence() const noexcept { if (impl.get()) return impl->GetFence(); return VK_NULL_HANDLE; }
    std::optional<VkBool32> GetState() const noexcept { if (impl.get()) return impl->GetState(); return std::optional<VkBool32>(); }
    VkResult Wait(const uint64_t timeout = UINT64_MAX) const noexcept { if (impl.get()) return impl->Wait(timeout); return VK_ERROR_UNKNOWN; }
    VkResult Reset() noexcept { if (impl.get()) return impl->Reset(); return VK_ERROR_UNKNOWN; }
    ~Fence() noexcept = default;
  };

  class FenceArray
  {
  private:
    std::shared_ptr<Device> device;
    std::vector<VkFence> p_fences;
    std::vector<std::shared_ptr<Fence>> fences;
  public:
    FenceArray() = delete;
    FenceArray(const std::shared_ptr<Device> dev);
    FenceArray(const FenceArray &obj);
    FenceArray(FenceArray &&obj) noexcept;
    FenceArray &operator=(const FenceArray &obj);
    FenceArray &operator=(FenceArray &&obj) noexcept;
    VkFence operator[](const size_t index) noexcept { return index < p_fences.size() ? p_fences[index] : VK_NULL_HANDLE; }
    void swap(FenceArray &obj) noexcept;
    VkResult Add(const VkFenceCreateFlags flags = 0);
    VkResult Add(const std::shared_ptr<Fence> &obj);
    VkResult Add(Fence &&obj);
    VkResult WaitFor(const uint64_t timeout = UINT64_MAX, const VkBool32 wait_for_all = VK_TRUE) const noexcept;
    VkResult ResetAll() noexcept;
    size_t Count() const noexcept { return fences.size(); }
    void Clear() noexcept { p_fences.clear(); fences.clear(); };
    std::shared_ptr<Fence> GetFence(const size_t index) const noexcept { return index < fences.size() ? fences[index] : nullptr; };
    std::shared_ptr<Device> GetDevice() const noexcept { return device; }
    bool IsValid() const noexcept { return device->IsValid(); }
    ~FenceArray() noexcept = default;
  };

  void swap(Fence &lhs, Fence &rhs) noexcept;
  void swap(FenceArray &lhs, FenceArray &rhs) noexcept;
  VkResult WaitForFences(const std::vector<Fence> fences, const uint64_t timeout = UINT64_MAX, const VkBool32 wait_for_all = VK_TRUE);
}

#endif