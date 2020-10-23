#ifndef __VULKAN_ARRAY_H
#define __VULKAN_ARRAY_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <cmath>
#include <iostream>
#include <mutex>
#include <cstring>
#include <tuple>

#include "Device.h"
#include "Logger.h"

namespace Vulkan
{
  enum class StorageType
  {
    Storage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
    Uniform = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    Vertex = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
    Index = VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
    TexelStorage = VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT,
    TexelUniform = VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT
  };

  enum class HostVisibleMemory
  {
    HostVisible = (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT),
    HostInvisible = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
  };

  struct sub_buffer_t
  {
    VkDeviceSize size = 0;
    VkDeviceSize offset = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
  private:
    friend class Array_impl;
    VkBufferView view = VK_NULL_HANDLE;
  };

  struct buffer_t
  {
    Vulkan::StorageType type = Vulkan::StorageType::Storage;
    VkDeviceSize sub_buffer_align = 16;
    VkDeviceSize size = 0;
    VkDeviceSize offset = 0;
    std::vector<sub_buffer_t> sub_buffers;
  private:
    friend class Array_impl;
    VkBuffer buffer = VK_NULL_HANDLE;
  };

  class BufferConfig
  {
  private:
    friend class Array_impl;

    Vulkan::StorageType buffer_type = Vulkan::StorageType::Storage;
    std::vector<std::tuple<VkDeviceSize, VkDeviceSize, VkFormat>> sizes;
  public:
    BufferConfig() = default;
    ~BufferConfig() = default;

    BufferConfig &AddSubBuffer(const VkDeviceSize length, const VkDeviceSize item_size = 1, const VkFormat format = VK_FORMAT_UNDEFINED) 
    { 
      sizes.push_back({length, item_size, format}); return *this; 
    }
    BufferConfig &AddSubBufferRange(const VkDeviceSize buffers_count, const VkDeviceSize length, 
                                    const VkDeviceSize item_size = 1, const VkFormat format = VK_FORMAT_UNDEFINED) 
    {
      for (VkDeviceSize i = 0; i < buffers_count; ++i) sizes.push_back({length, item_size, format});
      return *this;
    }
    BufferConfig &SetType(const Vulkan::StorageType type) { buffer_type = type; return *this; }
  };

  class Array_impl
  {
  public:
    Array_impl() = delete;
    Array_impl(const Array_impl &obj) = delete;
    Array_impl(Array_impl &&obj) = delete;
    Array_impl &operator=(const Array_impl &obj) = delete;
    Array_impl &operator=(Array_impl &&obj) = delete;
    ~Array_impl();
  private:
    friend class Array;
    Array_impl(std::shared_ptr<Vulkan::Device> dev);
    VkDeviceSize Align(const VkDeviceSize value, const VkDeviceSize align);
    VkBufferView CreateBufferView(const VkBuffer buffer, const VkFormat format, const VkDeviceSize offset, const VkDeviceSize size);
    void Abort(std::vector<buffer_t> &buffs);

    VkResult StartConfig(const HostVisibleMemory val = HostVisibleMemory::HostVisible);
    VkResult AddBuffer(const BufferConfig params);
    VkResult EndConfig();
    void Clear();
    size_t Count() { std::lock_guard lock(buffers_mutex); return buffers.size(); }
    size_t SubBuffsCount(const size_t index) { std::lock_guard lock(buffers_mutex); return index < buffers.size() ? buffers[index].sub_buffers.size() : 0;}
    buffer_t GetInfo(const size_t index) { std::lock_guard lock(buffers_mutex); return index < buffers.size() ? buffers[index] : buffer_t(); }
    template <typename T>
    bool GetBufferData(const size_t index, std::vector<T> &result);
    template <typename T>
    bool GetSubBufferData(const size_t index, const size_t sub_index, std::vector<T> &result);
    template <typename T>
    bool SetBufferData(const size_t index, const std::vector<T> &data);
    template <typename T>
    bool SetSubBufferData(const size_t index, const size_t sub_index, const std::vector<T> &data);
    void UseChunkedMapping(const bool val) { use_chunked_mapping = val; }

    std::shared_ptr<Vulkan::Device> device;
    std::vector<buffer_t> buffers;
    HostVisibleMemory access = HostVisibleMemory::HostVisible;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    VkDeviceSize align = 256;
    std::vector<BufferConfig> prebuild_config;
    HostVisibleMemory prebuild_access_config = HostVisibleMemory::HostVisible;

    bool use_chunked_mapping = false;

    std::mutex buffers_mutex;
    std::mutex config_mutex;
  };

