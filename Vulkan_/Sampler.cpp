#include "Sampler.h"
#include "Logger.h"

#include <algorithm>

namespace Vulkan
{
  Sampler_impl::~Sampler_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
  }

  Sampler_impl::Sampler_impl(const std::shared_ptr<Device> dev, const SamplerConfig &params) noexcept
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
    conf = params;
    conf.max_anisotropy = std::max(0.0f, std::min(conf.max_anisotropy, device->GetPhysicalDeviceProperties().limits.maxSamplerAnisotropy));
    conf.mip_lod_bias = std::max(0.0f, std::min(conf.mip_lod_bias, device->GetPhysicalDeviceProperties().limits.maxSamplerLodBias));
    conf.lod_min = std::max(0.0f, conf.lod_min);
    conf.lod_max = std::max(0.0f, conf.lod_max);

    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = params.mag_filter;
    sampler_info.minFilter = params.min_filter;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = (VkBool32) conf.enable_anisotropy;
    sampler_info.maxAnisotropy = conf.max_anisotropy;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = conf.mipmap_mode;
    sampler_info.mipLodBias = conf.mip_lod_bias;
    sampler_info.minLod = conf.lod_min;
    sampler_info.maxLod = conf.lod_max;

    auto er = vkCreateSampler(device->GetDevice(), &sampler_info, nullptr, &sampler);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Failed to create texture sampler");
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }
  }

  Sampler &Sampler::operator=(Sampler &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void Sampler::swap(Sampler &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(Sampler &lhs, Sampler &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }
}