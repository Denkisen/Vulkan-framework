#include "RenderPass.h"
#include "Logger.h"

namespace Vulkan
{
  RenderPass_impl::~RenderPass_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
    Clear();
  }

  void RenderPass_impl::Clear() noexcept
  {
    for (auto framebuffer : frame_buffers) 
    {
      if (framebuffer != VK_NULL_HANDLE)
        vkDestroyFramebuffer(device->GetDevice(), framebuffer, nullptr);
      framebuffer = VK_NULL_HANDLE;
    }
    if (render_pass != VK_NULL_HANDLE)
      vkDestroyRenderPass(device->GetDevice(), render_pass, nullptr);
    render_pass = VK_NULL_HANDLE;
  }

  RenderPass_impl::RenderPass_impl(const std::shared_ptr<Device> dev, const std::shared_ptr<SwapChain> swapchain, const RenderPassConfig &params)
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    if (swapchain.get() == nullptr || !swapchain->IsValid())
    {
      Logger::EchoError("SwapChainis empty", __func__);
      return;
    }

    device = dev;
    this->swapchain = swapchain;
    conf = params;

    if (auto er = Create(); er != VK_SUCCESS)
    {
      Logger::EchoDebug("Can't create render pass", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }
  }

  VkResult RenderPass_impl::ReCreate()
  {
    vkDeviceWaitIdle(device->GetDevice());
    return Create();
  }

  VkResult RenderPass_impl::Create()
  {
    Clear();

    std::vector<VkAttachmentDescription> attachments(conf.attach_configs.size());
    std::vector<VkImageView> attachment_views(conf.attach_configs.size());

    for (size_t i = 0; i < conf.attach_configs.size(); ++i)
    {
      attachments[i] = conf.attach_configs[i].description;
      attachment_views[i] = conf.attach_configs[i].view;
    }

    std::vector<VkSubpassDescription> subpasses(conf.subpass_consfig.size());

    for (size_t i = 0; i < conf.subpass_consfig.size(); ++i)
    {
      subpasses[i].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;

      subpasses[i].colorAttachmentCount = (uint32_t)conf.subpass_consfig[i].color_refs.size();
      subpasses[i].pColorAttachments = subpasses[i].colorAttachmentCount > 0 ? conf.subpass_consfig[i].color_refs.data() : nullptr;
      subpasses[i].pResolveAttachments = subpasses[i].colorAttachmentCount > 0 ? conf.subpass_consfig[i].resolve_refs.data() : nullptr;

      subpasses[i].inputAttachmentCount = (uint32_t)conf.subpass_consfig[i].input_refs.size();
      subpasses[i].pInputAttachments = subpasses[i].inputAttachmentCount > 0 ? conf.subpass_consfig[i].input_refs.data() : nullptr;

      subpasses[i].preserveAttachmentCount = (uint32_t)conf.subpass_consfig[i].preserve.size();
      subpasses[i].pPreserveAttachments = subpasses[i].preserveAttachmentCount > 0 ? conf.subpass_consfig[i].preserve.data() : nullptr;
    }

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = (uint32_t)attachments.size();
    render_pass_info.pAttachments = attachments.data();
    render_pass_info.subpassCount = (uint32_t)subpasses.size();
    render_pass_info.pSubpasses = subpasses.data();
    render_pass_info.dependencyCount = (uint32_t)conf.dependencies.size();
    render_pass_info.pDependencies = conf.dependencies.data();

    if (auto er = vkCreateRenderPass(device->GetDevice(), &render_pass_info, nullptr, &render_pass); er != VK_SUCCESS)
    {
      Logger::EchoDebug("Failed to create render pass", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return er;
    }

    auto image_views = swapchain->GetImageViews();
    if (image_views.empty())
    {
      Logger::EchoDebug("No image views in swapchain", __func__);
      return VK_ERROR_UNKNOWN;
    }

    frame_buffers.resize(image_views.size());

    for (size_t i = 0; i < image_views.size(); ++i)
    {
      attachment_views[attachment_views.size() - 1] = image_views[i];
      VkFramebufferCreateInfo framebuffer_info = {};
      framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_info.renderPass = render_pass;
      framebuffer_info.attachmentCount = (uint32_t)attachment_views.size();
      framebuffer_info.pAttachments = attachment_views.data();
      framebuffer_info.width = swapchain->GetExtent().width;
      framebuffer_info.height = swapchain->GetExtent().height;
      framebuffer_info.layers = 1;

      if (auto er = vkCreateFramebuffer(device->GetDevice(), &framebuffer_info, nullptr, &frame_buffers[i]); er != VK_SUCCESS)
      {
        Logger::EchoDebug("Failed to create framebuffer", __func__);
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
        return er;
      }
    }

    return VK_SUCCESS;
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