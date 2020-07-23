#include "RenderPass.h"

namespace Vulkan
{
  void RenderPass::Destroy()
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

  RenderPass::~RenderPass()
  {
#ifdef DEBUG
    std::cout << __func__ << std::endl;
#endif
    Destroy();
  }

  RenderPass::RenderPass(std::shared_ptr<Vulkan::Device> dev, std::shared_ptr<Vulkan::SwapChain> swapchain)
  {
    if (dev == nullptr)
      throw std::runtime_error("Device pointer is not valid.");
    device = dev;

    this->swapchain = swapchain;
    if (this->swapchain == nullptr)
      throw std::runtime_error("Swapchain pointer is not valid.");

    render_pass = CreateRenderPass();
    frame_buffers = CreateFrameBuffers();
  }

  VkRenderPass RenderPass::CreateRenderPass()
  {
    VkRenderPass ret = VK_NULL_HANDLE;
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = swapchain->GetSurfaceFormat().format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference color_attachment_ref = {};
    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = 1;
    render_pass_info.pAttachments = &color_attachment;
    render_pass_info.subpassCount = 1;
    render_pass_info.pSubpasses = &subpass;
    render_pass_info.dependencyCount = 1;
    render_pass_info.pDependencies = &dependency;

    if (vkCreateRenderPass(device->GetDevice(), &render_pass_info, nullptr, &ret) != VK_SUCCESS) 
    {
      throw std::runtime_error("Failed to create render pass!");
    }
    return ret;
  }

  std::vector<VkFramebuffer> RenderPass::CreateFrameBuffers()
  {
    auto image_views = swapchain->GetImageViews();
    std::vector<VkFramebuffer> frame_buffers(image_views.size());
    for (size_t i = 0; i < image_views.size(); i++) 
    {
      VkFramebufferCreateInfo framebuffer_info = {};
      framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_info.renderPass = render_pass;
      framebuffer_info.attachmentCount = 1;
      framebuffer_info.pAttachments = &image_views[i];
      framebuffer_info.width = swapchain->GetExtent().width;
      framebuffer_info.height = swapchain->GetExtent().height;
      framebuffer_info.layers = 1;

      if (vkCreateFramebuffer(device->GetDevice(), &framebuffer_info, nullptr, &frame_buffers[i]) != VK_SUCCESS) 
      {
        throw std::runtime_error("Failed to create framebuffer!");
      }
    }

    return frame_buffers;
  }

  void RenderPass::ReBuildRenderPass()
  {
    vkDeviceWaitIdle(device->GetDevice());
    Destroy();
    render_pass = CreateRenderPass();
    frame_buffers = CreateFrameBuffers();
  }
}