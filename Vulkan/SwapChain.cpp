#include "SwapChain.h"

#include <algorithm>
#include <optional>

namespace Vulkan
{
  void SwapChain::Destroy()
  {
    for (auto image_view : swapchain_image_views) 
    {
      if (image_view != VK_NULL_HANDLE)
        vkDestroyImageView(device->GetDevice(), image_view, nullptr);
    }
    swapchain_image_views.clear();
  }

  SwapChain::~SwapChain()
  {
#ifdef DEBUG
    std::cout << __func__ << std::endl;
#endif
    Destroy();
    if (swapchain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR(device->GetDevice(), swapchain, nullptr);
      swapchain = VK_NULL_HANDLE;
    }
  }

  SwapChain::SwapChain(std::shared_ptr<Vulkan::Device> dev)
  {
    if (dev == nullptr)
      throw std::runtime_error("Device pointer is not valid.");
    device = dev;
    Create();
  }

  void SwapChain::Create()
  {
    capabilities = device->GetSwapChainDetails();
    format = GetSwapChainFormat();
    present_mode = GetSwapChainPresentMode();
    extent = GetSwapChainExtent();
    images_in_swapchain = capabilities.capabilities.minImageCount + 5;

    if (capabilities.capabilities.maxImageCount > 0 && images_in_swapchain > capabilities.capabilities.maxImageCount) 
    {
      images_in_swapchain = capabilities.capabilities.maxImageCount;
    }

    auto g_index = device->GetGraphicFamilyQueueIndex();
    auto p_index = device->GetPresentFamilyQueueIndex();

    if (!g_index.has_value() || !p_index.has_value())
      throw std::runtime_error("Graphic or Present queue is not available.");

    VkSwapchainCreateInfoKHR create_info = {};
    create_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    create_info.surface = device->GetSurface();
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
    old_swapchain = swapchain;
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

    if (vkCreateSwapchainKHR(device->GetDevice(), &create_info, nullptr, &swapchain) != VK_SUCCESS) 
      throw std::runtime_error("Failed to create swap chain!");
    
    if (old_swapchain != VK_NULL_HANDLE)
    {
      vkDestroySwapchainKHR(device->GetDevice(), old_swapchain, nullptr);
      old_swapchain = VK_NULL_HANDLE;
    }

    swapchain_images.clear();
    if (vkGetSwapchainImagesKHR(device->GetDevice(), swapchain, &images_in_swapchain, nullptr) != VK_SUCCESS)
      throw std::runtime_error("Failed to get swap chain images!");

    swapchain_images.resize(images_in_swapchain);

    if (vkGetSwapchainImagesKHR(device->GetDevice(), swapchain, &images_in_swapchain, swapchain_images.data()) != VK_SUCCESS)
      throw std::runtime_error("Failed to get swap chain images!");

    CreateImageViews();
  }

  void SwapChain::CreateImageViews()
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

      if (vkCreateImageView(device->GetDevice(), &create_info, nullptr, &swapchain_image_views[i]) != VK_SUCCESS)
        throw std::runtime_error("Failed to create image views!");
    }
  }

  VkSurfaceFormatKHR SwapChain::GetSwapChainFormat()
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

  VkPresentModeKHR SwapChain::GetSwapChainPresentMode()
  {
    if (std::find(capabilities.present_modes.begin(), capabilities.present_modes.end(), VK_PRESENT_MODE_MAILBOX_KHR) != capabilities.present_modes.end())
      return VK_PRESENT_MODE_MAILBOX_KHR;

    return VK_PRESENT_MODE_FIFO_KHR;
  }

  VkExtent2D SwapChain::GetSwapChainExtent()
  {
    if (vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device->GetPhysicalDevice(), device->GetSurface(), &capabilities.capabilities) != VK_SUCCESS)
      throw std::runtime_error("Can't get surface capabilities.");

    size = device->GetWindowSize();

    if (capabilities.capabilities.currentExtent.width != UINT32_MAX) 
    {
      return capabilities.capabilities.currentExtent;
    } 
    else 
    {
      VkExtent2D actual_extent = {size.first, size.second};
      actual_extent.width = std::max(capabilities.capabilities.minImageExtent.width, std::min(capabilities.capabilities.maxImageExtent.width, actual_extent.width));
      actual_extent.height = std::max(capabilities.capabilities.minImageExtent.height, std::min(capabilities.capabilities.maxImageExtent.height, actual_extent.height));

      return actual_extent;
    }
  }

  void SwapChain::ReBuildSwapChain()
  {
    vkDeviceWaitIdle(device->GetDevice());
    Destroy();
    Create();
  }
}