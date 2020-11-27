#include "Semaphore.h"
#include "Logger.h"

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
}