#ifndef __CPU_NW_LIBS_VULKAN_SWAPCHAIN_H
#define __CPU_NW_LIBS_VULKAN_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <memory>

#include "Instance.h"
#include "Supply.h"
#include "Device.h"

namespace Vulkan
{
  class SwapChain
  {
  private:
    SwapChainDetails capabilities;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkSwapchainKHR old_swapchain = VK_NULL_HANDLE;
    VkExtent2D extent;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    std::shared_ptr<Vulkan::Device> device;
    std::pair<uint32_t, uint32_t> size;
    uint32_t images_in_swapchain = 0;
    VkSurfaceFormatKHR GetSwapChainFormat();
    VkPresentModeKHR GetSwapChainPresentMode();
    VkExtent2D GetSwapChainExtent();
    void Create();
    void CreateImageViews();
    void Destroy();
  public:
    SwapChain() = delete;
    SwapChain(const SwapChain &obj) = delete;
    SwapChain& operator= (const SwapChain &obj) = delete;
    SwapChain(std::shared_ptr<Vulkan::Device> dev);
    ~SwapChain();
    VkSwapchainKHR GetSwapChain() { return swapchain; }
    VkExtent2D GetExtent() { return extent; }
    VkSurfaceFormatKHR GetSurfaceFormat() { return format; }
    std::vector<VkImageView> GetImageViews() { return swapchain_image_views; }
    size_t GetImageViewsCount() { return swapchain_image_views.size(); }
    void ReBuildSwapChain();
  };
}

#endif