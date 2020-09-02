#ifndef __CPU_NW_VULKAN_BUFFERARRAY_H
#define __CPU_NW_VULKAN_BUFFERARRAY_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <memory>
#include <cmath>
#include <cstring>
#include <tuple>
#include <mutex>
#include <type_traits>

#include "Device.h"
#include "Buffer.h"

namespace Vulkan
{
  struct virtual_buffer_t
  {
    uint32_t size;
    uint32_t offset;
    VkBufferView view;
    VkFormat format = VK_FORMAT_UNDEFINED;
  };
  
  struct buffer_t
  {
    VkBuffer buffer = VK_NULL_HANDLE;
    Vulkan::StorageType type = StorageType::None;
    size_t virt_buffer_align = 0;    
    VkDeviceMemory memory = VK_NULL_HANDLE;
    std::pair<uint32_t, uint32_t> memory_size;
    std::vector<virtual_buffer_t> virt_buffers;
    HostVisibleMemory access = HostVisibleMemory::HostVisible;
  };

  class BufferArray
  {
  private:
    std::shared_ptr<Vulkan::Device> device;
    std::vector<buffer_t> buffers;
    std::mutex buffers_mutex;
    
    size_t GetAligned(const size_t value, const size_t align) const { return (std::ceil(value / (float) align) * align); }
    VkBufferView CreateBufferView(const VkBuffer buffer, const VkFormat format, const uint32_t offset, const uint32_t size);
    void Destroy();
  public:
    BufferArray() = delete;
    BufferArray(std::shared_ptr<Vulkan::Device> dev);
    BufferArray(const BufferArray &obj) = delete;
    BufferArray& operator= (const BufferArray &obj) = delete;
    size_t Count() const { return buffers.size(); }
    size_t VirtualBuffersCount(const size_t index);
    size_t BufferSize(const size_t index);
    Vulkan::StorageType BufferType(const size_t index);
    void DeclareBuffer(const size_t memory_size, const Vulkan::HostVisibleMemory memory_access, const Vulkan::StorageType buffer_type, const VkFormat buffer_format = VK_FORMAT_UNDEFINED);
    void DeclareVirtualBuffer(const size_t buffer_index, const size_t offset, const size_t size, const VkFormat buffer_format = VK_FORMAT_UNDEFINED);
    void ClearVirtualBuffers(const size_t buffer_index);
    std::pair<VkBuffer, VkBufferView> GetWholeBuffer(const size_t buffer_index);
    std::pair<VkBuffer, virtual_buffer_t> GetVirtualBuffer(const size_t buffer_index, const size_t virtual_buffer_index);

    template <typename T> bool TrySetValue(const size_t index, const size_t virtual_buffer_index, const std::vector<T> &value);
    template <typename T> bool TryGetValue(const size_t index, const size_t virtual_buffer_index, std::vector<T> &out_value);
    template <typename T> size_t ElementsInBuffer(const size_t index);
    template <typename T> size_t ElementsInVirtualBuffer(const size_t index, const size_t virtual_buffer_index);
    template <typename T> void SplitBufferOnEqualVirtualBuffers(const size_t index, const VkFormat buffer_format = VK_FORMAT_UNDEFINED);

    size_t CalculateBufferSize(const size_t virtual_buffer_size, const size_t virtual_buffers_count, const Vulkan::StorageType buffer_type);
    ~BufferArray();
  };

