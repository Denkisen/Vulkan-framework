#ifndef __CPU_NW_LIBS_VULKAN_STORAGEBUFFER_H
#define __CPU_NW_LIBS_VULKAN_STORAGEBUFFER_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

#include "IStorage.h"
#include "Device.h"
#include "Supply.h"

namespace Vulkan
{
  struct DataLayout
  {
    std::vector<StorageType> layout;
    std::size_t uniform_buffers = 0;
    std::size_t storage_buffers = 0;
  };
  
  class StorageBuffer
  {
  private:
    std::vector<IStorage*> buffers;
    VkDevice device = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;

    VkDescriptorSetLayout CreateDescriptorSetLayout(DataLayout data_layout);
    VkDescriptorPool CreateDescriptorPool(DataLayout data_layout);
    VkDescriptorSet CreateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout);
    std::vector<IStorage*> UpdateDescriptorSet(VkDescriptorSet set, const std::vector<IStorage*> &data);
    DataLayout GetDataLayout(const std::vector<IStorage*> &data);
    void Clear();
  public:
    StorageBuffer() = default;
    StorageBuffer(Device &dev);
    StorageBuffer(const StorageBuffer &obj);
    StorageBuffer(Device &dev, std::vector<IStorage*> &data);
    StorageBuffer& operator= (const StorageBuffer &obj);
    StorageBuffer& operator= (const std::vector<IStorage*> &obj);
    const VkDescriptorSetLayout GetDescriptorSetLayout() { return descriptor_set_layout; }
    const VkDescriptorPool GetDescriptorPool() { return descriptor_pool; }
    const VkDescriptorSet GetDescriptorSet() { return descriptor_set; }
    const VkDevice GetDevice() { return device; }
    const StorageType GetStorageTypeByIndex(const std::size_t index) { return index < buffers.size() ? buffers[index]->Type() : StorageType::None; }
    void* Extract(std::size_t &length, const std::size_t index);
    void UpdateValue(void *data_ptr, const std::size_t length, const std::size_t index);
    const std::size_t Size() { return buffers.size(); }

    ~StorageBuffer();
  };
}

#endif