#ifndef __VULKAN_STORAGEARRAY_H
#define __VULKAN_STORAGEARRAY_H

#include "Logger.h"
#include "Misc.h"
#include "Device.h"

#include <algorithm>
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <cmath>
#include <mutex>
#include <cstring>
#include <tuple>

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
    VkDeviceSize elements = 0;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkBufferView view = VK_NULL_HANDLE;
    std::string tag = "";
  };

  struct buffer_t
  {
    StorageType type = StorageType::Storage;
    VkDeviceSize sub_buffer_align = 16;
    VkDeviceSize size = 0;
    VkDeviceSize offset = 0;
    std::vector<sub_buffer_t> sub_buffers;
    VkBuffer buffer = VK_NULL_HANDLE;
  };

  class BufferConfig
  {
  private:
    friend class StorageArray_impl;

    StorageType buffer_type = StorageType::Storage;
    std::vector<std::tuple<VkDeviceSize, VkDeviceSize, VkFormat>> sizes;
  public:
    BufferConfig() = default;
    ~BufferConfig() noexcept = default;

    BufferConfig &AddSubBuffer(const VkDeviceSize length, const VkDeviceSize item_size = 1, const VkFormat format = VK_FORMAT_UNDEFINED)
    { 
      sizes.push_back({length, item_size, format});
      return *this; 
    }
    template <typename T> BufferConfig &AddSubBuffer(const std::vector<T> data, const VkFormat format = VK_FORMAT_UNDEFINED)
    {
      sizes.push_back({data.size(), sizeof(T), format});
      return *this;
    }
    BufferConfig &AddSubBufferRange(const VkDeviceSize buffers_count, const VkDeviceSize length, 
                                    const VkDeviceSize item_size = 1, const VkFormat format = VK_FORMAT_UNDEFINED)
    {
      for (VkDeviceSize i = 0; i < buffers_count; ++i) sizes.push_back({length, item_size, format});
      return *this;
    }
    template <typename T> BufferConfig &AddSubBufferRange(const VkDeviceSize buffers_count, const std::vector<T> data, const VkFormat format = VK_FORMAT_UNDEFINED)
    {
      for (VkDeviceSize i = 0; i < buffers_count; ++i) sizes.push_back({data.size(), sizeof(T), format});
      return *this;
    }
    BufferConfig &SetType(const StorageType type) noexcept { buffer_type = type; return *this; }
  };

  class StorageArray_impl
  {
  public:
    StorageArray_impl() = delete;
    StorageArray_impl(const StorageArray_impl &obj) = delete;
    StorageArray_impl(StorageArray_impl &&obj) = delete;
    StorageArray_impl &operator=(const StorageArray_impl &obj) = delete;
    StorageArray_impl &operator=(StorageArray_impl &&obj) = delete;
    ~StorageArray_impl() noexcept;
  private:
    friend class StorageArray;
    std::shared_ptr<Device> device;
    std::vector<buffer_t> buffers;
    HostVisibleMemory access = HostVisibleMemory::HostVisible;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    VkDeviceSize size = 0;
    VkDeviceSize align = 256;
    std::vector<BufferConfig> prebuild_config;
    HostVisibleMemory prebuild_access_config = HostVisibleMemory::HostVisible;

    StorageArray_impl(std::shared_ptr<Device> dev);
    VkBufferView CreateBufferView(const VkBuffer buffer, const VkFormat format, const VkDeviceSize offset, const VkDeviceSize size);
    void Abort(std::vector<buffer_t> &buffs) const noexcept;

    VkResult StartConfig(const HostVisibleMemory val = HostVisibleMemory::HostVisible) noexcept;
    VkResult AddBuffer(const BufferConfig params);
    VkResult EndConfig();
    void Clear() noexcept;
    size_t Count() const noexcept { return buffers.size(); }
    HostVisibleMemory GetMemoryAccess() const noexcept { return access; }
    size_t SubBuffsCount(const size_t index) const noexcept { return index < buffers.size() ? buffers[index].sub_buffers.size() : 0;}
    buffer_t GetInfo(const size_t index) const { return index < buffers.size() ? buffers[index] : buffer_t(); }
    std::shared_ptr<Device> GetDevice() const noexcept { return device; }
    template <typename T>
    VkResult GetBufferData(const size_t index, std::vector<T> &result) const;
    template <typename T>
    VkResult GetSubBufferData(const size_t index, const size_t sub_index, std::vector<T> &result) const;
    template <typename T>
    VkResult SetBufferData(const size_t index, const std::vector<T> &data);
    template <typename T>
    VkResult SetSubBufferData(const size_t index, const size_t sub_index, const std::vector<T> &data);
  };

  class StorageArray
  {
  private:
    std::unique_ptr<StorageArray_impl> impl;
  public:
    StorageArray() = delete;
    StorageArray(const StorageArray &obj);
    StorageArray(StorageArray &&obj) noexcept : impl(std::move(obj.impl)) {};
    StorageArray(std::shared_ptr<Device> dev) : impl(std::unique_ptr<StorageArray_impl>(new StorageArray_impl(dev))) {};
    StorageArray &operator=(const StorageArray &obj);
    StorageArray &operator=(StorageArray &&obj) noexcept;
    void swap(StorageArray &obj) noexcept;
    VkResult StartConfig(const HostVisibleMemory val = HostVisibleMemory::HostVisible) noexcept { if (impl.get()) return impl->StartConfig(val); return VK_ERROR_UNKNOWN; }
    VkResult AddBuffer(const BufferConfig params) { if (impl.get()) return impl->AddBuffer(params); return VK_ERROR_UNKNOWN; }
    VkResult EndConfig() { if (impl.get()) return impl->EndConfig(); return VK_ERROR_UNKNOWN; }
    size_t Count() const noexcept { if (impl.get()) return impl->Count(); return 0; }
    void Clear() noexcept { if (impl.get()) impl->Clear(); }
    bool IsValid() const noexcept { return impl.get() && impl->device->IsValid(); }
    size_t SubBuffsCount(const size_t index) const noexcept { if (impl.get()) return impl->SubBuffsCount(index); return 0; }
    buffer_t GetInfo(const size_t index) const { if (impl.get()) return impl->GetInfo(index); return {}; }
    std::shared_ptr<Device> GetDevice() const noexcept { if (impl.get()) return impl->GetDevice(); return nullptr; }
    HostVisibleMemory GetMemoryAccess() const noexcept { if (impl.get()) return impl->GetMemoryAccess(); return HostVisibleMemory::HostVisible; }
    template <typename T>
    VkResult GetBufferData(const size_t index, std::vector<T> &result) const { if (impl.get()) return impl->GetBufferData(index, result); return VK_ERROR_UNKNOWN; }
    template <typename T>
    VkResult GetSubBufferData(const size_t index, const size_t sub_index, std::vector<T> &result) const { if (impl.get()) return impl->GetSubBufferData(index, sub_index, result); return VK_ERROR_UNKNOWN; }
    template <typename T>
    VkResult SetBufferData(const size_t index, const std::vector<T> &data) { if (impl.get()) return impl->SetBufferData(index, data); return VK_ERROR_UNKNOWN; }
    template <typename T>
    VkResult SetSubBufferData(const size_t index, const size_t sub_index, const std::vector<T> &data) { if (impl.get()) return impl->SetSubBufferData(index, sub_index, data); return VK_ERROR_UNKNOWN; }
  };

  void swap(StorageArray &lhs, StorageArray &rhs) noexcept;

  template <typename T>
  VkResult StorageArray_impl::GetBufferData(const size_t index, std::vector<T> &result) const
  {
    if (index >= buffers.size())
    {
      Logger::EchoError("Index is out of range", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (memory == VK_NULL_HANDLE)
    {
      Logger::EchoError("Memory is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (buffers[index].buffer == VK_NULL_HANDLE)
    {
      Logger::EchoError("Buffer is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (access == HostVisibleMemory::HostInvisible)
    {
      Logger::EchoError("Can't get data from HostInvisible memory", __func__);
      return VK_ERROR_UNKNOWN;
    }

    std::vector<T> tmp(std::ceil(buffers[index].size / sizeof(T)));
    void* payload = nullptr;

    auto er = vkMapMemory(device->GetDevice(), memory, buffers[index].offset, buffers[index].size, 0, &payload);

    if (er != VK_SUCCESS && er != VK_ERROR_MEMORY_MAP_FAILED)
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (er == VK_SUCCESS)
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
          return VK_ERROR_UNKNOWN;
        }

        std::memcpy(((uint8_t*)tmp.data()) + (i * align), payload, align);
        vkUnmapMemory(device->GetDevice(), memory);
        offset += align;
      }
      result.swap(tmp);
    }

    return VK_SUCCESS;
  }

  template <typename T>
  VkResult StorageArray_impl::GetSubBufferData(const size_t index, const size_t sub_index, std::vector<T> &result) const
  {
    if (index >= buffers.size())
    {
      Logger::EchoError("Index is out of range", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (sub_index >= buffers[index].sub_buffers.size())
    {
      Logger::EchoError("Sub index is out of range", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (memory == VK_NULL_HANDLE)
    {
      Logger::EchoError("Memory is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (buffers[index].buffer == VK_NULL_HANDLE)
    {
      Logger::EchoError("Buffer is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (access == HostVisibleMemory::HostInvisible)
    {
      Logger::EchoError("Can't get data from HostInvisible memory", __func__);
      return VK_ERROR_UNKNOWN;
    }

    std::vector<T> tmp(std::ceil(buffers[index].sub_buffers[sub_index].size / sizeof(T)));
    void* payload = nullptr;

    auto main_offset = buffers[index].offset + buffers[index].sub_buffers[sub_index].offset;
    auto er = vkMapMemory(device->GetDevice(), memory, main_offset, buffers[index].sub_buffers[sub_index].size, 0, &payload);

    if (er != VK_SUCCESS && er != VK_ERROR_MEMORY_MAP_FAILED)
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (er == VK_SUCCESS)
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
          return VK_ERROR_UNKNOWN;
        }

        std::memcpy(((uint8_t*)tmp.data()) + (i * buffers[index].sub_buffer_align), payload, buffers[index].sub_buffer_align);
        vkUnmapMemory(device->GetDevice(), memory);
        offset += buffers[index].sub_buffer_align;
      }
      result.swap(tmp);
    }

    return VK_SUCCESS;
  }

  template <typename T>
  VkResult StorageArray_impl::SetBufferData(const size_t index, const std::vector<T> &data)
  {
    if (index >= buffers.size())
    {
      Logger::EchoError("Index is out of range", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (memory == VK_NULL_HANDLE)
    {
      Logger::EchoError("Memory is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (buffers[index].buffer == VK_NULL_HANDLE)
    {
      Logger::EchoError("Buffer is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (data.size() * sizeof(T) > buffers[index].size)
    {
      Logger::EchoWarning("Data is too big for buffer", __func__);
    }

    if (access == HostVisibleMemory::HostInvisible)
    {
      Logger::EchoError("Can't get data from HostInvisible memory", __func__);
      return VK_ERROR_UNKNOWN;
    }

    void *payload = nullptr;

    auto er = vkMapMemory(device->GetDevice(), memory, buffers[index].offset, buffers[index].size, 0, &payload);

    if (er != VK_SUCCESS && er != VK_ERROR_MEMORY_MAP_FAILED)
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (er == VK_SUCCESS)
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
          return VK_ERROR_UNKNOWN;
        }

        std::memcpy(payload, ((uint8_t *) data.data()) + (i * align), align);
        vkUnmapMemory(device->GetDevice(), memory);
        offset += align;
      }
    }

    return VK_SUCCESS;
  }

  template <typename T>
  VkResult StorageArray_impl::SetSubBufferData(const size_t index, const size_t sub_index, const std::vector<T> &data)
  {
    if (index >= buffers.size())
    {
      Logger::EchoError("Index is out of range", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (index >= buffers[index].sub_buffers.size())
    {
      Logger::EchoError("Sub index is out of range", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (memory == VK_NULL_HANDLE)
    {
      Logger::EchoError("Memory is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (buffers[index].buffer == VK_NULL_HANDLE)
    {
      Logger::EchoError("Buffer is NULL", __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (data.size() * sizeof(T) > buffers[index].sub_buffers[sub_index].size)
    {
      Logger::EchoWarning("Data is too big for buffer", __func__);
    }

    if (access == HostVisibleMemory::HostInvisible)
    {
      Logger::EchoError("Can't get data from HostInvisible memory", __func__);
      return VK_ERROR_UNKNOWN;
    }

    void *payload = nullptr;

    auto main_offset = buffers[index].offset + buffers[index].sub_buffers[sub_index].offset;
    auto er = vkMapMemory(device->GetDevice(), memory, main_offset, buffers[index].sub_buffers[sub_index].size, 0, &payload);

    if (er != VK_SUCCESS && er != VK_ERROR_MEMORY_MAP_FAILED)
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
      return VK_ERROR_UNKNOWN;
    }

    if (er == VK_SUCCESS)
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
          return VK_ERROR_UNKNOWN;
        }

        std::memcpy(payload, ((uint8_t *) data.data()) + (i * buffers[index].sub_buffer_align), buffers[index].sub_buffer_align);
        vkUnmapMemory(device->GetDevice(), memory);
        offset += buffers[index].sub_buffer_align;
      }
    }

    return VK_SUCCESS;
  }
}

#endif