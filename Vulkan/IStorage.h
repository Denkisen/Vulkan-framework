#ifndef __CPU_NW_LIBS_VULKAN_ISTORAGE_H
#define __CPU_NW_LIBS_VULKAN_ISTORAGE_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <cstring>
#include <memory>

#include "Device.h"

namespace Vulkan
{
  class IStorage
  {
  protected:
    StorageType type = StorageType::Storage;
    VkBuffer src_buffer = VK_NULL_HANDLE;
    VkBuffer dst_buffer = VK_NULL_HANDLE;
    VkDeviceMemory src_buffer_memory = VK_NULL_HANDLE;
    VkDeviceMemory dst_buffer_memory = VK_NULL_HANDLE;
    std::shared_ptr<Vulkan::Device> device;
    uint32_t buffer_size = 0;
    uint32_t elements_count = 0;

    VkBuffer GetDstBuffer() { return dst_buffer; }

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
          if (src_buffer != VK_NULL_HANDLE)
          {
            vkDestroyBuffer(device->GetDevice(), src_buffer, nullptr);
            vkFreeMemory(device->GetDevice(), src_buffer_memory, nullptr);
            src_buffer = VK_NULL_HANDLE;
          }

          if (dst_buffer != VK_NULL_HANDLE)
          {
            vkDestroyBuffer(device->GetDevice(), dst_buffer, nullptr);
            vkFreeMemory(device->GetDevice(), dst_buffer_memory, nullptr);
            dst_buffer = VK_NULL_HANDLE;
          }

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
        if (vkMapMemory(device->GetDevice(), src_buffer_memory, 0, buffer_size, 0, &payload) != VK_SUCCESS)
          throw std::runtime_error("Can't map memory.");
    
        std::memcpy(payload, data, length);
        vkUnmapMemory(device->GetDevice(), src_buffer_memory);
      }
      
      StorageType Type() const { return type; }

      void* Extract(std::size_t &length) const
      {
        void *payload = nullptr;
        void *result = nullptr;
        if (vkMapMemory(device->GetDevice(), src_buffer_memory, 0, buffer_size, 0, &payload) != VK_SUCCESS)
          throw std::runtime_error("Can't map memory.");

        result = std::malloc(buffer_size);
        std::memcpy(result, payload, buffer_size);
        vkUnmapMemory(device->GetDevice(), src_buffer_memory);
        length = buffer_size;
        return result;
      }

      VkBuffer GetBuffer() { return src_buffer; }
      uint32_t GetElementsCount() { return elements_count; }
  };
}

#endif