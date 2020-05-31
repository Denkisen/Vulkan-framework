#ifndef __CPU_NW_LIBS_VULKAN_ISTORAGE_H
#define __CPU_NW_LIBS_VULKAN_ISTORAGE_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <cstring>

namespace Vulkan
{
  enum class StorageType
  {
    None,
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

    friend class StorageBuffer;
    friend class Supply;
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
      void Update(const void *data, std::size_t length) const
      {
        if (data == nullptr)
          throw std::runtime_error("Update data is empty.");
        
        if (length > buffer_size)
          throw std::runtime_error("length >= buffer_size " + std::to_string (length) + " " + std::to_string (buffer_size));
        
        void *payload = nullptr;
        if (vkMapMemory(device, buffer_memory, 0, buffer_size, 0, &payload) != VK_SUCCESS)
          throw std::runtime_error("Can't map memory.");
    
        std::memcpy(payload, data, length);
        vkUnmapMemory(device, buffer_memory);
      }
      
      StorageType Type() const { return type; }

      void* Extract(std::size_t &length) const
      {
        void *payload = nullptr;
        void *result = nullptr;
        if (vkMapMemory(device, buffer_memory, 0, buffer_size, 0, &payload) != VK_SUCCESS)
          throw std::runtime_error("Can't map memory.");

        result = std::malloc(buffer_size);
        std::memcpy(result, payload, buffer_size);
        vkUnmapMemory(device, buffer_memory);
        length = buffer_size;
        return result;
      }
  };
}

#endif