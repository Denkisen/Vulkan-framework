#ifndef __CPU_NW_LIBS_VULKAN_DESCRIPTORS_H
#define __CPU_NW_LIBS_VULKAN_DESCRIPTORS_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <memory>
#include <map>

#include "Array.h"
#include "UniformBuffer.h"
#include "TransferArray.h"
#include "Device.h"

namespace Vulkan
{
  struct DescriptorInfo
  {
    std::vector<VkBuffer> buffers;
    std::vector<VkDescriptorSetLayout> layouts;
    std::vector<VkDescriptorSet> sets;
    std::vector<Vulkan::StorageType> types;
    VkShaderStageFlagBits stage;
    bool multiple_layouts_one_binding = true;
  };

  class Descriptors
  {
  private:
    std::shared_ptr<Vulkan::Device> device;
    std::vector<DescriptorInfo> buffers_info;
    std::vector<DescriptorInfo> build_buffers_info;
    std::multimap<Vulkan::StorageType, uint32_t> pool_config;
    uint32_t sets_to_allocate = 0;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

    VkDescriptorPool CreateDescriptorPool();
    void CreateDescriptorSetLayouts(DescriptorInfo &info);
    void CreateDescriptorSets(DescriptorInfo &info);
    void UpdateDescriptorSet(DescriptorInfo &info);
    void Destroy();
  public:
    Descriptors() = delete;
    Descriptors(const Descriptors &obj) = delete;
    Descriptors& operator= (const Descriptors &obj) = delete;
    Descriptors(std::shared_ptr<Vulkan::Device> dev);
    void Add(std::vector<std::shared_ptr<IStorage>> data, VkShaderStageFlagBits stage, bool multiple_layouts_one_binding);
    void Build();
    void Clear();
    VkDescriptorPool GetDescriptorPool() { return descriptor_pool; }
    std::vector<VkDescriptorSet> GetDescriptorSet(size_t index) { return index < buffers_info.size() ? buffers_info[index].sets : std::vector<VkDescriptorSet>(); }
    std::vector<VkDescriptorSetLayout> GetDescriptorSetLayout(size_t index) { return index < buffers_info.size() ? buffers_info[index].layouts : std::vector<VkDescriptorSetLayout>(); }
    ~Descriptors();
  };
}
#endif