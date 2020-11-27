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
  private:
    friend class ImageArray_impl;
    VkDeviceMemory memory = VK_NULL_HANDLE;
  public:
    VkImage image = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
    ImageType type = ImageType::Storage;
    VkDeviceSize size = 0;
    HostVisibleMemory access = HostVisibleMemory::HostVisible;
    
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
    HostVisibleMemory access = HostVisibleMemory::HostInvisible;
    VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
    std::string tag = "";
  public:
    ImageConfig() = default;
    ~ImageConfig() noexcept = default;
    auto &SetSize(const uint32_t im_height, const uint32_t im_width, const uint32_t im_channels = 4) noexcept 
    { 
      height = im_height; width = im_width; channels = im_channels; return *this; 
    }
    auto &PreallocateMipLevels(const bool val) noexcept { use_mip_levels = val; return *this; }
    auto &SetType(const ImageType val) noexcept { type = val; return *this; }
    auto &SetTiling(const ImageTiling val) noexcept { tiling = val; return *this; }
    auto &SetFormat(const VkFormat val) noexcept { format = val; return *this; }
    auto &SetSamplesCount(const VkSampleCountFlagBits val) noexcept { sample_count = val; return *this; }
    auto &SetTag(const std::string val) { tag = val; return *this; }
    auto &SetMemoryAccess(const HostVisibleMemory val) { access = val; return *this; }
  };

  class ImageArray_impl
  {
  public:
    ImageArray_impl() = delete;
    ImageArray_impl(const ImageArray_impl &obj) = delete;
    ImageArray_impl(ImageArray_impl &&obj) = delete;
    ImageArray_impl &operator=(const ImageArray_impl &obj) = delete;
    ImageArray_impl &operator=(ImageArray_impl &&obj) = delete;
    ~ImageArray_impl() noexcept;
  private:
    friend class ImageArray;
    std::shared_ptr<Device> device;
    std::vector<image_t> images;
    std::vector<ImageConfig> prebuild_config;

    void Abort(std::vector<image_t> &imgs) const noexcept;

    ImageArray_impl(const std::shared_ptr<Device> dev);
    VkResult StartConfig() noexcept;
    VkResult AddImage(const ImageConfig &params);
    VkResult EndConfig();
    void Clear() noexcept;
    size_t Count() const noexcept { return images.size(); }
    image_t GetInfo(const size_t index) const { return index < images.size() ? images[index] : image_t(); }
    std::shared_ptr<Device> GetDevice() const noexcept { return device; }
    VkResult ChangeLayout(const size_t index, VkImageLayout layout) noexcept 
    { 
      if (index >= images.size()) return VK_ERROR_UNKNOWN;

      images[index].layout = layout; 
      return VK_SUCCESS;
    }
    template <typename T>
    VkResult GetImageData(const size_t index, std::vector<T> &result) const;
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
    bool IsValid() const noexcept { return impl.get() && impl->device->IsValid(); }
    VkResult StartConfig() noexcept { if (impl.get()) return impl->StartConfig(); return VK_ERROR_UNKNOWN; }
    VkResult AddImage(const ImageConfig &params) { if (impl.get()) return impl->AddImage(params); return VK_ERROR_UNKNOWN; }
    VkResult EndConfig() { return impl->EndConfig(); return VK_ERROR_UNKNOWN; }
    VkResult ChangeLayout(const size_t index, VkImageLayout layout) noexcept { if (impl.get()) return impl->ChangeLayout(index, layout); return VK_ERROR_UNKNOWN; }
    void Clear() noexcept { impl->Clear(); }
    size_t Count() const noexcept { return impl->Count(); return 0; }
    std::shared_ptr<Device> GetDevice() const noexcept { if (impl.get()) return impl->GetDevice(); return nullptr; }
    image_t GetInfo(const size_t index) { if (impl.get()) return impl->GetInfo(index); return {}; }
    template <typename T>
    VkResult GetImageData(const size_t index, std::vector<T> &result) const { if (impl.get()) return impl->GetImageData(index, result); return VK_ERROR_UNKNOWN; }
    template <typename T>
    VkResult SetImageData(const size_t index, const std::vector<T> &data) { if (impl.get()) return impl->SetImageData(index, data); return VK_ERROR_UNKNOWN; }
  };

  void swap(ImageArray &lhs, ImageArray &rhs) noexcept;

  template <typename T>
  VkResult ImageArray_impl::GetImageData(const size_t index, std::vector<T> &result) const
  {
    if (index >= images.size())
    {
      Logger::EchoError("Index is out of range", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (images[index].memory == VK_NULL_HANDLE)
    {
      Logger::EchoError("Memory is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (images[index].image == VK_NULL_HANDLE)
    {
      Logger::EchoError("Buffer is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (images[index].access == HostVisibleMemory::HostInvisible)
    {
      Logger::EchoError("Can't get data from HostInvisible memory", __func__);
      return VK_ERROR_UNKNOWN;
    }

    std::vector<T> tmp(std::ceil(images[index].size / sizeof(T)));
    void* payload = nullptr;

    auto er = vkMapMemory(device->GetDevice(), images[index].memory, 0, images[index].size, 0, &payload);

    if (er != VK_SUCCESS && er != VK_ERROR_MEMORY_MAP_FAILED)
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (er == VK_SUCCESS)
    {
      std::memcpy(tmp.data(), payload, images[index].size);
      vkUnmapMemory(device->GetDevice(), images[index].memory);
      result.swap(tmp);
    }
    else
    {
      VkDeviceSize offset = 0;
      VkDeviceSize align = device->GetPhysicalDeviceProperties().limits.minMemoryMapAlignment;
      for (VkDeviceSize i = 0; i < images[index].size / align; ++i)
      {
        er = vkMapMemory(device->GetDevice(), images[index].memory, offset, align, 0, &payload);
        if (er != VK_SUCCESS)
        {
          Logger::EchoError("Can't map memory.", __func__);
          return VK_ERROR_UNKNOWN;
        }

        std::memcpy(((uint8_t*)tmp.data()) + (i * align), payload, align);
        vkUnmapMemory(device->GetDevice(), images[index].memory);
        offset += align;
      }
      result.swap(tmp);
    }

    return VK_SUCCESS;
  }

  template <typename T>
  VkResult ImageArray_impl::SetImageData(const size_t index, const std::vector<T> &data)
  {
    if (index >= images.size())
    {
      Logger::EchoError("Index is out of range", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (images[index].memory == VK_NULL_HANDLE)
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

    if (images[index].access == HostVisibleMemory::HostInvisible)
    {
      Logger::EchoError("Can't get data from HostInvisible memory", __func__);
      return VK_ERROR_UNKNOWN;
    }

    void *payload = nullptr;

    auto er = vkMapMemory(device->GetDevice(), images[index].memory, 0, images[index].size, 0, &payload);

    if (er != VK_SUCCESS && er != VK_ERROR_MEMORY_MAP_FAILED)
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (er == VK_SUCCESS)
    {
      std::memcpy(payload, data.data(), std::min(images[index].size, data.size() * sizeof(T)));
      vkUnmapMemory(device->GetDevice(), images[index].memory);
    }
    else
    {
      VkDeviceSize offset = 0;
      VkDeviceSize align = device->GetPhysicalDeviceProperties().limits.minMemoryMapAlignment;
      for (VkDeviceSize i = 0; i < std::min(images[index].size, data.size() * sizeof(T)) / align; ++i)
      {
        er = vkMapMemory(device->GetDevice(), images[index].memory, offset, align, 0, &payload);
        if (er != VK_SUCCESS)
        {
          Logger::EchoError("Can't map memory.", __func__);
          return VK_ERROR_UNKNOWN;
        }

        std::memcpy(payload, ((uint8_t *) data.data()) + (i * align), align);
        vkUnmapMemory(device->GetDevice(), images[index].memory);
        offset += align;
      }
    }

    return VK_SUCCESS;
  }
}

#endif