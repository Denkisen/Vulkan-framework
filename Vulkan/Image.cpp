#include "Image.h"

#include <cmath>

namespace Vulkan
{
  void Image::Destroy()
  {
    if (device.get() != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      if (image != VK_NULL_HANDLE)
      {
        vkDestroyImageView(device->GetDevice(), image_view, nullptr);
        vkDestroyImage(device->GetDevice(), image, nullptr);
        vkFreeMemory(device->GetDevice(), image_memory, nullptr);
        image = VK_NULL_HANDLE;
        image_view = VK_NULL_HANDLE;
        image_memory = VK_NULL_HANDLE;
      }
      device.reset();
    }
  }

  Image::~Image()
  {
#ifdef DEBUG
    std::cout << __func__ << std::endl;
#endif
    Destroy();
  }

  Image::Image(std::shared_ptr<Vulkan::Device> dev, const size_t w, const size_t h, Vulkan::ImageTiling tiling, Vulkan::HostVisibleMemory access, Vulkan::ImageType type)
  {
    Create(dev, w, h, tiling, access, type);
  }

  void Image::Create(std::shared_ptr<Vulkan::Device> dev, const size_t w, const size_t h, Vulkan::ImageTiling tiling, Vulkan::HostVisibleMemory access, Vulkan::ImageType type)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
      throw std::runtime_error("Invalid device pointer.");
    if (w == 0 || h == 0 || channels == 0 || channels == 2 || channels > 4)
      throw std::runtime_error("Invalid image size.");

    device = dev;
    width = w;
    height = h;
    buffer_size = width * height * channels * sizeof(uint8_t);
    buffer_size = (std::ceil(buffer_size / 256.0) * 256);
    this->tiling = tiling;
    this->access = access;
    this->type = type;

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = (uint32_t) width;
    image_info.extent.height = (uint32_t) height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;

    image_info.tiling = (VkImageTiling) tiling;
    image_info.initialLayout = layout;
    image_info.usage = (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT) | (VkImageUsageFlags) type;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = 0;

    if (vkCreateImage(device->GetDevice(), &image_info, nullptr, &image) != VK_SUCCESS)
      throw std::runtime_error("Failed to create image!");

    VkMemoryPropertyFlags flags = (VkMemoryPropertyFlags) access;

    auto memory_type_index = Vulkan::Supply::GetMemoryTypeIndex(device->GetDevice(), device->GetPhysicalDevice(), image, buffer_size, flags);
    if (!memory_type_index.has_value())
      throw std::runtime_error("Out of memory.");
    
    VkMemoryAllocateInfo memory_allocate_info = 
    {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      0,
      buffer_size,
      (uint32_t) memory_type_index.value()
    };

    if (vkAllocateMemory(device->GetDevice(), &memory_allocate_info, nullptr, &image_memory) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate memory");

    if (vkBindImageMemory(dev->GetDevice(), image, image_memory, 0) != VK_SUCCESS)
      throw std::runtime_error("Can't bind memory to image.");

    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;
    view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
    view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

    if (vkCreateImageView(device->GetDevice(), &view_info, nullptr, &image_view) != VK_SUCCESS)
      throw std::runtime_error("Failed to create texture image view!");
  }
}