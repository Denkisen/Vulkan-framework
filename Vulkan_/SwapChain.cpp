#include "SwapChain.h"
#include "Logger.h"
#include "Instance.h"

#include <algorithm>

namespace Vulkan
{
  SwapChain_impl::~SwapChain_impl()
  {
    Logger::EchoDebug("", __func__);

    for (auto image_view : swapchain_image_views) 
    {
      if (image_view != VK_NULL_HANDLE)
        vkDestroyImageView(device->GetDevice(), image_view, nullptr);
    }

    if (swapchain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR(device->GetDevice(), swapchain, nullptr);
      swapchain = VK_NULL_HANDLE;
    }
  }

  SwapChain_impl::SwapChain_impl(const std::shared_ptr<Device> dev, const VkPresentModeKHR mode)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
    present_mode = mode;
    auto er = Create();
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't create swapchain", __func__);
      return;
    }
  }

  VkResult SwapChain_impl::Create()
  {
    capabilities = Misc::GetSwapChainDetails(device->GetPhysicalDevice(), device->GetSurface()->GetSurface());
    format = GetSwapChainFormat();
    present_mode = GetSwapChainPresentMode();
    extent = GetSwapChainExtent();
    images_in_swapchain = capabilities.capabilities.minImageCount + 2;

    if (capabilities.capabilities.maxImageCount > 0 && images_in_swapchain > capabilities.capabilities.maxImageCount) 
    {
      images_in_swapchain = capabilities.capabilities.maxImageCount;
    }

    auto g_index = device->GetGraphicFamilyQueueIndex();
    auto p_index = device->GetPresentFamilyQueueIndex();

    if (!g_index.has_value() || !p_index.has_value())
    {
      Logger::EchoError("Graphic or Present queue is not available", __func__);
      return VK_ERROR_UNKNOWN;
    }

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = device->GetSurface()->GetSurface();
    create_info.minImageCount = images_in_swapchain;
    create_info.imageFormat = format.format;
    create_info.imageColorSpace = format.colorSpace;
    create_info.imageExtent = extent;
    create_info.imageArrayLayers = 1;
    create_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    create_info.preTransform = capabilities.capabilities.currentTransform;
    create_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    create_info.presentMode = present_mode;
    create_info.clipped = VK_TRUE;
    
    VkSwapchainKHR old_swapchain = swapchain;
    create_info.oldSwapchain = old_swapchain;

    uint32_t queue_family_indices[] = 
    {
      g_index.value(), 
      p_index.value()
    };

    if (g_index.value() != p_index.value()) 
    {
      create_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      create_info.queueFamilyIndexCount = 2;
      create_info.pQueueFamilyIndices = queue_family_indices;
    } 
    else 
    {
      create_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
      create_info.queueFamilyIndexCount = 0;
      create_info.pQueueFamilyIndices = nullptr;
    }

    auto er = vkCreateSwapchainKHR(device->GetDevice(), &create_info, nullptr, &swapchain);
    if (er != VK_SUCCESS) 
    {
      Logger::EchoError("Failed to create swap chain", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      swapchain = old_swapchain;
      return er;
    }
    
    if (old_swapchain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR(device->GetDevice(), old_swapchain, nullptr);
      old_swapchain = VK_NULL_HANDLE;
    }

    er = vkGetSwapchainImagesKHR(device->GetDevice(), swapchain, &images_in_swapchain, nullptr);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Failed to get swap chain images", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return er;
    }

    swapchain_images.resize(images_in_swapchain);
    er = vkGetSwapchainImagesKHR(device->GetDevice(), swapchain, &images_in_swapchain, swapchain_images.data());
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Failed to get swap chain images", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return er;
    }

    return CreateImageViews();
  }

  VkResult SwapChain_impl::CreateImageViews()
  {
    for (auto image_view : swapchain_image_views) 
    {
      if (image_view != VK_NULL_HANDLE)
        vkDestroyImageView(device->GetDevice(), image_view, nullptr);
    }

    swapchain_image_views.resize(swapchain_images.size());
    
    for (size_t i = 0; i < swapchain_image_views.size(); ++i)
    {
      VkImageViewCreateInfo create_info = {};
      create_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      create_info.image = swapchain_images[i];
      create_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      create_info.format = format.format;
      create_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
      create_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      create_info.subresourceRange.baseMipLevel = 0;
      create_info.subresourceRange.levelCount = 1;
      create_info.subresourceRange.baseArrayLayer = 0;
      create_info.subresourceRange.layerCount = 1;
      auto er = vkCreateImageView(device->GetDevice(), &create_info, nullptr, &swapchain_image_views[i]);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Failed to create image views", __func__);
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
        return VK_ERROR_UNKNOWN;
      }
    }

    return VK_SUCCESS;
  }

  VkSurfaceFormatKHR SwapChain_impl::GetSwapChainFormat()
  {
    auto format = std::find_if(capabilities.formats.begin(), capabilities.formats.end(), [](VkSurfaceFormatKHR &x) {
      return x.format == VK_FORMAT_B8G8R8A8_SRGB && x.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    });

    if (format != capabilities.formats.end())
    {
      return (*format);
    }

    return capabilities.formats[0];
  }

  VkPresentModeKHR SwapChain_impl::GetSwapChainPresentMode()
  {
    if (std::find(capabilities.present_modes.begin(), capabilities.present_modes.end(), present_mode) != capabilities.present_modes.end())
      return present_mode;

    Logger::EchoWarning("Using default presentation mode", __func__);
    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D SwapChain_impl::GetSwapChainExtent()
  {
    auto er = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice(), device->GetSurface()->GetSurface(), &capabilities.capabilities);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't get surface capabilities", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return {};
    }

    auto size = device->GetSurface()->GetFramebufferSize();

    if (capabilities.capabilities.currentExtent.width != UINT32_MAX) 
    {
      return capabilities.capabilities.currentExtent;
    } 
    else 
    {
      VkExtent2D actual_extent = 
      {
        size.first >= 0 ? (uint32_t) size.first : 0,
        size.second >= 0 ? (uint32_t) size.second : 0
      };
      actual_extent.width = std::max(capabilities.capabilities.minImageExtent.width, std::min(capabilities.capabilities.maxImageExtent.width, actual_extent.width));
      actual_extent.height = std::max(capabilities.capabilities.minImageExtent.height, std::min(capabilities.capabilities.maxImageExtent.height, actual_extent.height));

      return actual_extent;
    }
  }

  VkResult SwapChain_impl::ReCreate()
  {
    vkDeviceWaitIdle(device->GetDevice());
    std::lock_guard lock(swapchain_mutex);
    return Create();
  }

  VkResult SwapChain_impl::SetPresentMode(const VkPresentModeKHR mode)
  {
    std::lock_guard lock(swapchain_mutex);
    present_mode = mode;
    present_mode = GetSwapChainPresentMode();
    return VK_SUCCESS;
  }

  SwapChain &SwapChain::operator=(SwapChain &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void SwapChain::swap(SwapChain &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(SwapChain &lhs, SwapChain &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }
}