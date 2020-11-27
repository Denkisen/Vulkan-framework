#ifndef __VULKAN_SEMAPHORE_H
#define __VULKAN_SEMAPHORE_H

#include "Device.h"

#include <memory>
#include <optional>
#include <vulkan/vulkan.h>

namespace Vulkan
{
  class Semaphore_impl
  {
  public:
    Semaphore_impl() = delete;
    Semaphore_impl(const Semaphore_impl &obj) = delete;
    Semaphore_impl(Semaphore_impl &&obj) = delete;
    Semaphore_impl &operator=(const Semaphore_impl &obj) = delete;
    Semaphore_impl &operator=(Semaphore_impl &&obj) = delete;
    ~Semaphore_impl() noexcept;
  private:
    friend class Semaphore;

    std::shared_ptr<Device> device;
    VkSemaphore sem = VK_NULL_HANDLE;
    VkSemaphoreCreateFlags flags = 0;

    Semaphore_impl(const std::shared_ptr<Device> dev, const VkSemaphoreCreateFlags flags);
    std::shared_ptr<Device> GetDevice() const noexcept { return device; }
    VkSemaphore GetSemaphore() const noexcept { return sem; }
  };

  class Semaphore
  {
  private:
    friend class FenceArray;
    std::unique_ptr<Semaphore_impl> impl;
  public:
    Semaphore() = delete;
    Semaphore(const Semaphore &obj);
    Semaphore(Semaphore &&obj) noexcept : impl(std::move(obj.impl)) {};
    Semaphore(const std::shared_ptr<Device> dev, const VkFenceCreateFlags flags = 0) : impl(std::unique_ptr<Semaphore_impl>(new Semaphore_impl(dev, flags))) {};
    Semaphore &operator=(const Semaphore &obj);
    Semaphore &operator=(Semaphore &&obj) noexcept;
    void swap(Semaphore &obj) noexcept;
    bool IsValid() const noexcept { return impl.get() && impl->sem != VK_NULL_HANDLE; }
    std::shared_ptr<Device> GetDevice() const noexcept { if (impl.get()) return impl->GetDevice(); return nullptr; }
    VkSemaphore GetSemaphore() const noexcept { if (impl.get()) return impl->GetSemaphore(); return VK_NULL_HANDLE; }
    ~Semaphore() noexcept = default;
  };

  void swap(Semaphore &lhs, Semaphore &rhs) noexcept;
}

#endif