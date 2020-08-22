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
    Sampled = VK_IMAGE_USAGE_SAMPLED_BIT,
    DepthBuffer = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
  };

  enum class ImageFormat
  {
    SRGB_8 = VK_FORMAT_R8G8B8A8_SRGB,
    Depth_32 = VK_FORMAT_D32_SFLOAT,
    Depth_32_S8 = VK_FORMAT_D32_SFLOAT_S8_UINT,
    Depth_24_S8 = VK_FORMAT_D24_UNORM_S8_UINT
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
    uint32_t mip_levels = 1;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    VkImageLayout layout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkImageAspectFlags aspect_flags = 0;
    Vulkan::ImageTiling tiling;
    Vulkan::HostVisibleMemory access;
    Vulkan::ImageType type;
    void Create(std::shared_ptr<Vulkan::Device> dev, 
                const size_t w, const size_t h, 
                const bool enable_mip_mapping,
                const Vulkan::ImageTiling tiling, const Vulkan::HostVisibleMemory access, 
                const Vulkan::ImageType type, const Vulkan::ImageFormat format);
    void Destroy();
  public:
    Image() = delete;
    Image(const Image &obj) = delete;
    explicit Image(std::shared_ptr<Vulkan::Device> dev, const size_t w, const size_t h,
                  const bool enable_mip_mapping = true,
                  const Vulkan::ImageTiling tiling = Vulkan::ImageTiling::Optimal, 
                  const Vulkan::HostVisibleMemory access = Vulkan::HostVisibleMemory::HostVisible,
                  const Vulkan::ImageType type = Vulkan::ImageType::Sampled,
                  const Vulkan::ImageFormat format = Vulkan::ImageFormat::SRGB_8);
    Image& operator= (const Image &obj) = delete;
    size_t Width() const { return width; }
    size_t Height() const { return height; }
    size_t Channels() const { return channels; }
    Vulkan::HostVisibleMemory MemoryAccess() const { return access; }
    Vulkan::ImageTiling Tiling() const { return tiling; }
    Vulkan::ImageType Type() const { return type; }
    VkImage GetImage() const { return image; }
    VkFormat GetFormat() const { return format; }
    VkImageAspectFlags GetImageAspectFlags() const { return aspect_flags; }
    VkImageLayout GetLayout() const { return layout; }
    VkImageView GetImageView() const { return image_view; }
    uint32_t GetMipLevels() const { return mip_levels; }
    void SetLayout(const VkImageLayout l) { layout = l; }

    ~Image();
  };
}
#endif