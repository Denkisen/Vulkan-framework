#ifndef __CPU_NW_LIBS_VULKAN_ISTORAGE_H
#define __CPU_NW_LIBS_VULKAN_ISTORAGE_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <cstring>

namespace Vulkan
{
  enum class StorageType
  {
    Default,
    Uniform
  };

  class IStorage
  {
  protected:
    StorageType type = StorageType::Default;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory buffer_memory = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPhysicalDevice p_device = VK_NULL_HANDLE;
    uint32_t buffer_size = 0;
    uint32_t family_queue = 0;

    template <typename O> friend class Offload;
  public:
      IStorage() = default;
      ~IStorage()
      {
#ifdef DEBUG
        std::cout << __func__ << std::endl;
#endif
        if (device != VK_NULL_HANDLE)
        {
          vkFreeMemory(device, buffer_memory, nullptr);
          vkDestroyBuffer(device, buffer, nullptr);
          device = VK_NULL_HANDLE;
        }
      }
      void Update(const void *data) const
      {
        if (data == nullptr)
          throw std::runtime_error("Update data is empty.");
        
        void *payload = nullptr;
        if (vkMapMemory(device, buffer_memory, 0, buffer_size, 0, &payload) != VK_SUCCESS)
          throw std::runtime_error("Can't map memory.");
    
        std::memcpy(payload, data, buffer_size);
        vkUnmapMemory(device, buffer_memory);
      }
      StorageType Type() const { return type; }
  };
}

#endif