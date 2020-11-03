#ifndef __VULKAN_IMAGEARRAY_H
#define __VULKAN_IMAGEARRAY_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <cmath>
#include <mutex>
#include <cstring>
#include <tuple>

#include "Device.h"
#include "Logger.h"
#include "StorageArray.h"

namespace Vulkan
{
  enum class ImageTiling
  {
    Optimal = VK_IMAGE_TILING_OPTIMAL,
    Linear = VK_IMAGE_TILING_LINEAR
  };

  enum class ImageType
  {
    Storage = VK_IMAGE_USAGE_STORAGE_BIT,
    Sampled = VK_IMAGE_USAGE_SAMPLED_BIT,
    DepthBuffer = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
    Multisampling = (VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
  };

  // enum class ImageFormat
  // {
  //   SRGB_8 = VK_FORMAT_R8G8B8A8_SRGB,
  //   Depth_32 = VK_FORMAT_D32_SFLOAT,
  //   Depth_32_S8 = VK_FORMAT_D32_SFLOAT_S8_UINT,
  //   Depth_24_S8 = VK_FORMAT_D24_UNORM_S8_UINT
  // };

  struct image_t
  {
    VkImage image = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
    ImageType type = ImageType::Storage;
    VkDeviceSize size = 0;
    VkDeviceSize offset = 0;
    uint32_t channels = 4;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageAspectFlags aspect_flags = 0;
    VkImageCreateInfo image_info = {};
    std::string tag = "";
  };

  class ImageConfig
  {
  private:
    friend class ImageArray_impl;
    uint32_t height = 0;
    uint32_t width = 0;
    uint32_t channels = 4;
    bool use_mip_levels = false;
    ImageType type = ImageType::Storage;
    ImageTiling tiling = ImageTiling::Optimal;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
    std::string tag = "";
  public:
    ImageConfig() = default;
    ~ImageConfig() = default;
    auto &SetSize(const uint32_t im_height, const uint32_t im_width, const uint32_t im_channels = 4) 
    { 
      height = im_height; width = im_width; channels = im_channels; return *this; 
    }
    auto &PreallocateMipLevels(const bool val) { use_mip_levels = val; return *this; }
    auto &SetType(const ImageType val) { type = val; return *this; }
    auto &SetTiling(const ImageTiling val) { tiling = val; return *this; }
    auto &SetFormat(const VkFormat val) { format = val; return *this; }
    auto &SetSamplesCount(const VkSampleCountFlagBits val) { sample_count = val; return *this; }
    auto &SetTag(const std::string val) { tag = val; return *this; }
  };

  class ImageArray_impl
  {
  public:
    ImageArray_impl() = delete;
    ImageArray_impl(const ImageArray_impl &obj) = delete;
    ImageArray_impl(ImageArray_impl &&obj) = delete;
    ImageArray_impl &operator=(const ImageArray_impl &obj) = delete;
    ImageArray_impl &operator=(ImageArray_impl &&obj) = delete;
    ~ImageArray_impl();
  private:
    friend class ImageArray;
    std::shared_ptr<Device> device;
    std::vector<image_t> images;
    HostVisibleMemory access = HostVisibleMemory::HostVisible;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    VkDeviceSize align = 256;
    std::vector<ImageConfig> prebuild_config;
    HostVisibleMemory prebuild_access_config = HostVisibleMemory::HostVisible;
    std::mutex images_mutex;
    std::mutex config_mutex;

    VkDeviceSize Align(const VkDeviceSize value, const VkDeviceSize align);
    void Abort(std::vector<image_t> &imgs);

    ImageArray_impl(const std::shared_ptr<Device> dev);
    VkResult StartConfig(const HostVisibleMemory val);
    VkResult AddImage(const ImageConfig &params);
    VkResult EndConfig();
    void Clear();
    HostVisibleMemory GetMemoryAccess() { return access; }
    size_t Count() { std::lock_guard lock(images_mutex); return images.size(); }
    image_t GetInfo(const size_t index) {std::lock_guard lock(images_mutex); return index < images.size() ? images[index] : image_t(); }
    template <typename T>
    VkResult GetImageData(const size_t index, std::vector<T> &result);
    template <typename T>
    VkResult SetImageData(const size_t index, const std::vector<T> &data);
  };

