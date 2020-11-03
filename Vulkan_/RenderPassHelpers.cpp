#include "RenderPass.h"
#include "SwapChain.h"

namespace Vulkan::Helpers
{
  std::shared_ptr<RenderPass> CreateOneSubpassRenderPassMultisamplingDepth(const std::shared_ptr<Device> dev, 
                                                            const std::shared_ptr<SwapChain> swapchain,
                                                            Vulkan::ImageArray &buffers,
                                                            const VkSampleCountFlagBits samples_count)
  {
    if (dev.get() == nullptr || !dev->IsValid() || dev->GetDevice() == VK_NULL_HANDLE)
    {
      Logger::EchoError("Device is invalid", __func__);
      return nullptr;
    }

    if (swapchain.get() == nullptr || !swapchain->IsValid() || swapchain->GetSwapChain() == VK_NULL_HANDLE)
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
    VkSampleCountFlagBits multisampling = VK_SAMPLE_COUNT_4_BIT;
    tmp_buffers.StartConfig(Vulkan::HostVisibleMemory::HostInvisible);
    tmp_buffers.AddImage(Vulkan::ImageConfig()
                                   .PreallocateMipLevels(false)
                                   .SetFormat(VK_FORMAT_D32_SFLOAT)
                                   .SetSamplesCount(multisampling)
                                   .SetSize(swapchain->GetExtent().height, swapchain->GetExtent().width)
                                   .SetTiling(Vulkan::ImageTiling::Optimal)
                                   .SetType(Vulkan::ImageType::DepthBuffer));
    tmp_buffers.AddImage(Vulkan::ImageConfig()
                                   .PreallocateMipLevels(false)
                                   .SetFormat(swapchain->GetSurfaceFormat().format)
                                   .SetSamplesCount(multisampling)
                                   .SetSize(swapchain->GetExtent().height, swapchain->GetExtent().width)
                                   .SetTiling(Vulkan::ImageTiling::Optimal)
                                   .SetType(Vulkan::ImageType::Multisampling));
    tmp_buffers.EndConfig();

    Vulkan::RenderPassConfig config;

    VkAttachmentDescription desc = {};
    desc.format = tmp_buffers.GetInfo(1).image_info.format;
    desc.samples = multisampling;
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
    desc.samples = multisampling;
    desc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    config.AddAttachment(Vulkan::AttachmentConfig()
                             .SetAttachmentDescription(desc)
                             .SetImageView(tmp_buffers.GetInfo(0).image_view)); // depth

    desc.format = tmp_buffers.GetInfo(1).image_info.format;
    desc.samples = VK_SAMPLE_COUNT_1_BIT;
    desc.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    desc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    config.AddAttachment(Vulkan::AttachmentConfig()
                             .SetAttachmentDescription(desc)
                             .SetImageView(VK_NULL_HANDLE)); // output/frame buffer

    config.AddSubpass(Vulkan::SubpassConfig()
                          .SetDepthReference(1, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL)
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