  class Array
  {
  private:
    std::unique_ptr<Array_impl> impl;
  public:
    Array() = delete;
    Array(const Array &obj);
    Array(Array &&obj) noexcept : impl(std::move(obj.impl)) {};
    Array(std::shared_ptr<Vulkan::Device> dev) : impl(std::unique_ptr<Array_impl>(new Array_impl(dev))) {};
    Array &operator=(const Array &obj);
    Array &operator=(Array &&obj) noexcept;
    void swap(Array &obj) noexcept;
    VkResult StartConfig(const HostVisibleMemory val = HostVisibleMemory::HostVisible) { return impl->StartConfig(val); }
    VkResult AddBuffer(const BufferConfig params) { return impl->AddBuffer(params); }
    VkResult EndConfig() {return impl->EndConfig(); }
    size_t Count() { return impl->Count(); }
    size_t SubBuffsCount(const size_t index) { return impl->SubBuffsCount(index); }
    buffer_t GetInfo(const size_t index) { return impl->GetInfo(index); }
    template <typename T>
    bool GetBufferData(const size_t index, std::vector<T> &result) { return impl->GetBufferData(index, result); }
    template <typename T>
    bool GetSubBufferData(const size_t index, const size_t sub_index, std::vector<T> &result) { return impl->GetSubBufferData(index, sub_index, result); }
    template <typename T>
    bool SetBufferData(const size_t index, const std::vector<T> &data) { return impl->SetBufferData(index, data); }
    template <typename T>
    bool SetSubBufferData(const size_t index, const size_t sub_index, const std::vector<T> &data) { return impl->SetSubBufferData(index, sub_index, data); }
#ifdef DEBUG
    void UseChunkedMapping(const bool val) { impl->UseChunkedMapping(val); }
#endif
  };

  void swap(Array &lhs, Array &rhs) noexcept;

  template <typename T>
  bool Array_impl::GetBufferData(const size_t index, std::vector<T> &result)
  {
    std::lock_guard lock(buffers_mutex);
    if (index >= buffers.size())
    {
      Logger::EchoError("Index is out of range", __func__);
      return false;
    }

    if (memory == VK_NULL_HANDLE)
    {
      Logger::EchoError("Memory is NULL", __func__);
      return false;
    }

    if (buffers[index].buffer == VK_NULL_HANDLE)
    {
      Logger::EchoError("Buffer is NULL", __func__);
      return false;
    }

    std::vector<T> tmp(std::ceil(buffers[index].size / sizeof(T)));
    void *payload = nullptr;

    auto er = vkMapMemory(device->GetDevice(), memory, buffers[index].offset, buffers[index].size, 0, &payload);

    if (er != VK_SUCCESS && er != VK_ERROR_MEMORY_MAP_FAILED)
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
      return false;
    }

    if (use_chunked_mapping && er == VK_SUCCESS)
      vkUnmapMemory(device->GetDevice(), memory);

    if (!use_chunked_mapping && er == VK_SUCCESS)
    {
      std::memcpy(tmp.data(), payload, buffers[index].size);
      vkUnmapMemory(device->GetDevice(), memory);
      result.swap(tmp);
    }
    else
    {
      VkDeviceSize offset = buffers[index].offset;
      for (VkDeviceSize i = 0; i < buffers[index].size / align; ++i)
      {
        er = vkMapMemory(device->GetDevice(), memory, offset, align, 0, &payload);
        if (er != VK_SUCCESS)
        {
          Logger::EchoError("Can't map memory.", __func__);
          return false;
        }

        std::memcpy(((uint8_t *) tmp.data()) + (i * align), payload, align);
        vkUnmapMemory(device->GetDevice(), memory);
        offset += align;
      }
      result.swap(tmp);
    }

