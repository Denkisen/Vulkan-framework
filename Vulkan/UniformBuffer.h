#ifndef __CPU_NW_LIBS_VULKAN_UNIFORMBUFFER_H
#define __CPU_NW_LIBS_VULKAN_UNIFORMBUFFER_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>
#include <cstring>
#include <optional>

#include "IStorage.h"
#include "Device.h"

namespace Vulkan
{
  template <class T> class UniformBuffer : public IStorage
  {
  private:
    void Create(std::shared_ptr<Vulkan::Device> dev, T &data);
  public:
    UniformBuffer() = delete;
    UniformBuffer(std::shared_ptr<Vulkan::Device> dev);
    UniformBuffer(std::shared_ptr<Vulkan::Device> dev, T &data);
    UniformBuffer(const UniformBuffer &obj);
    UniformBuffer& operator= (const UniformBuffer &obj);
    UniformBuffer& operator= (const T &obj);
    T Extract() const;
    ~UniformBuffer()
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
  void UniformBuffer<T>::Create(std::shared_ptr<Vulkan::Device> dev, T &data)
  {
    if (dev == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
      throw std::runtime_error("Device is nullptr.");

    device = dev;
    type = StorageType::Uniform; // VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
    buffer_size = sizeof(T);
    elements_count = 1;

    VkBufferCreateInfo buffer_create_info = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      nullptr,
      0,
      buffer_size,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      1,
      nullptr
    };

    if (vkCreateBuffer(dev->GetDevice(), &buffer_create_info, nullptr, &src_buffer) != VK_SUCCESS)
      throw std::runtime_error("Can't create Buffer.");

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto memory_type_index = Vulkan::Supply::GetMemoryTypeIndex(device->GetDevice(), device->GetPhysicalDevice(), src_buffer, buffer_size, flags);

    if (!memory_type_index.has_value())
      throw std::runtime_error("Out of memory.");

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
    
    std::memcpy(payload, &data, sizeof(T));
    vkUnmapMemory(dev->GetDevice(), src_buffer_memory);

    if (vkBindBufferMemory(dev->GetDevice(), src_buffer, src_buffer_memory, 0) != VK_SUCCESS)
      throw std::runtime_error("Can't bind memory to buffer.");
  }

  template <class T>
  UniformBuffer<T>::UniformBuffer(std::shared_ptr<Vulkan::Device> dev)
  {
    T tmp = {};
    Create(dev, tmp);
  }

  template <class T>
  UniformBuffer<T>::UniformBuffer(std::shared_ptr<Vulkan::Device> dev, T &data)
  {
    Create(dev, data);
  }

  template <class T>
  UniformBuffer<T>::UniformBuffer(const UniformBuffer &obj)
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      vkFreeMemory(device->GetDevice(), src_buffer_memory, nullptr);
      vkDestroyBuffer(device->GetDevice(), src_buffer, nullptr);
      device.reset();
    }

    std::size_t sz = 0;
    T *tmp = (T*) Extract(sz);
    Create(obj.device, tmp);
    std::free(tmp);
  }

  template <class T>
  UniformBuffer<T>& UniformBuffer<T>::operator= (const UniformBuffer<T> &obj)
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      vkFreeMemory(device->GetDevice(), src_buffer_memory, nullptr);
      vkDestroyBuffer(device->GetDevice(), src_buffer, nullptr);
      device.reset();
    }
    
    std::size_t sz = 0;
    T *tmp = (T*) Extract(sz);
    Create(obj.device, tmp);
    std::free(tmp);
    
    return *this;
  }

  template <class T>
  UniformBuffer<T>& UniformBuffer<T>::operator= (const T &obj)
  {
    void *payload = nullptr;
    if (vkMapMemory(device->GetDevice(), src_buffer_memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");
    
    std::memcpy(payload, &obj, sizeof(T));
    vkUnmapMemory(device->GetDevice(), src_buffer_memory);

    return *this;
  }

  template <class T>
  T UniformBuffer<T>::Extract() const
  {
    void *payload = nullptr;
    T result;

    if (vkMapMemory(device->GetDevice(), src_buffer_memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");

    std::memcpy(&result, payload, sizeof(T));
    vkUnmapMemory(device->GetDevice(), src_buffer_memory);

    return result;
  }
}

#endif