#ifndef __CPU_NW_VULKAN_BUFFER_H
#define __CPU_NW_VULKAN_BUFFER_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <memory>
#include <optional>
#include <cmath>

#include "Device.h"
#include "Supply.h"

namespace Vulkan
{
  enum class StorageType
  {
    None,
    Storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    Uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    Vertex = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    Index = VK_BUFFER_USAGE_INDEX_BUFFER_BIT
  };

  class IBuffer
  {
  protected: 
    std::shared_ptr<Vulkan::Device> device;
    size_t size = 0;
    size_t item_count = 0; 
    Vulkan::StorageType type = Vulkan::StorageType::None;
    HostVisibleMemory access = HostVisibleMemory::HostVisible;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
  public:
    IBuffer() = default;
    IBuffer(const IBuffer &obj) = delete;
    IBuffer& operator=(const IBuffer &obj) = delete;
    Vulkan::StorageType Type() const { return type; }
    Vulkan::HostVisibleMemory MemoryAccess() const { return access; }
    size_t ItemsCount() const { return item_count; }
    size_t BufferLength() const { return size; }
    size_t ItemTypeSize() const { return item_count == 0 ? 0 : size / item_count; }
    VkBuffer GetBuffer() const { return buffer; }

    ~IBuffer()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
      if (buffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer(device->GetDevice(), buffer, nullptr);
        vkFreeMemory(device->GetDevice(), memory, nullptr);
        buffer = VK_NULL_HANDLE;
      }

      device.reset();
    }
  };

  template <class T> class Buffer : public IBuffer
  {
  private:
    size_t AllocBuffer(size_t length);
  public:
    Buffer() = delete;
    Buffer(std::shared_ptr<Vulkan::Device> dev, Vulkan::StorageType buffer_type, Vulkan::HostVisibleMemory access);
    Buffer(const Buffer<T> &obj);
    Buffer<T>& operator= (const Buffer<T> &obj);
    Buffer<T>& operator= (const std::vector<T> &obj);
    Buffer<T>& operator= (const T &obj);
    bool TryLoad(const std::vector<T> &data);
    std::vector<T> Extract() const;
    T Extract(const size_t index) const;
    void AllocateBuffer(size_t length);

    ~Buffer()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
    }
  };

// Implementation

  template <class T>
  Buffer<T>::Buffer(std::shared_ptr<Vulkan::Device> dev, Vulkan::StorageType buffer_type, Vulkan::HostVisibleMemory access)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE || dev->GetPhysicalDevice() == VK_NULL_HANDLE)
      throw std::runtime_error("Invalid device pointer.");

    if (buffer_type == Vulkan::StorageType::None)
      throw std::runtime_error("You should not use StorageType::None");

    device = dev;
    type = buffer_type;
    this->access = access;
  }

  template <class T>
  Buffer<T>::Buffer(const Buffer<T> &obj)
  {
    device = obj.device;
    type = obj.type;
    access = obj.access;
    if (obj.buffer != VK_NULL_HANDLE)
    {
      if (obj.access == Vulkan::HostVisibleMemory::HostVisible)
      {
        std::vector<T> tmp = obj.Extract();
        if (!TryLoad(tmp))
          throw std::runtime_error("Can't copy buffer.");
      }
    }
  }

  template <class T>
  Buffer<T>& Buffer<T>::operator= (const Buffer<T> &obj)
  {
    if (type != obj.type || access != obj.access)
    {
      if (buffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer(device->GetDevice(), buffer, nullptr);
        vkFreeMemory(device->GetDevice(), memory, nullptr);
        buffer = VK_NULL_HANDLE;
      }
    }

    device = obj.device;
    type = obj.type;
    access = obj.access;

    if (obj.access == Vulkan::HostVisibleMemory::HostVisible)
    {
      std::vector<T> tmp = obj.Extract();
      if (!TryLoad(tmp))
        throw std::runtime_error("Can't copy buffer.");
    }

    return *this;
  }

  template <class T>
  Buffer<T>& Buffer<T>::operator= (const std::vector<T> &obj)
  {
    TryLoad(obj);
    return *this;
  }

  template <class T>
  Buffer<T>& Buffer<T>::operator= (const T &obj)
  {
    std::vector<T> tmp(1, obj);
    TryLoad(tmp);
    return *this;
  }

  template <class T>
  size_t Buffer<T>::AllocBuffer(size_t length)
  {
    uint32_t result_size = (std::ceil(length / 256.0) * 256);

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = result_size;
    buffer_create_info.usage = (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT) | (VkBufferUsageFlags) type;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device->GetDevice(), &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
      throw std::runtime_error("Can't create Buffer.");

    VkMemoryPropertyFlags flags = (VkMemoryPropertyFlags) access;
        
    auto memory_type_index = Vulkan::Supply::GetMemoryTypeIndex(device->GetDevice(), device->GetPhysicalDevice(), buffer, result_size, flags);

    if (!memory_type_index.has_value())
      throw std::runtime_error("Out of memory.");

    VkMemoryAllocateInfo memory_allocate_info = 
    {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      0,
      result_size,
      (uint32_t) memory_type_index.value()
    };

    if (vkAllocateMemory(device->GetDevice(), &memory_allocate_info, nullptr, &memory) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate memory");

    if (vkBindBufferMemory(device->GetDevice(), buffer, memory, 0) != VK_SUCCESS)
      throw std::runtime_error("Can't bind memory to buffer.");

    return result_size;
  }

  template <class T>
  bool Buffer<T>::TryLoad(const std::vector<T> &data)
  {
    if (data.empty()) return true;

    AllocateBuffer(data.size());

    if (access == Vulkan::HostVisibleMemory::HostVisible)
    {
      void *payload = nullptr;
      if (vkMapMemory(device->GetDevice(), memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
        throw std::runtime_error("Can't map memory.");
    
      std::copy(data.begin(), data.end(), (T*) payload);
      vkUnmapMemory(device->GetDevice(), memory);

      return true;
    }
    else
      return false;
  }

  template <class T>
  void Buffer<T>::AllocateBuffer(size_t length)
  {
    if (size < length)
    {
      if (buffer != VK_NULL_HANDLE)
      {
        vkDestroyBuffer(device->GetDevice(), buffer, nullptr);
        vkFreeMemory(device->GetDevice(), memory, nullptr);
        buffer = VK_NULL_HANDLE;
      }
    }

    if (buffer == VK_NULL_HANDLE)
    {
      size = AllocBuffer(length * sizeof(T));
      item_count = length;
    }
  }

  template <class T>
  std::vector<T> Buffer<T>::Extract() const
  {
    if (memory == VK_NULL_HANDLE || item_count == 0 || access == Vulkan::HostVisibleMemory::HostInvisible)
      return {};

    std::vector<T> result(item_count);
    void *payload = nullptr;;
    if (vkMapMemory(device->GetDevice(), memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");
    
    std::copy((T*) payload, &((T*) payload)[item_count], result.begin());
    vkUnmapMemory(device->GetDevice(), memory);

    return result;
  }

  template <class T>
  T Buffer<T>::Extract(const size_t index) const
  {
    T result;
    if (memory == VK_NULL_HANDLE || item_count == 0 || access == Vulkan::HostVisibleMemory::HostInvisible)
      return result;

    void *payload;
    if (vkMapMemory(device->GetDevice(), memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");
    
    std::copy((T *) payload, &((T*) payload)[1], &result);
    vkUnmapMemory(device->GetDevice(), memory);

    return result;
  }
}

#endif