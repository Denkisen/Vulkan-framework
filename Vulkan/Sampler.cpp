#include "Sampler.h"

namespace Vulkan
{
  Sampler::~Sampler()
  {
#ifdef DEBUG
    std::cout << __func__ << std::endl;
#endif
    if (device.get() != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      if (sampler != VK_NULL_HANDLE)
      {
        vkDestroySampler(device->GetDevice(), sampler, nullptr);
        sampler = nullptr;
      }
    }
    device.reset();
  }

  Sampler::Sampler(std::shared_ptr<Vulkan::Device> dev)
  {
    Create(dev);
  }

  void Sampler::Create(std::shared_ptr<Vulkan::Device> dev)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
      throw std::runtime_error("Device is nullptr.");
    
    device = dev;
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = (VkBool32) enable_anisotropy;
    sampler_info.maxAnisotropy = max_anisotropy;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = mipmap_mode;
    sampler_info.mipLodBias = mip_lod_bias;
    sampler_info.minLod = lod_min;
    sampler_info.maxLod = lod_max;

    if (vkCreateSampler(device->GetDevice(), &sampler_info, nullptr, &sampler) != VK_SUCCESS)
      throw std::runtime_error("failed to create texture sampler!");
  }
}