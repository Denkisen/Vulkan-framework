#ifndef __VULKAN_RENDERPASS_H
#define __VULKAN_RENDERPASS_H

#include <vulkan/vulkan.h>
#include <memory>

#include "Device.h"

namespace Vulkan
{
  class RenderPass_impl
  {
  public:
    RenderPass_impl() = delete;
    RenderPass_impl(const RenderPass_impl &obj) = delete;
    RenderPass_impl(RenderPass_impl &&obj) = delete;
    RenderPass_impl &operator=(const RenderPass_impl &obj) = delete;
    RenderPass_impl &operator=(RenderPass_impl &&obj) = delete;
    ~RenderPass_impl();
  private:
    friend class RenderPass;
    RenderPass_impl(const std::shared_ptr<Device> dev);

    std::shared_ptr<Device> device;
  };

  class RenderPass
  {
  private:
    std::unique_ptr<RenderPass_impl> impl;
  public:
    RenderPass() = delete;
    RenderPass(const RenderPass &obj) = delete;
    RenderPass(RenderPass &&obj) noexcept : impl(std::move(obj.impl)) {};
    RenderPass(const std::shared_ptr<Device> dev) : impl(std::unique_ptr<RenderPass_impl>(new RenderPass_impl(dev))) {};
    RenderPass &operator=(const RenderPass &obj) = delete;
    RenderPass &operator=(RenderPass &&obj) noexcept;
    void swap(RenderPass &obj) noexcept;
    bool IsValid() { return impl != nullptr; }
    ~RenderPass() = default;
  };

  void swap(RenderPass &lhs, RenderPass &rhs) noexcept;
}

#endif