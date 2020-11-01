#include "ImageArray.h"

#include <algorithm>
#include <atomic>

namespace Vulkan
{
  ImageArray_impl::~ImageArray_impl()
  {
    Logger::EchoDebug("", __func__);
    Clear();
  }

  void ImageArray_impl::Abort(std::vector<image_t> &imgs)
  {
    std::for_each(imgs.begin(), imgs.end(), [&](auto &obj)
    {
      if (obj.image_view != VK_NULL_HANDLE)
        vkDestroyImageView(device->GetDevice(), obj.image_view, nullptr);
      if (obj.image != VK_NULL_HANDLE)
        vkDestroyImage(device->GetDevice(), obj.image, nullptr);
    });
  }

  void ImageArray_impl::Clear()
  {
    std::lock_guard lock(images_mutex);
    Abort(images);
    if (memory != VK_NULL_HANDLE)
      vkFreeMemory(device->GetDevice(), memory, nullptr);

    images.clear();
  }

  ImageArray_impl::ImageArray_impl(const std::shared_ptr<Device> dev)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
    align = device->GetPhysicalDeviceProperties().limits.minMemoryMapAlignment;
  }

  VkDeviceSize ImageArray_impl::Align(const VkDeviceSize value, const VkDeviceSize align)
  {
    return (std::ceil(value / (float) align) * align); 
  }

  VkResult ImageArray_impl::StartConfig(const HostVisibleMemory val)
  {
    std::lock_guard lock(config_mutex);
    prebuild_access_config = val;
    prebuild_config.clear();

    return VK_SUCCESS;
  }

  VkResult ImageArray_impl::AddBuffer(const ImageConfig &params)
  {
    std::lock_guard lock(config_mutex);
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
    std::lock_guard lock(config_mutex);
    if (prebuild_config.empty())
    {
      Logger::EchoWarning("Nothing to build", __func__);
      return VK_SUCCESS;
    }

    std::vector<image_t> tmp_images;
    tmp_images.reserve(prebuild_config.size());
    VkDeviceSize mem_size = 0;
    for (auto &p : prebuild_config)
    {
      image_t tmp = {};
      tmp.channels = p.channels;
      tmp.type = p.type;
      tmp.layout = VK_IMAGE_LAYOUT_UNDEFINED;
      tmp.size = p.width * p.height * p.channels * sizeof(uint8_t);
      tmp.size = Align(tmp.size, align);
      tmp.offset = mem_size;

      tmp.image_info = {};
      tmp.image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
      tmp.image_info.imageType = VK_IMAGE_TYPE_2D;
      tmp.image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
      tmp.image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
      tmp.image_info.flags = 0;
      tmp.image_info.extent.width = p.width;
      tmp.image_info.extent.height = p.height;
      tmp.image_info.extent.depth = 1;
      tmp.image_info.mipLevels = p.use_mip_levels ? (uint32_t) std::floor(std::log2(std::max(p.width, p.height))) + 1 : 1;
      tmp.image_info.arrayLayers = 1;
      tmp.image_info.format = (VkFormat) p.format;
      tmp.image_info.tiling = (VkImageTiling) p.tiling;
      tmp.image_info.samples = device->CheckSampleCountSupport(p.sample_count) ? p.sample_count : VK_SAMPLE_COUNT_1_BIT;
      tmp.image_info.usage = p.type != ImageType::Multisampling ? (VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT) | (VkImageUsageFlags) p.type : (VkImageUsageFlags) p.type;

      auto er = vkCreateImage(device->GetDevice(), &tmp.image_info, nullptr, &tmp.image);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Failed to create image", __func__);
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
        Abort(tmp_images);
        return er;
      }

      VkImageViewCreateInfo view_info = {};
      view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
      view_info.image = tmp.image;
      view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
      view_info.format = tmp.image_info.format;

      switch ((Vulkan::ImageFormat) view_info.format)
      {
        case Vulkan::ImageFormat::Depth_32:
          tmp.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT;
          break;
        case Vulkan::ImageFormat::Depth_32_S8:
        case Vulkan::ImageFormat::Depth_24_S8:
          tmp.aspect_flags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
          break;
        case Vulkan::ImageFormat::SRGB_8:
          tmp.aspect_flags = VK_IMAGE_ASPECT_COLOR_BIT;
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

      mem_size += tmp.size;
      tmp_images.push_back(tmp);
    }

    tmp_images.shrink_to_fit();
    if (tmp_images.empty())
    {
      Logger::EchoWarning("Nothing to build", __func__);
      return VK_SUCCESS;
    }

    std::lock_guard lock1(images_mutex);
    Abort(images);

    if (memory != VK_NULL_HANDLE)
      vkFreeMemory(device->GetDevice(), memory, nullptr);

    std::atomic<VkDeviceSize> req_mem_size = 0;
    std::atomic<VkMemoryRequirements> mem_req = {};

    std::for_each(tmp_images.begin(), tmp_images.end(), [&](auto &obj)
    {
      VkMemoryRequirements mem_req_tmp = {};
      vkGetImageMemoryRequirements(device->GetDevice(), obj.image, &mem_req_tmp);
      mem_req = mem_req_tmp;
      req_mem_size += mem_req.load().size;
    });

    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(device->GetPhysicalDevice(), &properties);

    std::optional<uint32_t> mem_index;
    for (uint32_t i = 0; i < properties.memoryTypeCount; i++) 
    {
      if (mem_req.load().memoryTypeBits & (1 << i) && 
          (properties.memoryTypes[i].propertyFlags & (VkMemoryPropertyFlags) prebuild_access_config) &&
          (req_mem_size < properties.memoryHeaps[properties.memoryTypes[i].heapIndex].size))
      {
        mem_index = i;
        break;
      }
    }

    if (!mem_index.has_value() || req_mem_size != mem_size)
    {
      Logger::EchoError("No memory index", __func__);
      Abort(tmp_images);
      return VK_ERROR_UNKNOWN;
    }

    VkMemoryAllocateInfo memory_allocate_info = 
    {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      0,
      req_mem_size,
      mem_index.value()
    };

    auto er = vkAllocateMemory(device->GetDevice(), &memory_allocate_info, nullptr, &memory);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't allocate memory", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      Abort(tmp_images);
      return er;
    }

    std::atomic<bool> fail = false;
    std::for_each(tmp_images.begin(), tmp_images.end(), [&](auto &obj)
    {
      auto er = vkBindImageMemory(device->GetDevice(), obj.image, memory, obj.offset);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Can't bind memory to buffer.");
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
        fail = true;
      }
    });

    if (fail)
    {
      Abort(tmp_images);
      if (memory != VK_NULL_HANDLE)
        vkFreeMemory(device->GetDevice(), memory, nullptr);
    }

    images.swap(tmp_images);
    access = prebuild_access_config;
    size = mem_size;

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