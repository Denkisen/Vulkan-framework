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
  enum class BufferUsage
  {
    Transfer_src = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    Transfer_dst = VK_BUFFER_USAGE_TRANSFER_DST_BIT
  };

  class IBuffer
  {
  protected: 
    std::shared_ptr<Vulkan::Device> device;
    size_t size = 0;
    size_t item_count = 0; 
    Vulkan::StorageType type = Vulkan::StorageType::None;
    BufferUsage usage = BufferUsage::Transfer_src;
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
  public:
    IBuffer() = default;
    IBuffer(const IBuffer &obj) = delete;
    IBuffer& operator=(const IBuffer &obj) = delete;
    Vulkan::StorageType Type() const { return type; }
    Vulkan::BufferUsage TransferUsage() const { return usage; }
    size_t ItemsCount() const { return item_count; }
    size_t BufferLength() const { return size; }
    size_t ItemTypeSize() const { return item_count == 0 ? 0 : size / item_count; }
    VkBuffer GetBuffer() const { return buffer; }
    static void MoveData(VkDevice device, VkCommandPool command_pool, VkQueue queue, std::shared_ptr<IBuffer> src, std::shared_ptr<IBuffer> dst)
    {
      VkFence move_sync = VK_NULL_HANDLE;

      if (device == VK_NULL_HANDLE)
        throw std::runtime_error("Device is nullptr");
      if (command_pool == VK_NULL_HANDLE)
        throw std::runtime_error("Command_pool is nullptr");
      if (queue == VK_NULL_HANDLE)
        throw std::runtime_error("queue is nullptr");
      if (src.get() == nullptr || dst.get() == nullptr)
        throw std::runtime_error("Buffer is nullptr");

      auto command_buffers = Vulkan::Supply::CreateCommandBuffers(device, 
                                                        command_pool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
      if (command_buffers.empty())
        throw std::runtime_error("No allocated buffers");

      VkCommandBufferBeginInfo begin_info = {};
      begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
      begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; 
#ifdef DEBUG
      std::cout << "Src:" << src->BufferLength() << " Dst:" << dst->BufferLength() << std::endl;
#endif
      VkBufferCopy copy_region = {};
      copy_region.srcOffset = 0;
      copy_region.dstOffset = 0;
      copy_region.size = std::min(src->BufferLength(), dst->BufferLength());

      VkSubmitInfo submit_info = {};
      submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit_info.commandBufferCount = 1;
      submit_info.pCommandBuffers = &command_buffers[0];

      VkFenceCreateInfo fence_info = {};
      fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fence_info.flags = 0;

      vkCreateFence(device, &fence_info, nullptr, &move_sync);
  
      vkBeginCommandBuffer(command_buffers[0], &begin_info);
      vkCmdCopyBuffer(command_buffers[0], src->GetBuffer(), dst->GetBuffer(), 1, &copy_region);
      vkEndCommandBuffer(command_buffers[0]);

      vkQueueSubmit(queue, 1, &submit_info, move_sync);
      vkWaitForFences(device, 1, &move_sync, VK_TRUE, UINT64_MAX);

      if (device != VK_NULL_HANDLE && move_sync != VK_NULL_HANDLE)
        vkDestroyFence(device, move_sync, nullptr);
    } 

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
    Buffer(std::shared_ptr<Vulkan::Device> dev, Vulkan::StorageType buffer_type, Vulkan::BufferUsage buffer_usage);
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

  template <class T>
  Buffer<T>::Buffer(std::shared_ptr<Vulkan::Device> dev, Vulkan::StorageType buffer_type, Vulkan::BufferUsage buffer_usage)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE || dev->GetPhysicalDevice() == VK_NULL_HANDLE)
      throw std::runtime_error("Invalid device pointer.");

    if (buffer_type == Vulkan::StorageType::None)
      throw std::runtime_error("You should not use StorageType::None");

    device = dev;
    type = buffer_type;
    usage = buffer_usage;
  }

  template <class T>
  Buffer<T>::Buffer(const Buffer<T> &obj)
  {
    device = obj.device;
    type = obj.type;
    usage = obj.usage;
    if (obj.buffer != VK_NULL_HANDLE)
    {
      if (obj.usage == Vulkan::BufferUsage::Transfer_src)
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
    if (type != obj.type || usage != obj.usage)
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
    usage = obj.usage;

    if (obj.usage == Vulkan::BufferUsage::Transfer_src)
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
    buffer_create_info.usage = (VkBufferUsageFlags) usage | (VkBufferUsageFlags) type;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateBuffer(device->GetDevice(), &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
      throw std::runtime_error("Can't create Buffer.");

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        
    switch (usage)
    {
      case Vulkan::BufferUsage::Transfer_dst:
        flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        break;
      case Vulkan::BufferUsage::Transfer_src:
        flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        break;
    }
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

    if (usage == Vulkan::BufferUsage::Transfer_src)
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
    if (memory == VK_NULL_HANDLE || item_count == 0 || usage == Vulkan::BufferUsage::Transfer_dst)
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
    void *payload;
    if (vkMapMemory(device->GetDevice(), memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");
    
    std::copy((T *) payload, &((T*) payload)[1], &result);
    vkUnmapMemory(device->GetDevice(), memory);

    return result;
  }
}

#endif