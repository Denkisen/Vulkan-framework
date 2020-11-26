#include "RenderPass.h"
#include "SwapChain.h"

namespace Vulkan::Helpers
{
  std::shared_ptr<RenderPass> CreateOneSubpassRenderPass(const std::shared_ptr<Device> dev, 
                                                            const std::shared_ptr<SwapChain> swapchain)
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is invalid", __func__);
      return nullptr;
    }

    if (swapchain.get() == nullptr || !swapchain->IsValid())
    {
      Logger::EchoError("Device is invalid", __func__);
      return nullptr;
    }

    Vulkan::RenderPassConfig config;
    VkAttachmentDescription desc = {};
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    desc.format = swapchain->GetSurfaceFormat().format;
    desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    config.AddAttachment(Vulkan::AttachmentConfig()
                             .SetAttachmentDescription(desc)
                             .SetImageView(VK_NULL_HANDLE));

    VkSubpassDependency dep = {};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    config.AddSubpass(Vulkan::SubpassConfig().AddColorReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));
    config.AddDependency(dep);

    auto result = std::make_shared<RenderPass>(dev, swapchain, config);
    if (result != nullptr && result->IsValid() && result->GetRenderPass() != VK_NULL_HANDLE)
    {
      return result;
    }
    
    Logger::EchoError("Can't create render pass", __func__);
    return nullptr;
  }
  std::shared_ptr<RenderPass> CreateOneSubpassRenderPassMultisamplingDepth(const std::shared_ptr<Device> dev, 
                                                            const std::shared_ptr<SwapChain> swapchain,
                                                            Vulkan::ImageArray &buffers,
                                                            const VkSampleCountFlagBits samples_count)
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is invalid", __func__);
      return nullptr;
    }

    if (swapchain.get() == nullptr || !swapchain->IsValid())
    {
      Logger::EchoError("Device is invalid", __func__);
      return nullptr;
    }

    if (samples_count == VK_SAMPLE_COUNT_1_BIT || !dev->CheckSampleCountSupport(samples_count))
    {
      Logger::EchoError("Samples count is invalid", __func__);
      return nullptr;
    }

    Vulkan::ImageArray tmp_buffers(dev);
    tmp_buffers.StartConfig();
    tmp_buffers.AddImage(Vulkan::ImageConfig()
                                   .PreallocateMipLevels(false)
                                   .SetFormat(VK_FORMAT_D32_SFLOAT)
                                   .SetSamplesCount(samples_count)
                                   .SetSize(swapchain->GetExtent().height, swapchain->GetExtent().width)
                                   .SetTiling(Vulkan::ImageTiling::Optimal)
                                   .SetType(Vulkan::ImageType::DepthBuffer)
                                   .SetMemoryAccess(Vulkan::HostVisibleMemory::HostInvisible));
    tmp_buffers.AddImage(Vulkan::ImageConfig()
                                   .PreallocateMipLevels(false)
                                   .SetFormat(swapchain->GetSurfaceFormat().format)
                                   .SetSamplesCount(samples_count)
                                   .SetSize(swapchain->GetExtent().height, swapchain->GetExtent().width)
                                   .SetTiling(Vulkan::ImageTiling::Optimal)
                                   .SetType(Vulkan::ImageType::Multisampling)
                                   .SetMemoryAccess(Vulkan::HostVisibleMemory::HostInvisible));
    tmp_buffers.EndConfig();

    Vulkan::RenderPassConfig config;

    VkAttachmentDescription desc = {};
    desc.format = tmp_buffers.GetInfo(1).image_info.format;
    desc.samples = samples_count;
    desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    desc.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    config.AddAttachment(Vulkan::AttachmentConfig()
                             .SetAttachmentDescription(desc)
                             .SetImageView(tmp_buffers.GetInfo(1).image_view)); // multisampling

    desc.format = tmp_buffers.GetInfo(0).image_info.format;
    desc.samples = samples_count;
    desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    config.AddAttachment(Vulkan::AttachmentConfig()
                             .SetAttachmentDescription(desc)
                             .SetImageView(tmp_buffers.GetInfo(0).image_view)); // depth

    desc.format = tmp_buffers.GetInfo(1).image_info.format;
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    config.AddAttachment(Vulkan::AttachmentConfig()
                             .SetAttachmentDescription(desc)
                             .SetImageView(VK_NULL_HANDLE)); // output/frame buffer

    config.AddSubpass(Vulkan::SubpassConfig()
                          .SetDepthReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
                          .AddColorReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
                          .AddResolveReference(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL));

    VkSubpassDependency dep = {};
    dep.srcSubpass = VK_SUBPASS_EXTERNAL;
    dep.dstSubpass = 0;
    dep.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.srcAccessMask = 0;
    dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    config.AddDependency(dep);

    auto result = std::make_shared<RenderPass>(dev, swapchain, config);
    if (result != nullptr && result->IsValid() && result->GetRenderPass() != VK_NULL_HANDLE)
    {
      buffers.swap(tmp_buffers);
      return result;
    }
    
    Logger::EchoError("Can't create render pass", __func__);
    return nullptr;
  }
}