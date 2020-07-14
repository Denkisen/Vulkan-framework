#ifndef __CPU_NW_LIBS_VULKAN_RENDERPASS_H
#define __CPU_NW_LIBS_VULKAN_RENDERPASS_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

#include "Device.h"
#include "SwapChain.h"
#include "Supply.h"

namespace Vulkan
{
  class RenderPass
  {
  private:
    std::shared_ptr<Vulkan::Device> device;
    std::shared_ptr<Vulkan::SwapChain> swapchain;
    VkRenderPass render_pass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> frame_buffers;
    VkRenderPass CreateRenderPass();
    std::vector<VkFramebuffer> CreateFrameBuffers();
  public:
    RenderPass() = delete;
    RenderPass(const RenderPass &obj) = delete;
    RenderPass& operator= (const RenderPass &obj) = delete;
    RenderPass(std::shared_ptr<Vulkan::Device> dev, std::shared_ptr<Vulkan::SwapChain> swapchain);
    VkRenderPass GetRenderPass() { return render_pass; }
    std::vector<VkFramebuffer> GetFrameBuffers() { return frame_buffers; }
    size_t GetFrameBuffersCount() { return frame_buffers.size(); }
    ~RenderPass();
  };
}


#endif