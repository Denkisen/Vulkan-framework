#ifndef __CPU_NW_VULKAN_RENDERPASS_H
#define __CPU_NW_VULKAN_RENDERPASS_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>
#include <vector>

#include "Device.h"
#include "SwapChain.h"
#include "Supply.h"
#include "Image.h"

namespace Vulkan
{
  class RenderPass
  {
  private:
    std::shared_ptr<Vulkan::Device> device;
    std::shared_ptr<Vulkan::SwapChain> swapchain;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> frame_buffers;
    std::shared_ptr<Vulkan::Image> depth_buffer;
    std::vector<VkClearValue> clear_colors;
    VkRenderPass CreateRenderPass();
    std::vector<VkFramebuffer> CreateFrameBuffers();
    void CreateDepthBuffer();
    void Destroy();
  public:
    RenderPass() = delete;
    RenderPass(const RenderPass &obj) = delete;
    RenderPass& operator= (const RenderPass &obj) = delete;
    RenderPass(std::shared_ptr<Vulkan::Device> dev, std::shared_ptr<Vulkan::SwapChain> swapchain);
    VkRenderPass GetRenderPass() const { return render_pass; }
    std::vector<VkFramebuffer> GetFrameBuffers() const { return frame_buffers; }
    size_t GetFrameBuffersCount() const { return frame_buffers.size(); }
    VkExtent2D GetSwapChainExtent() const { return swapchain->GetExtent(); }
    void ReBuildRenderPass();
    std::vector<VkClearValue> GetClearColors() const { return clear_colors; }
    ~RenderPass();
  };
}


#endif