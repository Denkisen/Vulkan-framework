#ifndef __CPU_NW_VULKAN_TRANSFERARRAY_H
#define __CPU_NW_VULKAN_TRANSFERARRAY_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <memory>
#include <optional>

#include "IStorage.h"
#include "Device.h"

namespace Vulkan
{
  template <class T> class TransferArray : public IStorage
  {
  private:
    std::vector<T> data;
    VkFence move_sync = VK_NULL_HANDLE;
    void CreateSrc();
    void CreateDst();
    void Create(std::shared_ptr<Vulkan::Device> dev, T *data, std::size_t len, Vulkan::StorageType storage_type);
  public:
    TransferArray() = delete;
    TransferArray(std::shared_ptr<Vulkan::Device> dev, Vulkan::StorageType storage_type);
    TransferArray(std::shared_ptr<Vulkan::Device> dev, std::vector<T> &data, Vulkan::StorageType storage_type);
    TransferArray(std::shared_ptr<Vulkan::Device> dev, T *data, std::size_t len, Vulkan::StorageType storage_type);
    TransferArray(const TransferArray<T> &array);
    std::vector<T> Extract() const;
    TransferArray<T>& operator= (const TransferArray<T> &obj);
    TransferArray<T>& operator= (const std::vector<T> &obj);
    void MoveData(VkCommandPool command_pool);
    VkBuffer GetBuffer() { return IStorage::GetDstBuffer(); }
    ~TransferArray()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
      if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE && move_sync != VK_NULL_HANDLE)
        vkDestroyFence(device->GetDevice(), move_sync, nullptr);
    }
  };
}

