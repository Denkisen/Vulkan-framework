#ifndef __CPU_NW_LIBS_VULKAN_ARRAY_H
#define __CPU_NW_LIBS_VULKAN_ARRAY_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <cstring>
#include <optional>
#include <cmath>

#include "IStorage.h"
#include "Device.h"

namespace Vulkan
{
  template <class T> class Array : public IStorage
  {
  private:
    void Create(std::shared_ptr<Vulkan::Device> dev, T *data, std::size_t len, Vulkan::StorageType storage_type);
    std::vector<T> data;
  public:
    Array() = delete;
    Array(std::shared_ptr<Vulkan::Device> dev, StorageType storage_type);
    Array(std::shared_ptr<Vulkan::Device> dev, std::vector<T> &data, Vulkan::StorageType storage_type);
    Array(std::shared_ptr<Vulkan::Device> dev, T *data, std::size_t len, Vulkan::StorageType storage_type);
    Array(const Array<T> &array);
    Array<T>& operator= (const Array<T> &obj);
    Array<T>& operator= (const std::vector<T> &obj);
    std::vector<T> Extract() const;
    ~Array()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
    }
  };
}

namespace Vulkan
{
  template <class T>
  void Array<T>::Create(std::shared_ptr<Vulkan::Device> dev, T *data, std::size_t len, Vulkan::StorageType storage_type)
  {
    if (len == 0 || data == nullptr || dev == nullptr)
      throw std::runtime_error("Data array is empty.");

    buffer_size = len * sizeof(T);
    elements_count = len;
    device = dev;
    this->type = storage_type;

    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = buffer_size;
    buffer_create_info.usage = Vulkan::Supply::StorageTypeToBufferUsageFlags(this->type);;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(dev->GetDevice(), &buffer_create_info, nullptr, &src_buffer) != VK_SUCCESS)
      throw std::runtime_error("Can't create Buffer.");

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto memory_type_index = Vulkan::Supply::GetMemoryTypeIndex(dev->GetDevice(), dev->GetPhysicalDevice(), src_buffer, buffer_size, flags);

    if (!memory_type_index.has_value())
      throw std::runtime_error("Out of memory.");

    this->data.resize(std::ceil((double) buffer_size / (sizeof(T))));
    std::copy(data, data + len, this->data.begin());

    VkMemoryAllocateInfo memory_allocate_info = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      0,
      buffer_size,
      (uint32_t) memory_type_index.value()
    };

    if (vkAllocateMemory(dev->GetDevice(), &memory_allocate_info, nullptr, &src_buffer_memory) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate memory");
    
    void *payload = nullptr;
    if (vkMapMemory(dev->GetDevice(), src_buffer_memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");
    
    std::memcpy(payload, this->data.data(), buffer_size);
    vkUnmapMemory(dev->GetDevice(), src_buffer_memory);

    if (vkBindBufferMemory(dev->GetDevice(), src_buffer, src_buffer_memory, 0) != VK_SUCCESS)
      throw std::runtime_error("Can't bind memory to buffer.");
  }

  template <class T>
  Array<T>::Array(std::shared_ptr<Vulkan::Device> dev, std::vector<T> &data, Vulkan::StorageType storage_type)
  {
    if (data.size() == 0)
      throw std::runtime_error("Data array is empty.");
    
    Create(dev, data.data(), data.size(), storage_type);
  }

  template <class T>
  Array<T>::Array(std::shared_ptr<Vulkan::Device> dev, T *data, std::size_t len, Vulkan::StorageType storage_type)
  {
    Create(dev, data, len, storage_type);
  }

  template <class T>
  Array<T>::Array(std::shared_ptr<Vulkan::Device> dev, Vulkan::StorageType storage_type)
  {
    this->data.resize(64);
    Create(dev, this->data.data(), this->data.size(), storage_type);
  }

  template <class T> 
  Array<T>::Array(const Array<T> &array)
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      vkFreeMemory(device->GetDevice(), src_buffer_memory, nullptr);
      vkDestroyBuffer(device->GetDevice(), src_buffer, nullptr);
      device.reset();
    }

    std::vector<T> data(array.Extract());
    Create(array.device, data.data(), data.size(), array.type);
  }

  template <class T> 
  Array<T>& Array<T>::operator= (const Array<T> &obj)
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      vkFreeMemory(device->GetDevice(), src_buffer_memory, nullptr);
      vkDestroyBuffer(device->GetDevice(), src_buffer, nullptr);
      device.reset();
    }
    
    std::vector<T> data(obj.Extract());
    Create(obj.device, data.data(), data.size(), obj.type);

    return *this;
  }

  template <class T> 
  Array<T>& Array<T>::operator= (const std::vector<T> &obj)
  {
    if (obj.size() > data.size())
    {
      if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
      {
        vkFreeMemory(device->GetDevice(), src_buffer_memory, nullptr);
        vkDestroyBuffer(device->GetDevice(), src_buffer, nullptr);
        
        Create(device, const_cast<T*> (obj.data()), obj.size(), type);
      }
    }
    else
    {
      this->data = obj;
      void *payload = nullptr;
      if (vkMapMemory(device->GetDevice(), src_buffer_memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
        throw std::runtime_error("Can't map memory.");
    
      buffer_size = this->data.size() * sizeof(T);
      std::memcpy(payload, this->data.data(), buffer_size);
      vkUnmapMemory(device->GetDevice(), src_buffer_memory);
    }

    return *this;
  }
  
  template <class T> 
  std::vector<T> Array<T>::Extract() const
  {
    void *payload = nullptr;
    if (vkMapMemory(device->GetDevice(), src_buffer_memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");
    
    std::vector<T> data(buffer_size / sizeof(T));
    std::copy((T *)payload, &((T *)payload)[data.size()], data.begin());
    vkUnmapMemory(device->GetDevice(), src_buffer_memory);

    return data;
  }
}

#endif