    return true;
  }

  template <typename T>
  bool Array_impl::GetSubBufferData(const size_t index, const size_t sub_index, std::vector<T> &result)
  {
    std::lock_guard lock(buffers_mutex);
    if (index >= buffers.size())
    {
      Logger::EchoError("Index is out of range", __func__);
      return false;
    }

    if (sub_index >= buffers[index].sub_buffers.size())
    {
      Logger::EchoError("Sub index is out of range", __func__);
      return false;
    }

    if (memory == VK_NULL_HANDLE)
    {
      Logger::EchoError("Memory is NULL", __func__);
      return false;
    }

    if (buffers[index].buffer == VK_NULL_HANDLE)
    {
      Logger::EchoError("Buffer is NULL", __func__);
      return false;
    }

    std::vector<T> tmp(std::ceil(buffers[index].sub_buffers[sub_index].size / sizeof(T)));
    void *payload = nullptr;

    auto main_offset = buffers[index].offset + buffers[index].sub_buffers[sub_index].offset;
    auto er = vkMapMemory(device->GetDevice(), memory, main_offset, buffers[index].sub_buffers[sub_index].size, 0, &payload);

    if (er != VK_SUCCESS && er != VK_ERROR_MEMORY_MAP_FAILED)
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
      return false;
    }

    if (use_chunked_mapping && er == VK_SUCCESS)
      vkUnmapMemory(device->GetDevice(), memory);

    if (!use_chunked_mapping && er == VK_SUCCESS)
    {
      std::memcpy(tmp.data(), payload, buffers[index].sub_buffers[sub_index].size);
      vkUnmapMemory(device->GetDevice(), memory);
      result.swap(tmp);
    }
    else
    {
      VkDeviceSize offset = main_offset;
      for (VkDeviceSize i = 0; i < buffers[index].sub_buffers[sub_index].size / buffers[index].sub_buffer_align; ++i)
      {
        er = vkMapMemory(device->GetDevice(), memory, offset, buffers[index].sub_buffer_align, 0, &payload);
        if (er != VK_SUCCESS)
        {
          Logger::EchoError("Can't map memory.", __func__);
          return false;
        }

        std::memcpy(((uint8_t *) tmp.data()) + (i * buffers[index].sub_buffer_align), payload, buffers[index].sub_buffer_align);
        vkUnmapMemory(device->GetDevice(), memory);
        offset += buffers[index].sub_buffer_align;
      }
      result.swap(tmp);
    }

    return true;
  }

  template <typename T>
  bool Array_impl::SetBufferData(const size_t index, const std::vector<T> &data)
  {
    std::lock_guard lock(buffers_mutex);
    if (index >= buffers.size())
    {
      Logger::EchoError("Index is out of range", __func__);
      return false;
    }

    if (memory == VK_NULL_HANDLE)
    {
      Logger::EchoError("Memory is NULL", __func__);
      return false;
    }

    if (buffers[index].buffer == VK_NULL_HANDLE)
    {
      Logger::EchoError("Buffer is NULL", __func__);
      return false;
    }

    if (data.size() * sizeof(T) > buffers[index].size)
    {
      Logger::EchoWarning("Data is too big for buffer", __func__);
    }

    void *payload = nullptr;

    auto er = vkMapMemory(device->GetDevice(), memory, buffers[index].offset, buffers[index].size, 0, &payload);

    if (er != VK_SUCCESS && er != VK_ERROR_MEMORY_MAP_FAILED)
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
      return false;
    }

    if (use_chunked_mapping && er == VK_SUCCESS)
      vkUnmapMemory(device->GetDevice(), memory);

    if (!use_chunked_mapping && er == VK_SUCCESS)
    {
      std::memcpy(payload, data.data(), std::min(buffers[index].size, data.size() * sizeof(T)));
      vkUnmapMemory(device->GetDevice(), memory);
    }
    else
    {
      VkDeviceSize offset = buffers[index].offset;
      for (VkDeviceSize i = 0; i < std::min(buffers[index].size, data.size() * sizeof(T)) / align; ++i)
      {
        er = vkMapMemory(device->GetDevice(), memory, offset, align, 0, &payload);
        if (er != VK_SUCCESS)
        {
          Logger::EchoError("Can't map memory.", __func__);
          return false;
        }

        std::memcpy(payload, ((uint8_t *) data.data()) + (i * align), align);
        vkUnmapMemory(device->GetDevice(), memory);
        offset += align;
      }
    }

    return true;
  }

  template <typename T>
  bool Array_impl::SetSubBufferData(const size_t index, const size_t sub_index, const std::vector<T> &data)
  {
    std::lock_guard lock(buffers_mutex);
    if (index >= buffers.size())
    {
      Logger::EchoError("Index is out of range", __func__);
      return false;
    }

    if (index >= buffers[index].sub_buffers.size())
    {
      Logger::EchoError("Sub index is out of range", __func__);
      return false;
    }

    if (memory == VK_NULL_HANDLE)
    {
      Logger::EchoError("Memory is NULL", __func__);
      return false;
    }

    if (buffers[index].buffer == VK_NULL_HANDLE)
    {
      Logger::EchoError("Buffer is NULL", __func__);
      return false;
    }

    if (data.size() * sizeof(T) > buffers[index].sub_buffers[sub_index].size)
    {
      Logger::EchoWarning("Data is too big for buffer", __func__);
    }

    void *payload = nullptr;

    auto main_offset = buffers[index].offset + buffers[index].sub_buffers[sub_index].offset;
    auto er = vkMapMemory(device->GetDevice(), memory, main_offset, buffers[index].sub_buffers[sub_index].size, 0, &payload);

    if (er != VK_SUCCESS && er != VK_ERROR_MEMORY_MAP_FAILED)
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
      return false;
    }

    if (use_chunked_mapping && er == VK_SUCCESS)
      vkUnmapMemory(device->GetDevice(), memory);

    if (!use_chunked_mapping && er == VK_SUCCESS)
    {
      std::memcpy(payload, data.data(), std::min(buffers[index].sub_buffers[sub_index].size, data.size() * sizeof(T)));
      vkUnmapMemory(device->GetDevice(), memory);
    }
    else
    {
      VkDeviceSize offset = main_offset;
      for (VkDeviceSize i = 0; i < std::min(buffers[index].sub_buffers[sub_index].size, data.size() * sizeof(T)) / buffers[index].sub_buffer_align; ++i)
      {
        er = vkMapMemory(device->GetDevice(), memory, offset, buffers[index].sub_buffer_align, 0, &payload);
        if (er != VK_SUCCESS)
        {
          Logger::EchoError("Can't map memory.", __func__);
          return false;
        }

        std::memcpy(payload, ((uint8_t *) data.data()) + (i * buffers[index].sub_buffer_align), buffers[index].sub_buffer_align);
        vkUnmapMemory(device->GetDevice(), memory);
        offset += buffers[index].sub_buffer_align;
      }
    }

    return true;
  }
}

#endif