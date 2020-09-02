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

  Image::Image(std::shared_ptr<Vulkan::Device> dev, const size_t w, const size_t h,
              const bool enable_mip_mapping,
              const Vulkan::ImageTiling tiling, const Vulkan::HostVisibleMemory access, 
              const Vulkan::ImageType type, const Vulkan::ImageFormat format,
              const VkSampleCountFlagBits multisampling)
  {
    Create(dev, w, h, enable_mip_mapping, tiling, access, type, (VkFormat) format, multisampling);
  }

  Image::Image(std::shared_ptr<Vulkan::Device> dev, const size_t w, const size_t h,
              const bool enable_mip_mapping, const Vulkan::ImageTiling tiling, 
              const Vulkan::HostVisibleMemory access, const Vulkan::ImageType type,
              const VkFormat format, const VkSampleCountFlagBits multisampling)
  {
    Create(dev, w, h, enable_mip_mapping, tiling, access, type, format, multisampling);
  }

  void Image::Create(std::shared_ptr<Vulkan::Device> dev, const size_t w, const size_t h, 
                    const bool enable_mip_mapping,
                    const Vulkan::ImageTiling tiling, const Vulkan::HostVisibleMemory access, 
                    const Vulkan::ImageType type, const VkFormat format,
                    const VkSampleCountFlagBits multisampling)
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
    this->format = format;
    this->tiling = tiling;
    this->access = access;
    this->type = type;

    if (enable_mip_mapping)
      mip_levels = (uint32_t) std::floor(std::log2(std::max(width, height))) + 1;
    else
      mip_levels = 1;

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = (uint32_t) width;
    image_info.extent.height = (uint32_t) height;
    image_info.extent.depth = 1;
    image_info.mipLevels = mip_levels;
    image_info.arrayLayers = 1;
    image_info.format = this->format;

    image_info.tiling = (VkImageTiling) tiling;
    image_info.initialLayout = layout;
    image_info.usage = type != ImageType::Multisampling ? (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT) | (VkImageUsageFlags) type : (VkImageUsageFlags) type;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = dev->CheckMultisampling(multisampling) ? multisampling : VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = 0;

    if (vkCreateImage(device->GetDevice(), &image_info, nullptr, &image) != VK_SUCCESS)
      throw std::runtime_error("Failed to create image!");

    VkMemoryPropertyFlags flags = (VkMemoryPropertyFlags) access;

    std::pair<uint32_t, uint32_t> sz = std::make_pair(buffer_size, 0);
    auto memory_type_index = Vulkan::Supply::GetMemoryTypeIndex(device->GetDevice(), device->GetPhysicalDevice(), image, sz, flags);

    if (!memory_type_index.has_value())
      throw std::runtime_error("Out of memory.");
    
    buffer_size = sz.first;
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
    view_info.format = this->format;
    switch ((Vulkan::ImageFormat) format)
    {
      case Vulkan::ImageFormat::Depth_32:
        aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
      case Vulkan::ImageFormat::Depth_32_S8:
      case Vulkan::ImageFormat::Depth_24_S8:
        aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
      case Vulkan::ImageFormat::SRGB_8:
        aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
        break;
      default:
        aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = mip_levels;
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