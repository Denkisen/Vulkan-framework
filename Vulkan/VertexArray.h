#ifndef __CPU_NW_LIBS_VULKAN_VERTEXBUFFER_H
#define __CPU_NW_LIBS_VULKAN_VERTEXBUFFER_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>
#include <cstring>
#include <optional>
#include <cmath>

#include "IStorage.h"
#include "Device.h"

namespace Vulkan
{
  template <class T> class VertexArray : public IStorage
  {
  private:
    void Create(std::shared_ptr<Vulkan::Device> dev, T *data, std::size_t len, uint32_t f_queue);
    std::vector<T> data;
  public:
    VertexArray() = delete;
    VertexArray(std::shared_ptr<Vulkan::Device> dev, uint32_t family_q);
    VertexArray(std::shared_ptr<Vulkan::Device> dev, std::vector<T> &data, uint32_t family_q);
    VertexArray(std::shared_ptr<Vulkan::Device> dev, T *data, std::size_t len, uint32_t family_q);
    VertexArray(const VertexArray<T> &array);
    VertexArray<T>& operator= (const VertexArray<T> &obj);
    VertexArray<T>& operator= (const std::vector<T> &obj);
    std::vector<T> Extract() const;
    ~VertexArray()
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
  void VertexArray<T>::Create(std::shared_ptr<Vulkan::Device> dev, T *data, std::size_t len, uint32_t f_queue)
  {
    if (len == 0 || data == nullptr || dev == nullptr)
      throw std::runtime_error("Data array is empty.");

    buffer_size = len * sizeof(T);
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(dev->GetPhysicalDevice(), &properties);

    VkBufferCreateInfo buffer_create_info = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      0,
      0,
      buffer_size,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      1,
      &f_queue
    };

    if (vkCreateBuffer(dev->GetDevice(), &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
      throw std::runtime_error("Can't create Buffer.");

    VkMemoryRequirements mem_req = {};
    vkGetBufferMemoryRequirements(dev->GetDevice(), buffer, &mem_req);

    buffer_size = mem_req.size;

    this->data.resize(std::ceil(buffer_size / (sizeof(T))));
    std::copy(data, data + len, this->data.begin());

    size_t memory_type_index = VK_MAX_MEMORY_TYPES;

    for (size_t i = 0; i < properties.memoryTypeCount; i++) 
    {
      if (mem_req.memoryTypeBits & (1 << i) && 
         (properties.memoryTypes[i].propertyFlags & 
            (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)) &&
         (buffer_size < properties.memoryHeaps[properties.memoryTypes[i].heapIndex].size))
      {
        memory_type_index = i;
        break;
      }
    }

    if (memory_type_index == VK_MAX_MEMORY_TYPES)
      throw std::runtime_error("Out of memory.");

    VkMemoryAllocateInfo memory_allocate_info = {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      0,
      buffer_size,
      (uint32_t) memory_type_index
    };
    
    if (vkAllocateMemory(dev->GetDevice(), &memory_allocate_info, nullptr, &buffer_memory) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate memory");
    
    void *payload = nullptr;
    if (vkMapMemory(dev->GetDevice(), buffer_memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");
    
    std::memcpy(payload, this->data.data(), buffer_size);
    vkUnmapMemory(dev->GetDevice(), buffer_memory);

    if (vkBindBufferMemory(dev->GetDevice(), buffer, buffer_memory, 0) != VK_SUCCESS)
      throw std::runtime_error("Can't bind memory to buffer.");

    device = dev;
    family_queue = f_queue;
    type = StorageType::Vertex; // VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
  }

  template <class T>
  VertexArray<T>::VertexArray(std::shared_ptr<Vulkan::Device> dev, std::vector<T> &data, uint32_t family_q)
  {
    if (data.size() == 0)
      throw std::runtime_error("Data array is empty.");
    
    Create(dev, this->data.data(), this->data.size(), family_q);
  }

  template <class T>
  VertexArray<T>::VertexArray(std::shared_ptr<Vulkan::Device> dev, T *data, std::size_t len, uint32_t family_q)
  {
    Create(dev, data, len, family_q);
  }

  template <class T>
  VertexArray<T>::VertexArray(std::shared_ptr<Vulkan::Device> dev, uint32_t family_q)
  {
    this->data.resize(64, 0.0);
    Create(dev, this->data.data(), this->data.size(), family_q);
  }

  template <class T> 
  VertexArray<T>::VertexArray(const VertexArray<T> &array)
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      vkFreeMemory(device->GetDevice(), buffer_memory, nullptr);
      vkDestroyBuffer(device->GetDevice(), buffer, nullptr);
      device.reset();
    }

    std::vector<T> data(array.Extract());
    Create(array.device, data.data(), data.size(), array.family_queue);
  }

  template <class T> 
  VertexArray<T>& VertexArray<T>::operator= (const VertexArray<T> &obj)
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      vkFreeMemory(device->GetDevice(), buffer_memory, nullptr);
      vkDestroyBuffer(device->GetDevice(), buffer, nullptr);
      device.reset();
    }
    
    std::vector<T> data(obj.Extract());
    Create(obj.device, data.data(), data.size(), obj.family_queue);

    return *this;
  }

  template <class T> 
  VertexArray<T>& VertexArray<T>::operator= (const std::vector<T> &obj)
  {
    if (obj.size() > data.size())
    {
      if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
      {
        vkFreeMemory(device->GetDevice(), buffer_memory, nullptr);
        vkDestroyBuffer(device->GetDevice(), buffer, nullptr);
        
        Create(device, const_cast<T*> (obj.data()), obj.size(), family_queue);
      }
    }
    else
    {
      this->data = obj;
      void *payload = nullptr;
      if (vkMapMemory(device->GetDevice(), buffer_memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
        throw std::runtime_error("Can't map memory.");
    
      buffer_size = this->data.size() * sizeof(T);
      std::memcpy(payload, this->data.data(), buffer_size);
      vkUnmapMemory(device->GetDevice(), buffer_memory);
    }

    return *this;
  }
  
  template <class T> 
  std::vector<T> VertexArray<T>::Extract() const
  {
    void *payload = nullptr;
    if (vkMapMemory(device->GetDevice(), buffer_memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");
    
    std::vector<T> data(buffer_size / sizeof(T));
    std::copy((T *)payload, &((T *)payload)[data.size()], data.begin());
    vkUnmapMemory(device->GetDevice(), buffer_memory);

    return data;
  }
}

#endif