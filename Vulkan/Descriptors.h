#ifndef __CPU_NW_LIBS_VULKAN_DESCRIPTORS_H
#define __CPU_NW_LIBS_VULKAN_DESCRIPTORS_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <memory>
#include <map>
#include <tuple>

#include "Array.h"
#include "UniformBuffer.h"
#include "TransferArray.h"
#include "Device.h"

namespace Vulkan
{
  struct SDataLayout
  {
    std::vector<std::tuple<Vulkan::StorageType, VkShaderStageFlagBits, uint32_t>> layout;
    std::multimap<Vulkan::StorageType, uint32_t> buffers;
  };

  class Descriptors
  {
  private:
    std::shared_ptr<Vulkan::Device> device;
    std::vector<std::tuple<Vulkan::StorageType, VkShaderStageFlagBits, uint32_t>> buffers;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> descriptor_sets;
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    uint32_t descriptor_pool_capacity = 1;

    VkDescriptorPool CreateDescriptorPool(SDataLayout layout);
    std::vector<VkDescriptorSetLayout> CreateDescriptorSetLayout(SDataLayout data_layout, bool layout_per_set);
    std::vector<VkDescriptorSet> CreateDescriptorSets(VkDescriptorPool pool, std::vector<VkDescriptorSetLayout> layouts);
    void Destroy();
  public:
    Descriptors() = delete;
    Descriptors(const Descriptors &obj) = delete;
    Descriptors& operator= (const Descriptors &obj) = delete;
    Descriptors(std::shared_ptr<Vulkan::Device> dev);
    void Add(std::shared_ptr<IStorage> buffer, VkShaderStageFlagBits stage);
    void Build(bool set_per_buffer);
    void Clear();
    VkDescriptorPool GetDescriptorPool() { return descriptor_pool; }
    std::vector<VkDescriptorSetLayout> GetDescriptorSetLayout() { return descriptor_set_layouts; }
    std::vector<VkDescriptorSet> GetDescriptorSets() { return descriptor_sets; }

    ~Descriptors();
  };
}
#endif