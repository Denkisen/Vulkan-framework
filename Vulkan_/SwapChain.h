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
    ~SwapChain_impl() noexcept;
  private:
    friend class SwapChain;
    SwapChain_impl(const std::shared_ptr<Device> dev, const VkPresentModeKHR mode) noexcept;
    VkResult Create() noexcept;
    VkSurfaceFormatKHR GetSwapChainFormat() const noexcept;
    VkPresentModeKHR GetSwapChainPresentMode() const noexcept;
    VkExtent2D GetSwapChainExtent() noexcept;
    VkResult CreateImageViews() noexcept;

    VkResult ReCreate() noexcept;
    VkResult SetPresentMode(const VkPresentModeKHR mode) noexcept;
    VkSurfaceFormatKHR GetSurfaceFormat() noexcept { std::lock_guard lock(swapchain_mutex); return format; }
    VkSwapchainKHR GetSwapChain() noexcept { std::lock_guard lock(swapchain_mutex); return swapchain; }
    VkExtent2D GetExtent() noexcept { std::lock_guard lock(swapchain_mutex); return extent; }
    uint32_t GetImagesCount() noexcept { std::lock_guard lock(swapchain_mutex); return images_in_swapchain; }
    std::vector<VkImageView> GetImageViews() noexcept { std::lock_guard lock(swapchain_mutex); return swapchain_image_views; }

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
    SwapChain(const std::shared_ptr<Device> dev, const VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR) noexcept :
      impl(std::unique_ptr<SwapChain_impl>(new SwapChain_impl(dev, mode))) {};
    SwapChain &operator=(const SwapChain &obj) = delete;
    SwapChain &operator=(SwapChain &&obj) noexcept;
    void swap(SwapChain &obj) noexcept;
    bool IsValid() const noexcept { return impl.get() && impl->swapchain != VK_NULL_HANDLE; }
    VkResult ReCreate() noexcept { if (impl.get()) return impl->ReCreate(); return VK_ERROR_UNKNOWN; }
    VkResult SetPresentMode(const VkPresentModeKHR mode) noexcept { if (impl.get()) return impl->SetPresentMode(mode); return VK_ERROR_UNKNOWN; }
    VkSurfaceFormatKHR GetSurfaceFormat() noexcept { if (impl.get()) return impl->GetSurfaceFormat(); return {}; }
    VkSwapchainKHR GetSwapChain() noexcept { if (impl.get()) return impl->GetSwapChain(); return VK_NULL_HANDLE; }
    VkExtent2D GetExtent() noexcept { if (impl.get()) return impl->GetExtent(); return {}; }
    uint32_t GetImagesCount() noexcept { if (impl.get()) return impl->GetImagesCount(); return 0; }
    std::vector<VkImageView> GetImageViews() noexcept { if (impl.get()) return impl->GetImageViews(); return {}; }
    ~SwapChain() noexcept = default;
  };

  void swap(SwapChain &lhs, SwapChain &rhs) noexcept;
}

#endif