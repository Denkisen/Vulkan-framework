#ifndef __CPU_NW_VULKAN_IMAGE_H
#define __CPU_NW_VULKAN_IMAGE_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>
#include <vector>

#include "Device.h"
#include "Supply.h"

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
    Sampled = VK_IMAGE_USAGE_SAMPLED_BIT
  };

  class Image
  {
  private:
    std::shared_ptr<Vulkan::Device> device;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory image_memory = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
    size_t height = 0;
    size_t width = 0;
    size_t channels = 4;
    uint32_t buffer_size = 0;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    Vulkan::ImageTiling tiling;
    Vulkan::HostVisibleMemory access;
    Vulkan::ImageType type;
    void Create(std::shared_ptr<Vulkan::Device> dev, const size_t w, const size_t h, Vulkan::ImageTiling tiling, Vulkan::HostVisibleMemory access, Vulkan::ImageType type);
    void Destroy();
  public:
    Image() = delete;
    Image(const Image &obj) = delete;
    explicit Image(std::shared_ptr<Vulkan::Device> dev, const size_t w, const size_t h,
                  Vulkan::ImageTiling tiling = Vulkan::ImageTiling::Optimal, 
                  Vulkan::HostVisibleMemory access = Vulkan::HostVisibleMemory::HostVisible,
                  Vulkan::ImageType type = Vulkan::ImageType::Sampled);
    Image& operator= (const Image &obj) = delete;
    size_t Width() const { return width; }
    size_t Height() const { return height; }
    size_t Channels() const { return channels; }
    Vulkan::HostVisibleMemory MemoryAccess() const { return access; }
    Vulkan::ImageTiling Tiling() const { return tiling; }
    Vulkan::ImageType Type() const { return type; }
    VkImage GetImage() const { return image; }
    VkFormat GetFormat() const { return format; }
    VkImageLayout GetLayout() const { return layout; }
    VkImageView GetImageView() const { return image_view; }
    void SetLayout(const VkImageLayout l) { layout = l; }

    ~Image();
  };
}
#endif