namespace Vulkan
{
  template <class T>
  void TransferArray<T>::CreateSrc()
  {
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = buffer_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device->GetDevice(), &buffer_create_info, nullptr, &src_buffer) != VK_SUCCESS)
      throw std::runtime_error("Can't create Buffer.");

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    auto memory_type_index = Vulkan::Supply::GetMemoryTypeIndex(device->GetDevice(), device->GetPhysicalDevice(), src_buffer, buffer_size, flags);
    if (!memory_type_index.has_value())
      throw std::runtime_error("Out of memory.");
    
    VkMemoryAllocateInfo memory_allocate_info = 
    {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      0,
      buffer_size,
      (uint32_t) memory_type_index.value()
    };

    if (vkAllocateMemory(device->GetDevice(), &memory_allocate_info, nullptr, &src_buffer_memory) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate memory");

    if (vkBindBufferMemory(device->GetDevice(), src_buffer, src_buffer_memory, 0) != VK_SUCCESS)
      throw std::runtime_error("Can't bind memory to buffer.");
  }

  template <class T>
  void TransferArray<T>::CreateDst()
  {
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = buffer_size;
    buffer_create_info.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | Vulkan::Supply::StorageTypeToBufferUsageFlags(this->type);
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(device->GetDevice(), &buffer_create_info, nullptr, &dst_buffer) != VK_SUCCESS)
      throw std::runtime_error("Can't create Buffer.");

    VkMemoryPropertyFlags flags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    auto memory_type_index = Vulkan::Supply::GetMemoryTypeIndex(device->GetDevice(), device->GetPhysicalDevice(), dst_buffer, buffer_size, flags);
    if (!memory_type_index.has_value())
      throw std::runtime_error("Out of memory.");

    VkMemoryAllocateInfo memory_allocate_info = 
    {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      0,
      buffer_size,
      (uint32_t) memory_type_index.value()
    };

    if (vkAllocateMemory(device->GetDevice(), &memory_allocate_info, nullptr, &dst_buffer_memory) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate memory");

    if (vkBindBufferMemory(device->GetDevice(), dst_buffer, dst_buffer_memory, 0) != VK_SUCCESS)
      throw std::runtime_error("Can't bind memory to buffer.");
  }

  template <class T>
  void TransferArray<T>::Create(std::shared_ptr<Vulkan::Device> dev, T *data, std::size_t len, Vulkan::StorageType storage_type)
  {
    if (len == 0 || data == nullptr || dev.get() == nullptr)
      throw std::runtime_error("Data array is empty.");

    buffer_size = len * sizeof(T);
    elements_count = len;
    device = dev;
    this->type = storage_type;

    CreateSrc();
    CreateDst();

    this->data.resize(std::ceil((double) buffer_size / (sizeof(T))));
    std::copy(data, data + len, this->data.begin());

    void *payload = nullptr;
    if (vkMapMemory(dev->GetDevice(), src_buffer_memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");
    
    std::memcpy(payload, this->data.data(), buffer_size);
    vkUnmapMemory(dev->GetDevice(), src_buffer_memory);

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = 0;

    vkCreateFence(device->GetDevice(), &fence_info, nullptr, &move_sync);
  }

  template <class T>
  TransferArray<T>::TransferArray(std::shared_ptr<Vulkan::Device> dev, Vulkan::StorageType storage_type)
  {
    std::vector<T> tmp(64);
    Create(dev, tmp.data(), tmp.size(), storage_type);
  }

  template <class T>
  TransferArray<T>::TransferArray(std::shared_ptr<Vulkan::Device> dev, std::vector<T> &data, Vulkan::StorageType storage_type)
  {
    if (data.size() == 0)
      throw std::runtime_error("Data array is empty.");
    
    Create(dev, data.data(), data.size(), storage_type);
  }

  template <class T>
  TransferArray<T>::TransferArray(std::shared_ptr<Vulkan::Device> dev, T *data, std::size_t len, Vulkan::StorageType storage_type)
  {
    Create(dev, data, len, storage_type);
  }

  template <class T>
  TransferArray<T>::TransferArray(const TransferArray<T> &array)
  {
    std::vector<T> data(array.Extract());
    Create(array.device, data.data(), data.size(), array.type);
  }

  template <class T>
  std::vector<T> TransferArray<T>::Extract() const
  {
    void *payload = nullptr;
    if (vkMapMemory(device->GetDevice(), src_buffer_memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");
    
    std::vector<T> data(buffer_size / sizeof(T));
    std::copy((T *)payload, &((T *)payload)[data.size()], data.begin());
    vkUnmapMemory(device->GetDevice(), src_buffer_memory);

    return data;
  }

  template <class T>
  TransferArray<T>& TransferArray<T>::operator= (const TransferArray<T> &obj)
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      if (src_buffer != VK_NULL_HANDLE)
      {
        vkFreeMemory(device->GetDevice(), src_buffer_memory, nullptr);
        vkDestroyBuffer(device->GetDevice(), src_buffer, nullptr);
      }

      if (dst_buffer != VK_NULL_HANDLE)
      {
        vkFreeMemory(device->GetDevice(), dst_buffer_memory, nullptr);
        vkDestroyBuffer(device->GetDevice(), dst_buffer, nullptr);
      }
      device.reset();
    }

    std::vector<T> data(obj.Extract());
    Create(obj.device, data.data(), data.size(), obj.type);

    return *this;
  }

  template <class T>
  TransferArray<T>& TransferArray<T>::operator= (const std::vector<T> &obj)
  {
    if (obj.size() > data.size())
    {
      if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
      {
        if (src_buffer != VK_NULL_HANDLE)
        {
          vkFreeMemory(device->GetDevice(), src_buffer_memory, nullptr);
          vkDestroyBuffer(device->GetDevice(), src_buffer, nullptr);
        }

        if (dst_buffer != VK_NULL_HANDLE)
        {
          vkFreeMemory(device->GetDevice(), dst_buffer_memory, nullptr);
          vkDestroyBuffer(device->GetDevice(), dst_buffer, nullptr);
        }

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
  void TransferArray<T>::MoveData(VkCommandPool command_pool)
  {
    auto command_buffers = Vulkan::Supply::CreateCommandBuffers(device->GetDevice(), 
                                                        command_pool, 1, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    if (command_buffers.empty())
      throw std::runtime_error("No allocated buffers");

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; 

    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = buffer_size;

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffers[0];
  
    vkBeginCommandBuffer(command_buffers[0], &begin_info);
    vkCmdCopyBuffer(command_buffers[0], src_buffer, dst_buffer, 1, &copy_region);
    vkEndCommandBuffer(command_buffers[0]);

    vkQueueSubmit(device->GetGraphicQueue(), 1, &submit_info, move_sync);
    vkWaitForFences(device->GetDevice(), 1, &move_sync, VK_TRUE, UINT64_MAX);
  }
}

#endif