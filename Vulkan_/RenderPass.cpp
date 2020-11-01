#include "RenderPass.h"
#include "Logger.h"

namespace Vulkan
{
  RenderPass_impl::~RenderPass_impl()
  {
    Logger::EchoDebug("", __func__);
  }

  RenderPass_impl::RenderPass_impl(const std::shared_ptr<Device> dev)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
  }

  RenderPass &RenderPass::operator=(RenderPass &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void RenderPass::swap(RenderPass &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(RenderPass &lhs, RenderPass &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  } 
}