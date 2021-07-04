#include "Semaphore.h"

namespace Vulkan
{
  Semaphore_impl::Semaphore_impl(const std::shared_ptr<Device> dev, const VkSemaphoreCreateFlags flags)
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
    this->flags = flags;
    VkSemaphoreCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    info.flags = this->flags;

    if (auto er = vkCreateSemaphore(device->GetDevice(), &info, nullptr, &sem); er != VK_SUCCESS)
    {
      Logger::EchoError("Failed to create semaphore");
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }
  }

  Semaphore_impl::~Semaphore_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
    if (sem != VK_NULL_HANDLE)
      vkDestroySemaphore(device->GetDevice(), sem, nullptr);
  }

  Semaphore::Semaphore(const Semaphore &obj)
  {
    if (!obj.IsValid())
    {
      Logger::EchoError("Can't copy fence", __func__);
      return;
    }

    impl = std::unique_ptr<Semaphore_impl>(new Semaphore_impl(obj.impl->device, obj.impl->flags));
  }

  Semaphore &Semaphore::operator=(const Semaphore &obj)
  {
    if (!obj.IsValid())
    {
      Logger::EchoError("Can't copy semaphore", __func__);
      return *this;
    }

    impl = std::unique_ptr<Semaphore_impl>(new Semaphore_impl(obj.impl->device, obj.impl->flags));

    return *this;
  }

  Semaphore &Semaphore::operator=(Semaphore &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void Semaphore::swap(Semaphore &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(Semaphore &lhs, Semaphore &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }

  SemaphoreArray::SemaphoreArray(const std::shared_ptr<Device> dev)
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
  }

  SemaphoreArray::SemaphoreArray(const SemaphoreArray &obj)
  {
    if (!obj.IsValid())
    {
      Logger::EchoError("SemaphoreArray is not valid", __func__);
      return;
    } 

    device = obj.device;
    for (auto &p : semaphores)
    {
      if (Add(p->impl->flags) != VK_SUCCESS)
      {
        Logger::EchoError("Can't add semaphore", __func__);
        Clear();
        return;
      }
    }
  }

  SemaphoreArray::SemaphoreArray(SemaphoreArray &&obj) noexcept
  {
    device = std::move(obj.device);
    p_semaphores = std::move(obj.p_semaphores);
    semaphores = std::move(obj.semaphores);
  }

  SemaphoreArray &SemaphoreArray::operator=(const SemaphoreArray &obj)
  {
    if (!obj.IsValid())
    {
      Logger::EchoError("SemaphoreArray is not valid", __func__);
      return *this;
    } 

    device = obj.device;
    for (auto &p : semaphores)
    {
      if (Add(p->impl->flags) != VK_SUCCESS)
      {
        Logger::EchoError("Can't add semaphore", __func__);
        Clear();
        return *this;
      }
    }

    return *this;
  }

  SemaphoreArray &SemaphoreArray::operator=(SemaphoreArray &&obj) noexcept
  {
    if (&obj == this) return *this;

    device = std::move(obj.device);
    p_semaphores = std::move(obj.p_semaphores);
    semaphores = std::move(obj.semaphores);

    return *this;
  }

  void SemaphoreArray::swap(SemaphoreArray &obj) noexcept
  {
    if (&obj == this) return;

    device.swap(obj.device);
    p_semaphores.swap(obj.p_semaphores);
    semaphores.swap(obj.semaphores);
  }

  void swap(SemaphoreArray &lhs, SemaphoreArray &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }

  VkResult SemaphoreArray::Add(const VkSemaphoreCreateFlags flags)
  {
    auto ptr = std::shared_ptr<Semaphore>(new Semaphore(device, flags));
    if (ptr->IsValid())
    {
      semaphores.push_back(ptr);
      p_semaphores.push_back(ptr->impl->sem);
      return VK_SUCCESS;
    }

    Logger::EchoError("Fence is not valid", __func__);

    return VK_ERROR_UNKNOWN;
  }

  VkResult SemaphoreArray::Add(const std::shared_ptr<Semaphore> &obj)
  {
    if (obj->IsValid() && obj->impl->device == device)
    {
      semaphores.push_back(obj);
      p_semaphores.push_back(obj->impl->sem);
      return VK_SUCCESS;
    }

    Logger::EchoError("Fence is not valid", __func__);

    return VK_ERROR_UNKNOWN;
  }

  VkResult SemaphoreArray::Add(Semaphore &&obj)
  {
    auto ptr = std::shared_ptr<Semaphore>(new Semaphore(obj));
    if (ptr->IsValid() && obj.impl->device == device)
    {
      semaphores.push_back(ptr);
      p_semaphores.push_back(ptr->impl->sem);
      return VK_SUCCESS;
    }

    obj = std::move(*ptr);
    Logger::EchoError("Fence is not valid", __func__);

    return VK_ERROR_UNKNOWN;
  }
}