  template <typename T> 
  bool BufferArray::TrySetValue(const size_t index, const size_t virtual_buffer_index, const std::vector<T> &value)
  {
    std::lock_guard<std::mutex> lock(buffers_mutex);

    if (index >= buffers.size())
      throw std::runtime_error("Index is out of bounds");

    if (virtual_buffer_index >= buffers[index].virt_buffers.size() - 1)
      throw std::runtime_error("Virtual buffer index is out of bounds");

    if (buffers[index].virt_buffers[virtual_buffer_index + 1].size < value.size() * sizeof(T))
      throw std::runtime_error("Data is too big for a virtual buffer.");

    if (value.empty() || buffers[index].access == HostVisibleMemory::HostInvisible)
      return false;

    void *payload = nullptr;

    if (vkMapMemory(device->GetDevice(), buffers[index].memory, 
                    buffers[index].virt_buffers[virtual_buffer_index + 1].offset, 
                    buffers[index].virt_buffers[virtual_buffer_index + 1].size, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");

    std::copy(value.begin(), value.end(), (T*) payload);

    vkUnmapMemory(device->GetDevice(), buffers[index].memory);

    return true;
  }

  template <typename T> 
  bool BufferArray::TryGetValue(const size_t index, const size_t virtual_buffer_index, std::vector<T> &out_value)
  {
    std::lock_guard<std::mutex> lock(buffers_mutex);

    if (index >= buffers.size())
      throw std::runtime_error("Index is out of bounds");

    if (virtual_buffer_index >= buffers[index].virt_buffers.size() - 1)
      throw std::runtime_error("Virtual buffer index is out of bounds");

    if (buffers[index].access == HostVisibleMemory::HostInvisible)
      return false;

    void *payload = nullptr;

    out_value.resize(std::floor(buffers[index].virt_buffers[virtual_buffer_index + 1].size / sizeof(T)));

    if (vkMapMemory(device->GetDevice(), buffers[index].memory, 
                    buffers[index].virt_buffers[virtual_buffer_index + 1].offset, 
                    buffers[index].virt_buffers[virtual_buffer_index + 1].size, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");

    std::copy((T*) payload, &((T*) payload)[out_value.size()], out_value.begin());

    vkUnmapMemory(device->GetDevice(), buffers[index].memory);

    return true;
  }

  template <typename T> 
  size_t BufferArray::ElementsInBuffer(const size_t index)
  {
    std::lock_guard<std::mutex> lock(buffers_mutex);

    if (index >= buffers.size())
      throw std::runtime_error("Index is out of bounds");

    return std::floor(buffers[index].memory_size.first / sizeof(T));
  }

  template <typename T> 
  size_t BufferArray::ElementsInVirtualBuffer(const size_t index, const size_t virtual_buffer_index)
  {
    std::lock_guard<std::mutex> lock(buffers_mutex);

    if (index >= buffers.size())
      throw std::runtime_error("Index is out of bounds");

    if (virtual_buffer_index >= buffers[index].virt_buffers.size() - 1)
      throw std::runtime_error("Virtual buffer index is out of bounds");

    return std::floor(buffers[index].virt_buffers[virtual_buffer_index + 1].size / sizeof(T));
  }

  template <typename T> 
  void BufferArray::SplitBufferOnEqualVirtualBuffers(const size_t index, const VkFormat buffer_format)
  {
    std::lock_guard<std::mutex> lock(buffers_mutex);

    if (index >= buffers.size())
      throw std::runtime_error("Index is out of bounds");

    for (size_t i = 1 ; i < buffers[index].virt_buffers.size(); ++i)
      if (buffers[index].virt_buffers[i].view != VK_NULL_HANDLE)
        vkDestroyBufferView(device->GetDevice(), buffers[index].virt_buffers[i].view, nullptr);
    buffers[index].virt_buffers.resize(1);

    uint32_t s = GetAligned(sizeof(T), buffers[index].virt_buffer_align);
    size_t elems = std::floor(buffers[index].memory_size.first / s);
    uint32_t offset = 0;
    
    for (size_t i = 0; i < elems; ++i)
    {
      offset = GetAligned(i * s, buffers[index].virt_buffer_align);

      buffers[index].virt_buffers.push_back(
      {
        s,
        offset,
        buffer_format != VK_FORMAT_UNDEFINED ? CreateBufferView(buffers[index].buffer, buffer_format, offset, s) : VK_NULL_HANDLE, 
        buffer_format
      });
    }
  }
}

#endif