  class ImageArray
  {
  private:
    std::unique_ptr<ImageArray_impl> impl;
  public:
    ImageArray() = delete;
    ImageArray(const ImageArray &obj);
    ImageArray(ImageArray &&obj) noexcept : impl(std::move(obj.impl)) {};
    ImageArray(const std::shared_ptr<Device> dev) : impl(std::unique_ptr<ImageArray_impl>(new ImageArray_impl(dev))) {};
    ImageArray &operator=(const ImageArray &obj);
    ImageArray &operator=(ImageArray &&obj) noexcept;
    void swap(ImageArray &obj) noexcept;
    bool IsValid() { return impl != nullptr; }
    VkResult StartConfig(const HostVisibleMemory val = HostVisibleMemory::HostVisible) { return impl->StartConfig(val); }
    VkResult AddImage(const ImageConfig &params) { return impl->AddImage(params); }
    VkResult EndConfig() { return impl->EndConfig(); }
    HostVisibleMemory GetMemoryAccess() { return impl->GetMemoryAccess(); }
    void Clear() { impl->Clear(); }
    size_t Count() { return impl->Count(); }
    image_t GetInfo(const size_t index) { return impl->GetInfo(index); }
    template <typename T>
    VkResult GetImageData(const size_t index, std::vector<T> &result) { return impl->GetImageData(index, result); }
    template <typename T>
    VkResult SetImageData(const size_t index, const std::vector<T> &data) { return impl->SetImageData(index, data); }
  };

  void swap(ImageArray &lhs, ImageArray &rhs) noexcept;

  template <typename T>
  VkResult ImageArray_impl::GetImageData(const size_t index, std::vector<T> &result)
  {
    std::lock_guard lock(images_mutex);
    if (index >= images.size())
    {
      Logger::EchoError("Index is out of range", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (memory == VK_NULL_HANDLE)
    {
      Logger::EchoError("Memory is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (images[index].image == VK_NULL_HANDLE)
    {
      Logger::EchoError("Buffer is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (access == HostVisibleMemory::HostInvisible)
    {
      Logger::EchoError("Can't get data from HostInvisible memory", __func__);
      return VK_ERROR_UNKNOWN;
    }

    std::vector<T> tmp(std::ceil(images[index].size / sizeof(T)));
    void *payload = nullptr;

    auto er = vkMapMemory(device->GetDevice(), memory, images[index].offset, images[index].size, 0, &payload);

    if (er != VK_SUCCESS && er != VK_ERROR_MEMORY_MAP_FAILED)
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (er == VK_SUCCESS)
    {
      std::memcpy(tmp.data(), payload, images[index].size);
      vkUnmapMemory(device->GetDevice(), memory);
      result.swap(tmp);
    }
    else
    {
      VkDeviceSize offset = images[index].offset;
      for (VkDeviceSize i = 0; i < images[index].size / align; ++i)
      {
        er = vkMapMemory(device->GetDevice(), memory, offset, align, 0, &payload);
        if (er != VK_SUCCESS)
        {
          Logger::EchoError("Can't map memory.", __func__);
          return VK_ERROR_UNKNOWN;
        }

        std::memcpy(((uint8_t *) tmp.data()) + (i * align), payload, align);
        vkUnmapMemory(device->GetDevice(), memory);
        offset += align;
      }
      result.swap(tmp);
    }

    return VK_SUCCESS;
  }

  template <typename T>
  VkResult ImageArray_impl::SetImageData(const size_t index, const std::vector<T> &data)
  {
    std::lock_guard lock(images_mutex);
    if (index >= images.size())
    {
      Logger::EchoError("Index is out of range", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (memory == VK_NULL_HANDLE)
    {
      Logger::EchoError("Memory is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (images[index].image == VK_NULL_HANDLE)
    {
      Logger::EchoError("Buffer is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (data.size() * sizeof(T) > images[index].size)
    {
      Logger::EchoWarning("Data is too big for buffer", __func__);
    }

    if (access == HostVisibleMemory::HostInvisible)
    {
      Logger::EchoError("Can't get data from HostInvisible memory", __func__);
      return VK_ERROR_UNKNOWN;
    }

    void *payload = nullptr;

    auto er = vkMapMemory(device->GetDevice(), memory, images[index].offset, images[index].size, 0, &payload);

    if (er != VK_SUCCESS && er != VK_ERROR_MEMORY_MAP_FAILED)
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (er == VK_SUCCESS)
    {
      std::memcpy(payload, data.data(), std::min(images[index].size, data.size() * sizeof(T)));
      vkUnmapMemory(device->GetDevice(), memory);
    }
    else
    {
      VkDeviceSize offset = images[index].offset;
      for (VkDeviceSize i = 0; i < std::min(images[index].size, data.size() * sizeof(T)) / align; ++i)
      {
        er = vkMapMemory(device->GetDevice(), memory, offset, align, 0, &payload);
        if (er != VK_SUCCESS)
        {
          Logger::EchoError("Can't map memory.", __func__);
          return VK_ERROR_UNKNOWN;
        }

        std::memcpy(payload, ((uint8_t *) data.data()) + (i * align), align);
        vkUnmapMemory(device->GetDevice(), memory);
        offset += align;
      }
    }

    return VK_SUCCESS;
  }
}

#endif