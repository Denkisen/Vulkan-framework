#ifndef __VULKAN_SWAPCHAIN_H
#define __VULKAN_SWAPCHAIN_H

#include "Logger.h"
#include "Misc.h"
#include "Instance.h"
#include "Device.h"

#include <algorithm>
#include <vulkan/vulkan.h>
#include <memory>

namespace Vulkan
{
  class SwapChainConfig
  {  
  private:
    friend class SwapChain_impl;
    VkPresentModeKHR mode = VK_PRESENT_MODE_FIFO_KHR;
    uint32_t images_count = 2;
  public:
    SwapChainConfig() = default;
    ~SwapChainConfig() noexcept = default;
    auto &SetPresentMode(const VkPresentModeKHR val) noexcept { mode = val; return *this; }
    auto &SetImagesCount(const uint32_t val) noexcept { images_count = val; return *this; }
  };

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
    SwapChainDetails capabilities;
    VkSurfaceFormatKHR format;
    VkPresentModeKHR present_mode = VK_PRESENT_MODE_FIFO_KHR;
    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    VkExtent2D extent;
    std::vector<VkImage> swapchain_images;
    std::vector<VkImageView> swapchain_image_views;
    std::shared_ptr<Device> device;
    uint32_t images_in_swapchain = 0;

    SwapChain_impl(const std::shared_ptr<Device> dev, const SwapChainConfig &params);
    VkResult Create();
    VkSurfaceFormatKHR GetSwapChainFormat() const;
    VkPresentModeKHR GetSwapChainPresentMode() const;
    VkExtent2D GetSwapChainExtent();
    VkResult CreateImageViews();

    VkResult ReCreate();
    VkResult SetPresentMode(const VkPresentModeKHR mode);
    VkSurfaceFormatKHR GetSurfaceFormat() const noexcept { return format; }
    VkSwapchainKHR GetSwapChain() const noexcept { return swapchain; }
    VkExtent2D GetExtent() const noexcept { return extent; }
    uint32_t GetImagesCount() const noexcept { return images_in_swapchain; }
    std::vector<VkImageView> GetImageViews() const { return swapchain_image_views; }
    std::shared_ptr<Device> GetDevice() const noexcept { return device; }
  };

  class SwapChain
  {
  private:
    std::unique_ptr<SwapChain_impl> impl;
  public:
    SwapChain() = delete;
    SwapChain(const SwapChain &obj) = delete;
    SwapChain(SwapChain &&obj) noexcept : impl(std::move(obj.impl)) {};
    SwapChain(const std::shared_ptr<Device> dev, const SwapChainConfig &params)  :
      impl(std::unique_ptr<SwapChain_impl>(new SwapChain_impl(dev, params))) {};
    SwapChain &operator=(const SwapChain &obj) = delete;
    SwapChain &operator=(SwapChain &&obj) noexcept;
    void swap(SwapChain &obj) noexcept;
    bool IsValid() const noexcept { return impl.get() && impl->swapchain != VK_NULL_HANDLE; }
    VkResult ReCreate() { if (impl.get()) return impl->ReCreate(); return VK_ERROR_UNKNOWN; }
    VkResult SetPresentMode(const VkPresentModeKHR mode) { if (impl.get()) return impl->SetPresentMode(mode); return VK_ERROR_UNKNOWN; }
    VkSurfaceFormatKHR GetSurfaceFormat() const noexcept { if (impl.get()) return impl->GetSurfaceFormat(); return {}; }
    VkSwapchainKHR GetSwapChain() const noexcept { if (impl.get()) return impl->GetSwapChain(); return VK_NULL_HANDLE; }
    VkExtent2D GetExtent() const noexcept { if (impl.get()) return impl->GetExtent(); return {}; }
    uint32_t GetImagesCount() const noexcept { if (impl.get()) return impl->GetImagesCount(); return 0; }
    std::vector<VkImageView> GetImageViews() const { if (impl.get()) return impl->GetImageViews(); return {}; }
    std::shared_ptr<Device> GetDevice() const noexcept { if (impl.get()) return impl->GetDevice(); return nullptr; }
    ~SwapChain() noexcept = default;
  };

  void swap(SwapChain &lhs, SwapChain &rhs) noexcept;
}

#endif