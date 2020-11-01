#ifndef __VULKAN_SAMPLER_H
#define __VULKAN_SAMPLER_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

#include "Device.h"

namespace Vulkan
{
  class SamplerConfig
  {
  private:
    friend class Sampler_impl;
    bool enable_anisotropy = false;
    float max_anisotropy = 16.0;
    VkSamplerMipmapMode mipmap_mode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    VkFilter min_filter = VK_FILTER_LINEAR;
    VkFilter mag_filter = VK_FILTER_LINEAR;
    float mip_lod_bias = 0.0; 
    float lod_min = 0.0; 
    float lod_max = 0.0; 
  public:
    SamplerConfig() = default;
    ~SamplerConfig() = default;
    auto &SetLODMax(const float val) { lod_max = val; return *this; }
    auto &SetLODMin(const float val) { lod_min = val; return *this; }
    auto &SetLODBias(const float val) { mip_lod_bias = val; return *this; }
    auto &UseAnisotropy(const bool val) { enable_anisotropy = val; return *this; }
    auto &SetMinificationFilter(const VkFilter val) { min_filter = val; return *this; }
    auto &SetMagnificationFilter(const VkFilter val) { mag_filter = val; return *this; }
    auto &SetAnisotropyLevel(const float val) { max_anisotropy = val; return *this; }
    auto &SetMipmapMode(const VkSamplerMipmapMode val) { mipmap_mode = val; return *this; }
  };

  class Sampler_impl
  {
  public:
    ~Sampler_impl();
    Sampler_impl() = delete;
    Sampler_impl(const Sampler_impl &obj) = delete;
    Sampler_impl(Sampler_impl &&obj) = delete;
    Sampler_impl &operator=(const Sampler_impl &obj) = delete;
    Sampler_impl &operator=(Sampler_impl &&obj) = delete;
  private:
    friend class Sampler;
    Sampler_impl(const std::shared_ptr<Device> dev, const SamplerConfig &params);
    VkSampler GetSampler() { return sampler; }
    VkFilter GetMinificationFilter() { return conf.min_filter; }
    VkFilter GetMagnificationFilter() { return conf.mag_filter; }

    VkSampler sampler = VK_NULL_HANDLE;
    std::shared_ptr<Vulkan::Device> device;
    SamplerConfig conf;
  };

  class Sampler
  {
  private:
    std::unique_ptr<Sampler_impl> impl;
  public:
    Sampler() = delete;
    Sampler(const Sampler &obj) = delete;
    Sampler &operator=(const Sampler &obj) = delete;
    ~Sampler() = default;
    Sampler(Sampler &&obj) noexcept : impl(std::move(obj.impl)) {};
    Sampler(const std::shared_ptr<Device> dev, const SamplerConfig &params) : impl(std::unique_ptr<Sampler_impl>(new Sampler_impl(dev, params))) {};
    Sampler &operator=(Sampler &&obj) noexcept;
    void swap(Sampler &obj) noexcept;
    bool IsValid() { return impl != nullptr; }
    VkSampler GetSampler() { return impl->GetSampler(); }
    VkFilter GetMinificationFilter() { return impl->GetMinificationFilter(); }
    VkFilter GetMagnificationFilter() { return impl->GetMagnificationFilter(); }
  };

  void swap(Sampler &lhs, Sampler &rhs) noexcept;
}

#endif