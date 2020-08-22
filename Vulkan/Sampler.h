#ifndef __CPU_NW_VULKAN_SAMPLER_H
#define __CPU_NW_VULKAN_SAMPLER_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>
#include <vector>

#include "Device.h"

namespace Vulkan
{
  class Sampler
  {
  private:
    VkSampler sampler = VK_NULL_HANDLE;
    std::shared_ptr<Vulkan::Device> device;
    bool enable_anisotropy = true;
    float max_anisotropy = 16.0;
    VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkFilter min_filter = VK_FILTER_LINEAR;
    float mip_lod_bias = 0.0; 
    float lod_min = 0.0; 
    float lod_max = 0.0; 

    void Create(std::shared_ptr<Vulkan::Device> dev, const uint32_t lod_levels);
  public:
    Sampler() = delete;
    Sampler(const Sampler &obj) = delete;
    Sampler(std::shared_ptr<Vulkan::Device> dev, const uint32_t lod_levels);
    Sampler& operator= (const Sampler &obj) = delete;
    VkSampler GetSampler() const { return sampler; }
    VkFilter GetMinificationFilter() const { return min_filter; }
    ~Sampler();
  };
}

#endif