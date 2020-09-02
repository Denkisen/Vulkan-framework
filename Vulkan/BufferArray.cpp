#include "BufferArray.h"
#include "Supply.h"

#include <optional>
#include <algorithm>

namespace Vulkan
{
  void BufferArray::Destroy()
  {
    if (device.get() != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      std::lock_guard<std::mutex> lock(buffers_mutex);
      for (auto &b : buffers)
      {
        for (auto &s : b.virt_buffers)
          if (s.view != VK_NULL_HANDLE)
            vkDestroyBufferView(device->GetDevice(), s.view, nullptr);
        
        if (b.buffer != VK_NULL_HANDLE)
          vkDestroyBuffer(device->GetDevice(), b.buffer, nullptr);
        
        if (b.memory != VK_NULL_HANDLE)
          vkFreeMemory(device->GetDevice(), b.memory, nullptr);
      }
      buffers.clear();
    }
  }

  BufferArray::BufferArray(std::shared_ptr<Vulkan::Device> dev)
  {
    device = dev;
  }

  BufferArray::~BufferArray()
  {
#ifdef DEBUG
    std::cout << __func__ << std::endl;
#endif
    Destroy();
    device.reset();
  }

  VkBufferView BufferArray::CreateBufferView(const VkBuffer buffer, const VkFormat format, const uint32_t offset, const uint32_t size)
  {
    VkBufferView result = VK_NULL_HANDLE;
    VkBufferViewCreateInfo buffer_view_create_info = {};
    buffer_view_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO;
    buffer_view_create_info.pNext = nullptr;
    buffer_view_create_info.flags = 0;
    buffer_view_create_info.buffer = buffer;
    buffer_view_create_info.offset = offset;
    buffer_view_create_info.range = size;
    buffer_view_create_info.format = format;

    if (vkCreateBufferView(device->GetDevice(), &buffer_view_create_info, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("failed to creat BufferView!");

    return result;
  }

  void BufferArray::DeclareBuffer(const size_t memory_size, const Vulkan::HostVisibleMemory memory_access, const Vulkan::StorageType buffer_type, const VkFormat buffer_format)
  {
    if (buffer_type == Vulkan::StorageType::None)
      throw std::runtime_error("Invalid buffer type");

    if ((buffer_type == Vulkan::StorageType::TexelStorage || buffer_type == Vulkan::StorageType::TexelUniform) && buffer_format == VK_FORMAT_UNDEFINED)
      throw std::runtime_error("Invalid buffer format");

    if (memory_size == 0) return;

    if (buffer_format != VK_FORMAT_UNDEFINED)
    {
      auto props = device->GetFormatProperties(buffer_format);

      if(buffer_type == StorageType::TexelUniform && !(props.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT))
        throw std::runtime_error("Invalid Texel buffer format.");

      if(buffer_type == StorageType::TexelStorage && !(props.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT))
        throw std::runtime_error("Invalid Texel buffer format.");
    }

    buffer_t result = {};

    auto dev_limits = device->GetLimits();

    switch (buffer_type)
    {
      case StorageType::Storage:
        result.virt_buffer_align = dev_limits.minStorageBufferOffsetAlignment;
        if (dev_limits.maxStorageBufferRange < memory_size)
          throw std::runtime_error("Buffer size is too big for device.");
        break;
      case StorageType::Uniform:
        result.virt_buffer_align = dev_limits.minUniformBufferOffsetAlignment;
        if (dev_limits.maxUniformBufferRange < memory_size)
          throw std::runtime_error("Buffer size is too big for device.");
        break;
      case StorageType::Vertex:
        result.virt_buffer_align = dev_limits.minStorageBufferOffsetAlignment;
        break;
      case StorageType::Index:
        result.virt_buffer_align = dev_limits.minStorageBufferOffsetAlignment;
        break;
      case StorageType::TexelStorage:
      case StorageType::TexelUniform:
        result.virt_buffer_align = dev_limits.minTexelBufferOffsetAlignment;
        if (dev_limits.maxTexelBufferElements < memory_size / Supply::SizeOfFormat(buffer_format))
          throw std::runtime_error("Buffer size is too big for device.");
        break;
      default:
        throw std::runtime_error("Unknown buffer type.");
    }

// Get memory alignment
    VkBufferCreateInfo buffer_create_info = {};
    buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_create_info.size = memory_size;
    buffer_create_info.usage = (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT) | (VkBufferUsageFlags) buffer_type;
    buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    VkMemoryPropertyFlags flags = (VkMemoryPropertyFlags) memory_access;

    if (vkCreateBuffer(device->GetDevice(), &buffer_create_info, nullptr, &result.buffer) != VK_SUCCESS)
      throw std::runtime_error("Can't create Buffer.");

    auto memory_type_index = Vulkan::Supply::GetMemoryTypeIndex(device->GetDevice(), device->GetPhysicalDevice(), result.buffer, result.memory_size, flags);

    if (!memory_type_index.has_value())
      throw std::runtime_error("Out of memory.");

    if (result.memory_size.first != memory_size)
    {
      vkDestroyBuffer(device->GetDevice(), result.buffer, nullptr); 

      buffer_create_info.size = result.memory_size.first;

      if (vkCreateBuffer(device->GetDevice(), &buffer_create_info, nullptr, &result.buffer) != VK_SUCCESS)
        throw std::runtime_error("Can't create Buffer.");

      memory_type_index = Vulkan::Supply::GetMemoryTypeIndex(device->GetDevice(), device->GetPhysicalDevice(), result.buffer, result.memory_size, flags);

      if (!memory_type_index.has_value())
        throw std::runtime_error("Out of memory.");
    }
    
    VkMemoryAllocateInfo memory_allocate_info = 
    {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      0,
      result.memory_size.first,
      (uint32_t) memory_type_index.value()
    };

    if (vkAllocateMemory(device->GetDevice(), &memory_allocate_info, nullptr, &result.memory) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate memory");

    if (vkBindBufferMemory(device->GetDevice(), result.buffer, result.memory, 0) != VK_SUCCESS)
      throw std::runtime_error("Can't bind memory to buffer.");

    result.access = memory_access;
    result.type = buffer_type;
    result.virt_buffers.push_back(
    {
      result.memory_size.first,
      0,
      buffer_format != VK_FORMAT_UNDEFINED ? CreateBufferView(result.buffer, buffer_format, 0, result.memory_size.first) : VK_NULL_HANDLE, 
      buffer_format
    });

    std::lock_guard<std::mutex> lock(buffers_mutex);
    buffers.push_back(result);
  }

  void BufferArray::DeclareVirtualBuffer(const size_t buffer_index, const size_t offset, const size_t size, const VkFormat buffer_format)
  {
    std::lock_guard<std::mutex> lock(buffers_mutex);
    if (buffer_index >= buffers.size())
      throw std::runtime_error("Index is out of bounds");

    if (offset == 0 && size == 0) return;

    if ((buffers[buffer_index].type == Vulkan::StorageType::TexelStorage || buffers[buffer_index].type == Vulkan::StorageType::TexelUniform) && buffer_format == VK_FORMAT_UNDEFINED)
      throw std::runtime_error("Invalid buffer format");

    if (buffer_format != VK_FORMAT_UNDEFINED)
    {
      auto props = device->GetFormatProperties(buffer_format);

      if(buffers[buffer_index].type == StorageType::TexelUniform && !(props.bufferFeatures & VK_FORMAT_FEATURE_UNIFORM_TEXEL_BUFFER_BIT))
        throw std::runtime_error("Invalid Texel buffer format.");

      if(buffers[buffer_index].type == StorageType::TexelStorage && !(props.bufferFeatures & VK_FORMAT_FEATURE_STORAGE_TEXEL_BUFFER_BIT))
        throw std::runtime_error("Invalid Texel buffer format.");
    }

    uint32_t s = GetAligned(size, device->GetLimits().minMemoryMapAlignment);
    uint32_t o = GetAligned(offset, buffers[buffer_index].virt_buffer_align);

    buffers[buffer_index].virt_buffers.push_back(
    {
      s,
      o,
      buffer_format != VK_FORMAT_UNDEFINED ? CreateBufferView(buffers[buffer_index].buffer, buffer_format, o, s) : VK_NULL_HANDLE, 
      buffer_format
    });
  }

  void BufferArray::ClearVirtualBuffers(const size_t buffer_index)
  {
    std::lock_guard<std::mutex> lock(buffers_mutex);

    if (buffer_index >= buffers.size())
      throw std::runtime_error("Index is out of bounds");

    for (size_t i = 1 ; i < buffers[buffer_index].virt_buffers.size(); ++i)
      if (buffers[buffer_index].virt_buffers[i].view != VK_NULL_HANDLE)
        vkDestroyBufferView(device->GetDevice(), buffers[buffer_index].virt_buffers[i].view, nullptr);
    buffers[buffer_index].virt_buffers.resize(1);
  }

  std::pair<VkBuffer, VkBufferView> BufferArray::GetWholeBuffer(const size_t buffer_index)
  {
    std::lock_guard<std::mutex> lock(buffers_mutex);

    if (buffer_index >= buffers.size())
      throw std::runtime_error("Index is out of bounds");

    return { buffers[buffer_index].buffer, buffers[buffer_index].virt_buffers[0].view };
  }

  std::pair<VkBuffer, virtual_buffer_t> BufferArray::GetVirtualBuffer(const size_t buffer_index, const size_t virtual_buffer_index)
  {
    std::lock_guard<std::mutex> lock(buffers_mutex);

    if (buffer_index >= buffers.size())
      throw std::runtime_error("Index is out of bounds");

    if (virtual_buffer_index >= buffers[buffer_index].virt_buffers.size() - 1)
      throw std::runtime_error("Virtual buffer index is out of bounds");

    return { buffers[buffer_index].buffer, buffers[buffer_index].virt_buffers[virtual_buffer_index + 1] };
  }

  size_t BufferArray::BufferSize(const size_t index)
  {
    std::lock_guard<std::mutex> lock(buffers_mutex);

    if (index >= buffers.size())
      throw std::runtime_error("Index is out of bounds");

    return buffers[index].memory_size.first;
  }

  size_t BufferArray::VirtualBuffersCount(const size_t index)
  {
    std::lock_guard<std::mutex> lock(buffers_mutex);

    if (index >= buffers.size())
      throw std::runtime_error("Index is out of bounds");

    return buffers[index].virt_buffers.size() - 1;
  }

  Vulkan::StorageType BufferArray::BufferType(const size_t index)
  {
    std::lock_guard<std::mutex> lock(buffers_mutex);

    if (index >= buffers.size())
      throw std::runtime_error("Index is out of bounds");

    return buffers[index].type;
  }

  size_t BufferArray::CalculateBufferSize(const size_t virtual_buffer_size, const size_t virtual_buffers_count, const Vulkan::StorageType buffer_type)
  {
    size_t result = 0;
    size_t sz = 0;

    auto dev_limits = device->GetLimits();

    switch (buffer_type)
    {
      case StorageType::Storage:
        sz = dev_limits.minStorageBufferOffsetAlignment;
        break;
      case StorageType::Uniform:
        sz = dev_limits.minUniformBufferOffsetAlignment;
        break;
      case StorageType::Vertex:
        sz = dev_limits.minStorageBufferOffsetAlignment;
        break;
      case StorageType::Index:
        sz = dev_limits.minStorageBufferOffsetAlignment;
        break;
      case StorageType::TexelStorage:
      case StorageType::TexelUniform:
        sz = dev_limits.minTexelBufferOffsetAlignment;
        break;
      default:
        throw std::runtime_error("Unknown buffer type.");
    }

    sz = GetAligned(virtual_buffer_size, sz);

    for (size_t i = 0; i < virtual_buffers_count; ++i)
      result += sz;

    return result;
  }
}