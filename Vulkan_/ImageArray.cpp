#include "ImageArray.h"

#include <algorithm>

namespace Vulkan
{
  ImageArray_impl::~ImageArray_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
    Clear();
  }

  void ImageArray_impl::Abort(std::vector<image_t> &imgs) const noexcept
  {
    for (auto &obj : imgs)
    {
      if (obj.image_view != VK_NULL_HANDLE)
        vkDestroyImageView(device->GetDevice(), obj.image_view, nullptr);
      if (obj.image != VK_NULL_HANDLE)
        vkDestroyImage(device->GetDevice(), obj.image, nullptr); 
      if (obj.memory != VK_NULL_HANDLE)
        vkFreeMemory(device->GetDevice(), obj.memory, nullptr);
    }
  }

  void ImageArray_impl::Clear() noexcept
  {
    Abort(images);
    images.clear();
  }

  ImageArray_impl::ImageArray_impl(const std::shared_ptr<Device> dev)
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
  }

  VkResult ImageArray_impl::StartConfig() noexcept
  {
    prebuild_config.clear();

    return VK_SUCCESS;
  }

  VkResult ImageArray_impl::AddImage(const ImageConfig &params)
  {
    if (params.width == 0 || params.height == 0 || params.channels == 0 || params.channels == 2 || params.channels > 4)
    {
      Logger::EchoError("Invalid image size", __func__);
      return VK_ERROR_UNKNOWN;
    }

    prebuild_config.push_back(params);
    
    return VK_SUCCESS;
  }

  VkResult ImageArray_impl::EndConfig()
  {
    if (prebuild_config.empty())
    {
      Logger::EchoWarning("Nothing to build", __func__);
      return VK_SUCCESS;
    }

    std::vector<image_t> tmp_images;
    tmp_images.reserve(prebuild_config.size());

    for (auto& p : prebuild_config)
    {
      p.sample_count = device->CheckSampleCountSupport(p.sample_count) ? p.sample_count : VK_SAMPLE_COUNT_1_BIT;
      image_t tmp = {};
      tmp.channels = p.channels;
      tmp.type = p.type;
      tmp.layout = VK_IMAGE_LAYOUT_UNDEFINED;
      tmp.tag = p.tag;
      tmp.access = p.access;

      tmp.image_info = {};
      tmp.image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      tmp.image_info.imageType = VK_IMAGE_TYPE_2D;
      tmp.image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      tmp.image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      tmp.image_info.flags = 0;
      tmp.image_info.extent.width = p.width;
      tmp.image_info.extent.height = p.height;
      tmp.image_info.extent.depth = 1;
      tmp.image_info.mipLevels = p.use_mip_levels ? (uint32_t)std::floor(std::log2(std::max(p.width, p.height))) + 1 : 1;
      tmp.image_info.arrayLayers = 1;
      tmp.image_info.format = p.format;
      tmp.image_info.tiling = (VkImageTiling)p.tiling;
      tmp.image_info.samples = p.sample_count;
      tmp.image_info.usage = p.type != ImageType::Multisampling ? (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT) | (VkImageUsageFlags)p.type : (VkImageUsageFlags)p.type;

      auto er = vkCreateImage(device->GetDevice(), &tmp.image_info, nullptr, &tmp.image);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Failed to create image", __func__);
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
        Abort(tmp_images);
        return er;
      }

      VkMemoryRequirements mem_req = {};
      vkGetImageMemoryRequirements(device->GetDevice(), tmp.image, &mem_req);
      tmp.size = mem_req.size;

      VkPhysicalDeviceMemoryProperties properties;
      vkGetPhysicalDeviceMemoryProperties(device->GetPhysicalDevice(), &properties);

      std::optional<uint32_t> mem_index;
      for (uint32_t i = 0; i < properties.memoryTypeCount; i++)
      {
        if (mem_req.memoryTypeBits & (1 << i) &&
          (properties.memoryTypes[i].propertyFlags & (VkMemoryPropertyFlags)tmp.access) &&
          (mem_req.size < properties.memoryHeaps[properties.memoryTypes[i].heapIndex].size))
        {
          mem_index = i;
          break;
        }
      }

      if (!mem_index.has_value())
      {
        Logger::EchoError("No memory index", __func__);
        Abort(tmp_images);
        return VK_ERROR_UNKNOWN;
      }

      VkMemoryAllocateInfo memory_allocate_info =
      {
        VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        0,
        mem_req.size,
        mem_index.value() 
      };

      er = vkAllocateMemory(device->GetDevice(), &memory_allocate_info, nullptr, &tmp.memory);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Can't allocate memory", __func__);
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
        Abort(tmp_images);
        return er;
      }

      er = vkBindImageMemory(device->GetDevice(), tmp.image, tmp.memory, 0);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Can't bind memory to buffer.");
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
        Abort(tmp_images);
        return er;
      }

      VkImageViewCreateInfo view_info = {};
      view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      view_info.image = tmp.image;
      view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_info.format = tmp.image_info.format;

      switch (view_info.format)
      {
      case VK_FORMAT_D32_SFLOAT:
        tmp.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
        break;
      case VK_FORMAT_D32_SFLOAT_S8_UINT:
      case VK_FORMAT_D24_UNORM_S8_UINT:
        tmp.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        break;
      default:
        tmp.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
      }

      view_info.subresourceRange.aspectMask = tmp.aspect_flags;
      view_info.subresourceRange.baseMipLevel = 0;
      view_info.subresourceRange.levelCount = tmp.image_info.mipLevels;
      view_info.subresourceRange.baseArrayLayer = 0;
      view_info.subresourceRange.layerCount = 1;
      view_info.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_info.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_info.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
      view_info.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

      er = vkCreateImageView(device->GetDevice(), &view_info, nullptr, &tmp.image_view);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Failed to create texture image view", __func__);
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
        Abort(tmp_images);
        return er;
      }

      tmp_images.push_back(tmp);
    }

    if (tmp_images.empty())
    {
      Logger::EchoWarning("Nothing to build", __func__);
      return VK_SUCCESS;
    }

    images.swap(tmp_images);
    Abort(tmp_images);

    return VK_SUCCESS;
  }

  ImageArray &ImageArray::operator=(ImageArray &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void ImageArray::swap(ImageArray &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(ImageArray &lhs, ImageArray &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }
}