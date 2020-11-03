#ifndef __VULKAN_SWAPCHAIN_H
#define __VULKAN_SWAPCHAIN_H

#include <vulkan/vulkan.h>
#include <memory>
#include <mutex>

#include "Device.h"
#include "Misc.h"

namespace Vulkan
{
  class SwapChain_impl
  {
  public:
    SwapChain_impl() = delete;
    SwapChain_impl(const SwapChain_impl &obj) = delete;
    SwapChain_impl(SwapChain_impl &&obj) = delete;
    SwapChain_impl &operator=(const SwapChain_impl &obj) = delete;
    SwapChain_impl &operator=(SwapChain_impl &&obj) = delete;
    ~SwapChain_impl();
  private:
    friend class SwapChain;
    SwapChain_impl(const std::shared_ptr<Device> dev, const VkPresentModeKHR mode);
    VkResult Create();
    VkSurfaceFormatKHR GetSwapChainFormat();
    VkPresentModeKHR GetSwapChainPresentMode();
    VkExtent2D GetSwapChainExtent();
    VkResult CreateImageViews();

    VkResult ReCreate();
    VkResult SetPresentMode(const VkPresentModeKHR mode);
    VkSurfaceFormatKHR GetSurfaceFormat() { std::lock_guard lock(swapchain_mutex); return format; }
    VkSwapchainKHR GetSwapChain() { std::lock_guard lock(swapchain_mutex); return swapchain; }
    VkExtent2D GetExtent() { std::lock_guard lock(swapchain_mutex); return extent; }
    uint32_t GetImagesCount() { std::lock_guard lock(swapchain_mutex); return images_in_swapchain; }
    std::vector<VkImageView> GetImageViews() { std::lock_guard lock(swapchain_mutex); return swapchain_image_views; }

    SwapChainDetails capabilities;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkExtent2D extent;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    std::shared_ptr<Device> device;
    uint32_t images_in_swapchain = 0;
    std::mutex swapchain_mutex;
  };

  class SwapChain
  {
  private:
    std::unique_ptr<SwapChain_impl> impl;
  public:
    SwapChain() = delete;
    SwapChain(const SwapChain &obj) = delete;
    SwapChain(SwapChain &&obj) noexcept : impl(std::move(obj.impl)) {};
    SwapChain(const std::shared_ptr<Device> dev, const VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR) :
      impl(std::unique_ptr<SwapChain_impl>(new SwapChain_impl(dev, mode))) {};
    SwapChain &operator=(const SwapChain &obj) = delete;
    SwapChain &operator=(SwapChain &&obj) noexcept;
    void swap(SwapChain &obj) noexcept;
    bool IsValid() { return impl != nullptr; }
    VkResult ReCreate() { return impl->ReCreate(); }
    VkResult SetPresentMode(const VkPresentModeKHR mode) { return impl->SetPresentMode(mode); }
    VkSurfaceFormatKHR GetSurfaceFormat() { return impl->GetSurfaceFormat(); }
    VkSwapchainKHR GetSwapChain() { return impl->GetSwapChain(); }
    VkExtent2D GetExtent() { return impl->GetExtent(); }
    uint32_t GetImagesCount() { return impl->GetImagesCount(); }
    std::vector<VkImageView> GetImageViews() { return impl->GetImageViews(); }
    ~SwapChain() = default;
  };

  void swap(SwapChain &lhs, SwapChain &rhs) noexcept;
}

#endif