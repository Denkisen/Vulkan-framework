#ifndef __VULKAN_DESCRIPTORS_H
#define __VULKAN_DESCRIPTORS_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <mutex>
#include <iostream>

#include "Device.h"
#include "StorageArray.h"

namespace Vulkan
{
  enum class DescriptorType
  {
    BufferStorage = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    BufferUniform = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    TexelStorage = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
    TexelUniform = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER,
    ImageSamplerCombined = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    ImageSampled = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    ImageStorage = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    Sampler = VK_DESCRIPTOR_TYPE_SAMPLER
  };

  struct DescriptorSetLayout
  {
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    VkDescriptorSet set = VK_NULL_HANDLE;
  };

  struct DescriptorInfo
  {
    struct BufferInfo
    {
      VkBuffer buffer = VK_NULL_HANDLE;
      VkBufferView buffer_view = VK_NULL_HANDLE;
    } buffer_info;

    struct ImageInfo
    {
      VkImageLayout image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
      VkSampler sampler = VK_NULL_HANDLE;
      VkImageView image_view = VK_NULL_HANDLE;
    } image_info;

    VkDeviceSize size = 0;
    VkDeviceSize offset = 0;
    VkShaderStageFlags stage = VK_SHADER_STAGE_ALL;
    DescriptorType type;
    static DescriptorType MapStorageType(StorageType type) noexcept;
  };

  class LayoutConfig
  {
  private:
    friend class Descriptors_impl;
    std::vector<DescriptorInfo> info;
  public:
    LayoutConfig() = default;
    ~LayoutConfig() noexcept = default;
    auto &AddBufferOrImage(const DescriptorInfo desc_info)
    { 
      if (desc_info.buffer_info.buffer == VK_NULL_HANDLE && desc_info.image_info.image_view == VK_NULL_HANDLE)
      {
        Logger::EchoWarning("Nothing to add", __func__);
        return *this;
      }

      if (desc_info.buffer_info.buffer != VK_NULL_HANDLE && desc_info.image_info.image_view != VK_NULL_HANDLE)
      {
        Logger::EchoError("Must be only one buffer or image", __func__);
        return *this;
      }

      if (desc_info.buffer_info.buffer != VK_NULL_HANDLE && desc_info.size == 0)
      {
        Logger::EchoError("Size is 0", __func__);
        return *this;
      }

      switch (desc_info.type)
      {
      case DescriptorType::Sampler:
        if (desc_info.image_info.sampler != VK_NULL_HANDLE)
        {
          info.push_back(desc_info);
          break;
        }
      case DescriptorType::ImageStorage:
      case DescriptorType::ImageSampled:
        if (desc_info.image_info.image_view != VK_NULL_HANDLE)
        {
          info.push_back(desc_info);
          break;
        }
      case DescriptorType::ImageSamplerCombined:
        if (desc_info.image_info.image_view != VK_NULL_HANDLE && desc_info.image_info.sampler != VK_NULL_HANDLE)
        {
          info.push_back(desc_info);
          break;
        }
      case DescriptorType::BufferStorage:
      case DescriptorType::BufferUniform:
      case DescriptorType::TexelUniform:
      case DescriptorType::TexelStorage:
        if (desc_info.buffer_info.buffer != VK_NULL_HANDLE)
        {
          info.push_back(desc_info);
          break;
        }
      default:
        Logger::EchoError("Incompatible DescriptorType and image", __func__);
      }

      return *this; 
    }
  };

  class Descriptors_impl
  {
  public:
    Descriptors_impl() = delete;
    Descriptors_impl(const Descriptors_impl &obj) = delete;
    Descriptors_impl(Descriptors_impl &&obj) = delete;
    Descriptors_impl &operator=(Descriptors_impl &&obj) = delete;
    Descriptors_impl &operator=(const Descriptors_impl &obj) = delete;
    ~Descriptors_impl() noexcept;
  private:
    friend class Descriptors;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    std::shared_ptr<Device> device;
    std::vector<LayoutConfig> build_config;
    std::vector<LayoutConfig> build_config_copy;
    std::vector<DescriptorSetLayout> layouts;
    struct PoolConfig
    {
      std::vector<VkDescriptorPoolSize> sizes;
      uint32_t max_sets = 0;
      void AddDescriptorType(const DescriptorType type) noexcept
      {
        for (auto &d : sizes)
        {
          if (d.type == (VkDescriptorType) type)
          {
            d.descriptorCount++;
            return;
          }
        }
        try { sizes.push_back({(VkDescriptorType) type, 1}); } catch (...) {}
      }
    };
    
