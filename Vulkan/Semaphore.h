#ifndef __VULKAN_SEMAPHORE_H
#define __VULKAN_SEMAPHORE_H

#include "Logger.h"
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
    friend class SemaphoreArray;
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
    friend class SemaphoreArray;
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

  class SemaphoreArray
  {
  private:
    std::shared_ptr<Device> device;
    std::vector<VkSemaphore> p_semaphores;
    std::vector<std::shared_ptr<Semaphore>> semaphores;
  public:
    SemaphoreArray() = delete;
    SemaphoreArray(const std::shared_ptr<Device> dev);
    SemaphoreArray(const SemaphoreArray &obj);
    SemaphoreArray(SemaphoreArray &&obj) noexcept;
    SemaphoreArray &operator=(const SemaphoreArray &obj);
    SemaphoreArray &operator=(SemaphoreArray &&obj) noexcept;
    VkSemaphore operator[](const size_t index) noexcept { return index < p_semaphores.size() ? p_semaphores[index] : VK_NULL_HANDLE; }
    void swap(SemaphoreArray &obj) noexcept;
    VkResult Add(const VkSemaphoreCreateFlags flags = 0);
    VkResult Add(const std::shared_ptr<Semaphore> &obj);
    VkResult Add(Semaphore &&obj);
    size_t Count() const noexcept { return semaphores.size(); }
    void Clear() noexcept { p_semaphores.clear(); semaphores.clear(); };
    std::shared_ptr<Semaphore> GetSemaphore(const size_t index) const noexcept { return index < semaphores.size() ? semaphores[index] : nullptr; };
    std::shared_ptr<Device> GetDevice() const noexcept { return device; }
    bool IsValid() const noexcept { return device->IsValid(); }
    ~SemaphoreArray() noexcept = default;
  };

  void swap(Semaphore &lhs, Semaphore &rhs) noexcept;
  void swap(SemaphoreArray &lhs, SemaphoreArray &rhs) noexcept;
}

#endif