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
  enum class ImageUsage
  {
    Transfer_Dst_Sampled = (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT),
    Transfer_Dst_Storage = (VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT),
    Transfer_Src = VK_IMAGE_USAGE_TRANSFER_SRC_BIT,
    Storage = VK_IMAGE_USAGE_STORAGE_BIT
  };

  class Image
  {
  private:
    std::shared_ptr<Vulkan::Device> device;
    VkImage image = VK_NULL_HANDLE;
    VkDeviceMemory image_memory = VK_NULL_HANDLE;
    size_t height = 0;
    size_t width = 0;
    size_t channels = 4;
    uint32_t buffer_size = 0;
    Vulkan::ImageTiling tiling;
    Vulkan::ImageUsage usage;
    void Create(std::shared_ptr<Vulkan::Device> dev, const size_t w, const size_t h, const size_t channels, Vulkan::ImageTiling tiling, Vulkan::ImageUsage usage);
    void Destroy();
  public:
    Image() = delete;
    Image(const Image &obj) = delete;
    explicit Image(std::shared_ptr<Vulkan::Device> dev, const size_t w, const size_t h, 
                  const size_t channels = 4, 
                  Vulkan::ImageTiling tiling = Vulkan::ImageTiling::Optimal, 
                  Vulkan::ImageUsage usage = Vulkan::ImageUsage::Transfer_Dst_Sampled);
    Image& operator= (const Image &obj) = delete;
    size_t Width() const { return width; }
    size_t Height() const { return height; }
    size_t Channels() const { return channels; }
    Vulkan::ImageUsage Usage() const { return usage; }
    Vulkan::ImageTiling Tiling() const { return tiling; }
    VkImage GetImage() const { return image; }

    ~Image();
  };
}
#endif