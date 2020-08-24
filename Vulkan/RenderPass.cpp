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

    CreateMultisampleBuffer();
    CreateDepthBuffer();
    render_pass = CreateRenderPass();
    frame_buffers = CreateFrameBuffers();
  }

  VkRenderPass RenderPass::CreateRenderPass()
  {
    VkRenderPass ret = VK_NULL_HANDLE;

    VkAttachmentDescription color_attachment = {};
    VkAttachmentReference color_attachment_ref = {};
    VkAttachmentDescription depth_attachment = {};
    VkAttachmentReference depth_attachment_ref = {};
    VkAttachmentDescription sample_attachment = {};
    VkAttachmentReference sample_attachment_ref = {};

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    clear_colors.resize(2);
    
    color_attachment.format = swapchain->GetSurfaceFormat().format;
    color_attachment.samples = multisampling;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = multisampling != VK_SAMPLE_COUNT_1_BIT ? VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    clear_colors[0].color = {0.0f, 0.0f, 0.0f, 1.0f};

    color_attachment_ref.attachment = 0;
    color_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &color_attachment_ref;

    depth_attachment.format = depth_buffer->GetFormat();
    depth_attachment.samples = multisampling;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    clear_colors[1].depthStencil = {1.0f, 0};

    depth_attachment_ref.attachment = 1;
    depth_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    subpass.pDepthStencilAttachment = &depth_attachment_ref;

    if (multisampling != VK_SAMPLE_COUNT_1_BIT)
    {
      sample_attachment.format = swapchain->GetSurfaceFormat().format;
      sample_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
      sample_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      sample_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
      sample_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
      sample_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
      sample_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      sample_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

      sample_attachment_ref.attachment = 2;
      sample_attachment_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

      subpass.pResolveAttachments = &sample_attachment_ref;
    }
  
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 3> attachments;
    attachments[0] = color_attachment;
    attachments[1] = depth_attachment;
    if (multisampling != VK_SAMPLE_COUNT_1_BIT)
      attachments[2] = sample_attachment;

    VkRenderPassCreateInfo render_pass_info = {};
    render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount = multisampling != VK_SAMPLE_COUNT_1_BIT ? (uint32_t) attachments.size() : (uint32_t) attachments.size() - 1;
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
      std::array<VkImageView, 3> attachments;
      if (multisampling != VK_SAMPLE_COUNT_1_BIT) 
      {
        attachments[0] = multisampling_buffer->GetImageView();
        attachments[1] = depth_buffer->GetImageView();
        attachments[2] = image_views[i];
      }
      else
      {
        attachments[0] = image_views[i];
        attachments[1] = depth_buffer->GetImageView();
      }

      VkFramebufferCreateInfo framebuffer_info = {};
      framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
      framebuffer_info.renderPass = render_pass;
      framebuffer_info.attachmentCount = multisampling != VK_SAMPLE_COUNT_1_BIT ? (uint32_t) attachments.size() : (uint32_t) attachments.size() - 1;
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
                                                  false,
                                                  Vulkan::ImageTiling::Optimal,
                                                  Vulkan::HostVisibleMemory::HostInvisible,
                                                  Vulkan::ImageType::DepthBuffer,
                                                  Vulkan::ImageFormat::Depth_32,
                                                  multisampling);
  }

  void RenderPass::CreateMultisampleBuffer()
  {
    multisampling_buffer = std::make_shared<Vulkan::Image>(device, swapchain->GetExtent().width, 
                                                          swapchain->GetExtent().height,
                                                          false,
                                                          Vulkan::ImageTiling::Optimal,
                                                          Vulkan::HostVisibleMemory::HostInvisible,
                                                          Vulkan::ImageType::Multisampling,
                                                          swapchain->GetSurfaceFormat().format,
                                                          multisampling);
  }

  void RenderPass::ReBuildRenderPass()
  {
    vkDeviceWaitIdle(device->GetDevice());
    Destroy();
    CreateMultisampleBuffer();
    CreateDepthBuffer();
    render_pass = CreateRenderPass();
    frame_buffers = CreateFrameBuffers();
  }

  void RenderPass::SetSamplesCount(const VkSampleCountFlagBits count)
  {
    if (device->CheckMultisampling(count))
      multisampling = count;
  }
}