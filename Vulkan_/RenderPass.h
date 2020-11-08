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
    AttachmentConfig() = default;
    ~AttachmentConfig() = default;
    auto &SetTag(const std::string val) { tag = val; return *this; }
    auto &SetImageView(const VkImageView val) { view = val; return *this; }
    auto &SetAttachmentDescription(const VkAttachmentDescription val) { description = val; return *this; }
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
    SubpassConfig() = default;
    ~SubpassConfig() = default;
    auto &SetDepthReference(const uint32_t attachment_index, const VkImageLayout dst_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) 
    { 
      depth_ref = {attachment_index, dst_layout};
      return *this;
    }
    auto &AddInputReference(const uint32_t attachment_index, const VkImageLayout dst_layout) 
    { 
      input_refs.push_back({attachment_index, dst_layout});
      return *this; 
    }
    auto &AddColorReference(const uint32_t attachment_index, const VkImageLayout dst_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) 
    { 
      color_refs.push_back({attachment_index, dst_layout});
      return *this; 
    }
    auto &AddResolveReference(const uint32_t attachment_index, const VkImageLayout dst_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) 
    { 
      resolve_refs.push_back({attachment_index, dst_layout});
      return *this; 
    }
    auto &AddPreserveReference(const uint32_t attachment_index) 
    { 
      preserve.push_back(attachment_index);
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
    RenderPassConfig() = default;
    ~RenderPassConfig() = default;
    auto &AddAttachment(const AttachmentConfig val) { attach_configs.push_back(val); return *this; }
    auto &AddSubpass(const SubpassConfig val) { subpass_consfig.push_back(val); return *this; }
    auto &AddDependency(const VkSubpassDependency val) { dependencies.push_back(val); return *this; }
  };

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
    RenderPass_impl(const std::shared_ptr<Device> dev, const std::shared_ptr<SwapChain> swapchain, const RenderPassConfig &params);
    VkResult Create();
    void Clear();

    VkRenderPass GetRenderPass() { std::lock_guard lock(render_pass_mutex); return render_pass; }
    VkResult ReCreate();
    std::vector<VkFramebuffer> GetFrameBuffers() { std::lock_guard lock(render_pass_mutex); return frame_buffers; }
    uint32_t GetSubpassCount() { return conf.subpass_consfig.size(); }
    std::shared_ptr<SwapChain> GetSwapChain() { return swapchain; }

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
    RenderPass(const std::shared_ptr<Device> dev, const std::shared_ptr<SwapChain> swapchain, const RenderPassConfig &params) : impl(std::unique_ptr<RenderPass_impl>(new RenderPass_impl(dev, swapchain, params))) {};
    RenderPass &operator=(const RenderPass &obj) = delete;
    RenderPass &operator=(RenderPass &&obj) noexcept;
    void swap(RenderPass &obj) noexcept;
    bool IsValid() { return impl != nullptr; }
    VkRenderPass GetRenderPass() { return impl->GetRenderPass(); }
    VkResult ReCreate() { return impl->ReCreate(); }
    std::vector<VkFramebuffer> GetFrameBuffers() { return impl->GetFrameBuffers(); }
    uint32_t GetSubpassCount() { return impl->GetSubpassCount(); }
    VkExtent2D GetExtent() { return impl->GetSwapChain()->GetExtent(); }
    ~RenderPass() = default;
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