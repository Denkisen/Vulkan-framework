#ifndef __CPU_NW_VULKAN_DESCRIPTORS_H
#define __CPU_NW_VULKAN_DESCRIPTORS_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <memory>
#include <map>

#include "Buffer.h"
#include "Image.h"
#include "Sampler.h"
#include "Device.h"

namespace Vulkan
{  
  enum class DescriptorType
  {
    BufferStorage = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
    BufferUniform = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
    ImageSamplerCombined = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
    ImageSampled = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
    ImageStorage = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE,
    Sampler = VK_DESCRIPTOR_TYPE_SAMPLER
  };

  struct DescriptorSetLayout
  {
    VkDescriptorSetLayout layout = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> sets;
    uint32_t sets_count = 0;
    VkShaderStageFlags stage;
  };

  struct DescriptorInfo
  {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkImageLayout image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    VkSampler sampler = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
    uint32_t binding = 0;
    VkShaderStageFlags stage;
    Vulkan::DescriptorType type;
  };

  class Descriptors
  {
  private:
    std::shared_ptr<Vulkan::Device> device;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    std::vector<DescriptorSetLayout> layouts;
    std::vector<std::pair<bool, std::vector<DescriptorInfo>>> build_info;
    std::map<Vulkan::DescriptorType, uint32_t> pool_config;
    VkDescriptorPool CreateDescriptorPool(std::map<Vulkan::DescriptorType, uint32_t> pool_conf);

    void CreateDescriptorSetLayout(DescriptorSetLayout &layout, std::vector<DescriptorInfo> info);
    void CreateDescriptorSets(const VkDescriptorPool pool, DescriptorSetLayout &info);
    void UpdateDescriptorSet(DescriptorSetLayout &layout, std::vector<DescriptorInfo> info);
    Vulkan::DescriptorType MapStorageType(Vulkan::StorageType type);
    void Destroy();
  public:
    Descriptors() = delete;
    Descriptors(const Descriptors &obj) = delete;
    Descriptors& operator= (const Descriptors &obj) = delete;
    Descriptors(std::shared_ptr<Vulkan::Device> dev);
    void ClearDescriptorSetLayout(const uint32_t index);
    void Add(const uint32_t index, const std::shared_ptr<IBuffer> buffer, const VkShaderStageFlags stage, const uint32_t binding);
    void Add(const uint32_t index, const std::shared_ptr<Image> image, const std::shared_ptr<Sampler> sampler, const VkShaderStageFlags stage, const uint32_t binding);
    void BuildAll();
    size_t GetLayoutsCount() const { return layouts.size(); }
    VkDescriptorSetLayout GetDescriptorSetLayout(const size_t index) const { return index < layouts.size() ? layouts[index].layout : VK_NULL_HANDLE; }
    std::vector<VkDescriptorSet> GetDescriptorSet(const size_t layout_index) const { return layout_index < layouts.size() ? layouts[layout_index].sets : std::vector<VkDescriptorSet>(); }
    ~Descriptors();
  };
}
#endif