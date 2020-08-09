#include "Image.h"

namespace Vulkan
{
  void Image::Destroy()
  {
    if (device.get() != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      if (image != VK_NULL_HANDLE)
      {
        vkDestroyImage(device->GetDevice(), image, nullptr);
        vkFreeMemory(device->GetDevice(), image_memory, nullptr);
        image = VK_NULL_HANDLE;
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

  Image::Image(std::shared_ptr<Vulkan::Device> dev, const size_t w, const size_t h, const size_t channels, Vulkan::ImageTiling tiling, Vulkan::ImageUsage usage)
  {
    Create(dev, w, h, channels, tiling, usage);
  }

  void Image::Create(std::shared_ptr<Vulkan::Device> dev, const size_t w, const size_t h, const size_t channels, Vulkan::ImageTiling tiling, Vulkan::ImageUsage usage)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
      throw std::runtime_error("Invalid device pointer.");
    if (w == 0 || h == 0 || channels == 0 || channels == 2 || channels > 4)
      throw std::runtime_error("Invalid image size.");
    device = dev;
    width = w;
    height = h;
    this->channels = channels;
    buffer_size = width * height * channels * sizeof(uint8_t);
    this->tiling = tiling;
    this->usage = usage;

    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = (uint32_t) width;
    image_info.extent.height = (uint32_t) height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;

    switch (this->channels)
    {
      case 1:
        image_info.format = VK_FORMAT_R8_SRGB;
        break;
      case 3:
        image_info.format = VK_FORMAT_B8G8R8_SRGB;
        break;
      case 4:
      default:
        image_info.format = VK_FORMAT_R8G8B8A8_SRGB;
    }

    image_info.tiling = (VkImageTiling) tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = (VkImageUsageFlags) usage;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.flags = 0;

    if (vkCreateImage(device->GetDevice(), &image_info, nullptr, &image) != VK_SUCCESS)
      throw std::runtime_error("Failed to create image!");

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    switch (usage)
    {
      case Vulkan::ImageUsage::Transfer_Dst_Sampled:
      case Vulkan::ImageUsage::Transfer_Dst_Storage:
        flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
      case Vulkan::ImageUsage::Transfer_Src:
      case Vulkan::ImageUsage::Storage:
        flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
      default:
        flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

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
  }
}