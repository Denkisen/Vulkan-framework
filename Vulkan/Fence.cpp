#include "Fence.h"

namespace Vulkan
{
  Fence_impl::Fence_impl(const std::shared_ptr<Device> dev, const VkFenceCreateFlags flags)
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
    this->flags = flags;

    VkFenceCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    info.flags = this->flags;

    if (auto er = vkCreateFence(device->GetDevice(), &info, nullptr, &fence); er != VK_SUCCESS)
    {
      Logger::EchoError("Failed to create fence");
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }
  }

  Fence_impl::~Fence_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
    if (fence != VK_NULL_HANDLE)
      vkDestroyFence(device->GetDevice(), fence, nullptr);
  }

  Fence::Fence(const Fence &obj)
  {
    if (!obj.IsValid())
    {
      Logger::EchoError("Can't copy fence", __func__);
      return;
    }

    impl = std::unique_ptr<Fence_impl>(new Fence_impl(obj.impl->device, obj.impl->flags));
  }

  Fence &Fence::operator=(const Fence &obj)
  {
    if (!obj.IsValid())
    {
      Logger::EchoError("Can't copy fence", __func__);
      return *this;
    }

    impl = std::unique_ptr<Fence_impl>(new Fence_impl(obj.impl->device, obj.impl->flags));

    return *this;
  }

  Fence &Fence::operator=(Fence &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void Fence::swap(Fence &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(Fence &lhs, Fence &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }

  VkResult WaitForFences(const std::vector<Fence> fences, const uint64_t timeout, const VkBool32 wait_for_all)
  {
    if (fences.empty()) return VK_SUCCESS;

    VkDevice dev = fences[0].GetDevice()->GetDevice();
    if (dev == VK_NULL_HANDLE)
    {
      Logger::EchoError("Device is empty", __func__);
      return VK_ERROR_UNKNOWN;
    } 

    std::vector<VkFence> f;
    f.reserve(fences.size());
    for (size_t i = 1; i < fences.size(); ++i)
    {
      if (fences[i].IsValid() && fences[i].GetDevice()->GetDevice() == dev)
      {
        f.push_back(fences[i].GetFence());
      }
      else
      {
        Logger::EchoError("Fence is not valid", __func__);
        return VK_ERROR_UNKNOWN;
      }
    }
    return vkWaitForFences(dev, (uint32_t) f.size(), f.data(), wait_for_all, timeout);
  }

  FenceArray::FenceArray(const std::shared_ptr<Device> dev)
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
  }

  VkResult FenceArray::Add(const VkFenceCreateFlags flags)
  {
    auto ptr = std::shared_ptr<Fence>(new Fence(device, flags));
    if (ptr->IsValid())
    {
      fences.push_back(ptr);
      p_fences.push_back(ptr->impl->fence);
      return VK_SUCCESS;
    }

    Logger::EchoError("Fence is not valid", __func__);

    return VK_ERROR_UNKNOWN;
  }

  VkResult FenceArray::Add(const std::shared_ptr<Fence> &obj)
  {
    if (obj->IsValid() && obj->impl->device == device)
    {
      fences.push_back(obj);
      p_fences.push_back(obj->impl->fence);
      return VK_SUCCESS;
    }

    Logger::EchoError("Fence is not valid", __func__);

    return VK_ERROR_UNKNOWN;
  }

  VkResult FenceArray::Add(Fence&& obj)
  {
    auto ptr = std::shared_ptr<Fence>(new Fence(obj));
    if (ptr->IsValid() && obj.impl->device == device)
    {
      fences.push_back(ptr);
      p_fences.push_back(ptr->impl->fence);
      return VK_SUCCESS;
    }

    obj = std::move(*ptr);
    Logger::EchoError("Fence is not valid", __func__);

    return VK_ERROR_UNKNOWN;
  }

  VkResult FenceArray::WaitFor(const uint64_t timeout, const VkBool32 wait_for_all) const noexcept
  {
    if (p_fences.empty()) return VK_SUCCESS;
    return vkWaitForFences(device->GetDevice(), (uint32_t) p_fences.size(), p_fences.data(), wait_for_all, timeout);
  }

  VkResult FenceArray::ResetAll() noexcept
  {
    if (p_fences.empty()) return VK_SUCCESS;
    return vkResetFences(device->GetDevice(), (uint32_t) p_fences.size(), p_fences.data());
  }

  FenceArray::FenceArray(const FenceArray& obj)
  {
    if (!obj.IsValid())
    {
      Logger::EchoError("FenceArray is not valid", __func__);
      return;
    } 

    device = obj.device;
    for (auto &p : fences)
    {
      if (Add(p->impl->flags) != VK_SUCCESS)
      {
        Logger::EchoError("Can't add fence", __func__);
        Clear();
        return;
      }
    }
  }

  FenceArray::FenceArray(FenceArray&& obj) noexcept
  {
    device = std::move(obj.device);
    p_fences = std::move(obj.p_fences);
    fences = std::move(obj.fences);
  }

  FenceArray &FenceArray::operator=(const FenceArray& obj)
  {
    if (!obj.IsValid())
    {
      Logger::EchoError("FenceArray is not valid", __func__);
      return *this;
    } 

    device = obj.device;
    for (auto &p : fences)
    {
      if (Add(p->impl->flags) != VK_SUCCESS)
      {
        Logger::EchoError("Can't add fence", __func__);
        Clear();
        return *this;
      }
    }

    return *this;
  }

  FenceArray &FenceArray::operator=(FenceArray&& obj) noexcept
  {
    if (&obj == this) return *this;

    device = std::move(obj.device);
    p_fences = std::move(obj.p_fences);
    fences = std::move(obj.fences);

    return *this;
  }

  void FenceArray::swap(FenceArray& obj) noexcept
  {
    if (&obj == this) return;

    device.swap(obj.device);
    p_fences.swap(obj.p_fences);
    fences.swap(obj.fences);
  }

  void swap(FenceArray &lhs, FenceArray &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }
}