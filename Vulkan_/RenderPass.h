#ifndef __VULKAN_RENDERPASS_H
#define __VULKAN_RENDERPASS_H

#include <vulkan/vulkan.h>
#include <memory>
#include <mutex>

#include "Device.h"
#include "SwapChain.h"
#include "ImageArray.h"

namespace Vulkan
{
  enum class AttachmentType
  {
    DepthStencil,
    Resolve,
    Color,
    Input
  };

  class AttachmentConfig
  {
  private:
    friend class RenderPass_impl;
    VkImageView view = VK_NULL_HANDLE;
    VkAttachmentDescription description;
    std::string tag = "";
  public:
    AttachmentConfig() noexcept = default;
    ~AttachmentConfig() noexcept = default;
    auto &SetTag(const std::string val) noexcept { try { tag = val; } catch (...) { } return *this; }
    auto &SetImageView(const VkImageView val) noexcept { view = val; return *this; }
    auto &SetAttachmentDescription(const VkAttachmentDescription val) noexcept { description = val; return *this; }
  };

  class SubpassConfig
  {
  private:
    friend class RenderPass_impl;
    std::vector<VkAttachmentReference> color_refs;
    VkAttachmentReference depth_ref = {};
    std::vector<VkAttachmentReference> resolve_refs;
    std::vector<VkAttachmentReference> input_refs;
    std::vector<uint32_t> preserve;
  public:
    SubpassConfig() noexcept = default;
    ~SubpassConfig() noexcept = default;
    auto &SetDepthReference(const uint32_t attachment_index, const VkImageLayout dst_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) noexcept
    { 
      try { depth_ref = {attachment_index, dst_layout}; } catch (...) { }
      return *this;
    }
    auto &AddInputReference(const uint32_t attachment_index, const VkImageLayout dst_layout) noexcept
    { 
      try { input_refs.push_back({attachment_index, dst_layout}); } catch (...) { }
      return *this; 
    }
    auto &AddColorReference(const uint32_t attachment_index, const VkImageLayout dst_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) noexcept
    { 
      try { color_refs.push_back({attachment_index, dst_layout}); } catch (...) { }
      return *this; 
    }
    auto &AddResolveReference(const uint32_t attachment_index, const VkImageLayout dst_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) noexcept
    { 
      try { resolve_refs.push_back({attachment_index, dst_layout}); } catch (...) { }
      return *this; 
    }
    auto &AddPreserveReference(const uint32_t attachment_index) noexcept
    { 
      try { preserve.push_back(attachment_index); } catch (...) { }
      return *this; 
    }
  };
  

  class RenderPassConfig
  {
  private:
    friend class RenderPass_impl;
    std::vector<AttachmentConfig> attach_configs;
    std::vector<SubpassConfig> subpass_consfig;
    std::vector<VkSubpassDependency> dependencies;
  public:
    RenderPassConfig() noexcept = default;
    ~RenderPassConfig() noexcept = default;
    auto &AddAttachment(const AttachmentConfig val) noexcept { try { attach_configs.push_back(val);  } catch (...) { }return *this; }
    auto &AddSubpass(const SubpassConfig val) noexcept { try { subpass_consfig.push_back(val); } catch (...) { } return *this; }
    auto &AddDependency(const VkSubpassDependency val) noexcept { try { dependencies.push_back(val); } catch (...) { } return *this; }
  };

  class RenderPass_impl
  {
  public:
    RenderPass_impl() = delete;
    RenderPass_impl(const RenderPass_impl &obj) = delete;
    RenderPass_impl(RenderPass_impl &&obj) = delete;
    RenderPass_impl &operator=(const RenderPass_impl &obj) = delete;
    RenderPass_impl &operator=(RenderPass_impl &&obj) = delete;
    ~RenderPass_impl() noexcept;
  private:
    friend class RenderPass;
    RenderPass_impl(const std::shared_ptr<Device> dev, const std::shared_ptr<SwapChain> swapchain, const RenderPassConfig &params) noexcept;
    VkResult Create() noexcept;
    void Clear() noexcept;

    VkRenderPass GetRenderPass() noexcept { std::lock_guard lock(render_pass_mutex); return render_pass; }
    VkResult ReCreate() noexcept;
    std::vector<VkFramebuffer> GetFrameBuffers() noexcept { std::lock_guard lock(render_pass_mutex); return frame_buffers; }
    uint32_t GetSubpassCount() const noexcept { return conf.subpass_consfig.size(); }
    std::shared_ptr<SwapChain> GetSwapChain() const noexcept { return swapchain; }

    std::shared_ptr<Device> device;
    std::shared_ptr<SwapChain> swapchain;
    std::vector<VkFramebuffer> frame_buffers;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    RenderPassConfig conf;
    std::mutex render_pass_mutex;
  };

  class RenderPass
  {
  private:
    std::unique_ptr<RenderPass_impl> impl;
  public:
    RenderPass() = delete;
    RenderPass(const RenderPass &obj) = delete;
    RenderPass(RenderPass &&obj) noexcept : impl(std::move(obj.impl)) {};
    RenderPass(const std::shared_ptr<Device> dev, const std::shared_ptr<SwapChain> swapchain, const RenderPassConfig &params) noexcept : 
      impl(std::unique_ptr<RenderPass_impl>(new RenderPass_impl(dev, swapchain, params))) {};
    RenderPass &operator=(const RenderPass &obj) = delete;
    RenderPass &operator=(RenderPass &&obj) noexcept;
    void swap(RenderPass &obj) noexcept;
    bool IsValid() const noexcept { return impl.get() && impl->render_pass && !impl->frame_buffers.empty(); }
    VkRenderPass GetRenderPass() noexcept { if (impl.get()) return impl->GetRenderPass(); return VK_NULL_HANDLE; }
    VkResult ReCreate() noexcept { if (impl.get()) return impl->ReCreate(); return VK_ERROR_UNKNOWN; }
    std::vector<VkFramebuffer> GetFrameBuffers() noexcept { if (impl.get()) return impl->GetFrameBuffers(); return {}; }
    uint32_t GetSubpassCount() const noexcept { if (impl.get()) return impl->GetSubpassCount(); return 0; }
    VkExtent2D GetExtent() noexcept { if (impl.get()) return impl->GetSwapChain()->GetExtent(); return {}; }
    ~RenderPass() noexcept = default;
  };

  void swap(RenderPass &lhs, RenderPass &rhs) noexcept;

  namespace Helpers
  {
    std::shared_ptr<RenderPass> CreateOneSubpassRenderPassMultisamplingDepth(const std::shared_ptr<Device> dev, 
                                                              const std::shared_ptr<SwapChain> swapchain,
                                                              ImageArray &buffers,
                                                              const VkSampleCountFlagBits samples_count);
  }
}

#endif