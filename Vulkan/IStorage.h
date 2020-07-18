#ifndef __CPU_NW_LIBS_VULKAN_ISTORAGE_H
#define __CPU_NW_LIBS_VULKAN_ISTORAGE_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <cstring>
#include <memory>

#include "Device.h"

namespace Vulkan
{
  enum class StorageType
  {
    None,
    Default,
    Uniform,
    Vertex
  };

  class IStorage
  {
  protected:
    StorageType type = StorageType::Default;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory buffer_memory = VK_NULL_HANDLE;
    std::shared_ptr<Vulkan::Device> device;
    uint32_t buffer_size = 0;
    uint32_t elements_count = 0;

    friend class StorageBuffer;
  public:
      IStorage() = default;
      ~IStorage()
      {
#ifdef DEBUG
        std::cout << __func__ << std::endl;
#endif
        if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
        {
          vkDestroyBuffer(device->GetDevice(), buffer, nullptr);
          vkFreeMemory(device->GetDevice(), buffer_memory, nullptr);
          device.reset();
        }
      }
      void Update(const void *data, std::size_t length) const
      {
        if (data == nullptr)
          throw std::runtime_error("Update data is empty.");
        
        if (length > buffer_size)
          throw std::runtime_error("length >= buffer_size " + std::to_string (length) + " " + std::to_string (buffer_size));
        
        void *payload = nullptr;
        if (vkMapMemory(device->GetDevice(), buffer_memory, 0, buffer_size, 0, &payload) != VK_SUCCESS)
          throw std::runtime_error("Can't map memory.");
    
        std::memcpy(payload, data, length);
        vkUnmapMemory(device->GetDevice(), buffer_memory);
      }
      
      StorageType Type() const { return type; }

      void* Extract(std::size_t &length) const
      {
        void *payload = nullptr;
        void *result = nullptr;
        if (vkMapMemory(device->GetDevice(), buffer_memory, 0, buffer_size, 0, &payload) != VK_SUCCESS)
          throw std::runtime_error("Can't map memory.");

        result = std::malloc(buffer_size);
        std::memcpy(result, payload, buffer_size);
        vkUnmapMemory(device->GetDevice(), buffer_memory);
        length = buffer_size;
        return result;
      }

      VkBuffer GetBuffer() { return buffer; }
      uint32_t GetElementsCount() { return elements_count; }
  };
}

#endif