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
    if (dev.get() == nullptr)
      throw std::runtime_error("Device pointer is not valid.");
    device = dev;

    this->swapchain = swapchain;
    if (this->swapchain == nullptr)
      throw std::runtime_error("Swapchain pointer is not valid.");

    CreateDepthBuffer();
    render_pass = CreateRenderPass();
    frame_buffers = CreateFrameBuffers();
  }

  VkRenderPass RenderPass::CreateRenderPass()
  {
    VkRenderPass ret = VK_NULL_HANDLE;

    std::vector<VkAttachmentDescription> attachments(2);
    std::vector<VkAttachmentReference> attachment_refs(2);
    clear_colors.resize(2);

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    
    attachments[0].format = swapchain->GetSurfaceFormat().format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    clear_colors[0].color = {0.0f, 0.0f, 0.0f, 1.0f};

    attachment_refs[0].attachment = 0;
    attachment_refs[0].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &attachment_refs[0];

    attachments[1].format = depth_buffer->GetFormat();
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    clear_colors[1].depthStencil = {1.0f, 0};

    attachment_refs[1].attachment = 1;
    attachment_refs[1].layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    subpass.pDepthStencilAttachment = &attachment_refs[1];
  
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = (uint32_t) attachments.size();
    render_pass_info.pAttachments = attachments.data();
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
      std::vector<VkImageView> attachments(2);
      attachments[0] = image_views[i];
      attachments[1] = depth_buffer->GetImageView();

      VkFramebufferCreateInfo framebuffer_info = {};
      framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_info.renderPass = render_pass;
      framebuffer_info.attachmentCount = (uint32_t) attachments.size();
      framebuffer_info.pAttachments = attachments.data();
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

  void RenderPass::CreateDepthBuffer()
  {
    depth_buffer = std::make_shared<Vulkan::Image>(device, swapchain->GetExtent().width, 
                                                  swapchain->GetExtent().height,
                                                  Vulkan::ImageTiling::Optimal,
                                                  Vulkan::HostVisibleMemory::HostInvisible,
                                                  Vulkan::ImageType::DepthBuffer,
                                                  Vulkan::ImageFormat::Depth_32);
  }

  void RenderPass::ReBuildRenderPass()
  {
    vkDeviceWaitIdle(device->GetDevice());
    Destroy();
    CreateDepthBuffer();
    render_pass = CreateRenderPass();
    frame_buffers = CreateFrameBuffers();
  }
}