    Descriptors_impl(std::shared_ptr<Device> dev);
    VkDescriptorPool CreateDescriptorPool(const PoolConfig &pool_conf);
    DescriptorSetLayout CreateDescriptorSetLayout(const LayoutConfig &info);
    VkResult CreateDescriptorSets(const VkDescriptorPool pool, DescriptorSetLayout &layout);
    VkResult UpdateDescriptorSet(const DescriptorSetLayout &layout, const LayoutConfig &info);
    void Destroy() noexcept;

    VkResult AddSetLayoutConfig(const LayoutConfig &config);
    VkResult BuildAllSetLayoutConfigs();
    void ClearAllSetLayoutConfigs() noexcept;
    size_t GetLayoutsCount() const noexcept { return layouts.size(); }
    VkDescriptorSetLayout GetDescriptorSetLayout(const size_t index) const noexcept {  return index < layouts.size() ? layouts[index].layout : VK_NULL_HANDLE; }
    VkDescriptorSet GetDescriptorSet(const size_t index) const noexcept { return index < layouts.size() ? layouts[index].set : VK_NULL_HANDLE; }
    std::vector<VkDescriptorSetLayout> GetDescriptorSetLayouts() const;
    std::vector<VkDescriptorSet> GetDescriptorSets() const;
    std::shared_ptr<Device> GetDevice() const noexcept { return device; }
  };

  class Descriptors
  {
  private:
    std::unique_ptr<Descriptors_impl> impl;
  public:
    Descriptors() = delete;
    Descriptors(const Descriptors &obj);
    Descriptors(Descriptors &&obj) noexcept;
    Descriptors(std::shared_ptr<Device> dev) : impl(std::unique_ptr<Descriptors_impl>(new Descriptors_impl(dev))) {}
    Descriptors &operator=(const Descriptors &obj);
    Descriptors &operator=(Descriptors &&obj) noexcept;
    void swap(Descriptors &obj) noexcept;
    ~Descriptors() noexcept = default;
    VkResult AddSetLayoutConfig(const LayoutConfig &config) { if (impl.get()) return impl->AddSetLayoutConfig(config); return VK_ERROR_UNKNOWN; }
    VkResult BuildAllSetLayoutConfigs() { if (impl.get()) return impl->BuildAllSetLayoutConfigs(); return VK_ERROR_UNKNOWN; }
    void ClearAllSetLayoutConfigs() { if (impl.get()) impl->ClearAllSetLayoutConfigs(); }
    size_t GetLayoutsCount() const noexcept { if (impl.get()) return impl->GetLayoutsCount(); return 0; }
    VkDescriptorSetLayout GetDescriptorSetLayout(const size_t index) const noexcept { if (impl.get()) return impl->GetDescriptorSetLayout(index); return VK_NULL_HANDLE; }
    VkDescriptorSet GetDescriptorSet(const size_t index) const noexcept { if (impl.get()) return impl->GetDescriptorSet(index); return VK_NULL_HANDLE; }
    std::vector<VkDescriptorSetLayout> GetDescriptorSetLayouts() const { if (impl.get()) return impl->GetDescriptorSetLayouts(); return {}; }
    std::vector<VkDescriptorSet> GetDescriptorSets() const { if (impl.get()) return impl->GetDescriptorSets(); return {}; }
    bool IsValid() const noexcept { return impl.get() && impl->descriptor_pool != VK_NULL_HANDLE; }
    std::shared_ptr<Device> GetDevice() const noexcept { if (impl.get()) return impl->GetDevice(); return nullptr; }
  };

  void swap(Descriptors &lhs, Descriptors &rhs) noexcept;
}

#endif