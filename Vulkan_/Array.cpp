#include "Array.h"

namespace Vulkan
{
  Array_impl::~Array_impl()
  {
    Logger::EchoDebug("", __func__);
    Abort(buffers);
    if (memory != VK_NULL_HANDLE)
      vkFreeMemory(device->GetDevice(), memory, nullptr);
  }

  void Array_impl::Abort(std::vector<buffer_t> &buffs)
  {
    for (auto &p : buffs)
    {
      for (auto &v : p.sub_buffers)
      {
        if (v.view != VK_NULL_HANDLE)
          vkDestroyBufferView(device->GetDevice(), v.view, nullptr);
      }

      if (p.buffer != VK_NULL_HANDLE)
        vkDestroyBuffer(device->GetDevice(), p.buffer, nullptr);
    }
    buffs.clear();
  }

  void Array_impl::Clear()
  {
    Abort(buffers);
    if (memory != VK_NULL_HANDLE)
      vkFreeMemory(device->GetDevice(), memory, nullptr);
  }

  Array_impl::Array_impl(std::shared_ptr<Vulkan::Device> dev)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
  }

  VkBufferView Array_impl::CreateBufferView(const VkBuffer buffer, const VkFormat format, const VkDeviceSize offset, const VkDeviceSize size)
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

    auto er = vkCreateBufferView(device->GetDevice(), &buffer_view_create_info, nullptr, &result);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't create buffer view", __func__);
      Logger::EchoDebug("vkCreateBufferView return = " + std::to_string(er), __func__);
      return result;
    }

    return result;
  }

  VkDeviceSize Array_impl::Align(const VkDeviceSize value, const VkDeviceSize align)
  { 
    return (std::ceil(value / (float) align) * align); 
  }

  VkResult Array_impl::StartConfig(const HostVisibleMemory val)
  {
    std::lock_guard lock(config_mutex);
    prebuild_access_config = val;
    prebuild_config.clear();

    return VK_SUCCESS;
  }

  VkResult Array_impl::AddBuffer(const BufferConfig params)
  {
    std::lock_guard lock(config_mutex);
    BufferConfig tmp;
    for (const auto &p : params.sizes)
    {
      if (std::get<0>(p) != 0 && std::get<1>(p) != 0)
        tmp.sizes.push_back(p);
    }
    tmp.buffer_type = params.buffer_type;

    if (tmp.sizes.empty())
      Logger::EchoWarning("No sub buffers to process", __func__);
    else
      prebuild_config.push_back(tmp);
    
    return VK_SUCCESS;
  }

  VkResult Array_impl::EndConfig()
  {
    std::lock_guard lock(config_mutex);
    if (prebuild_config.empty())
    {
      Logger::EchoWarning("Nothing to build", __func__);
      return VK_SUCCESS;
    }

    std::vector<buffer_t> tmp_buffers;
    VkDeviceSize mem_size = 0;
    VkDeviceSize b_offset = 0;
    for (auto &p : prebuild_config)
    {
      buffer_t tmp_b = {};
      tmp_b.type = p.buffer_type;
      tmp_b.size = 0;
      switch (tmp_b.type)
      {
        case StorageType::Index:
        case StorageType::Vertex:
        case StorageType::Storage:
          tmp_b.sub_buffer_align = device->GetPhysicalDeviceProperties().limits.minStorageBufferOffsetAlignment;
          break;
        case StorageType::Uniform:
          tmp_b.sub_buffer_align = device->GetPhysicalDeviceProperties().limits.minUniformBufferOffsetAlignment;
          break;
        case StorageType::TexelStorage:
        case StorageType::TexelUniform:
          tmp_b.sub_buffer_align = device->GetPhysicalDeviceProperties().limits.minTexelBufferOffsetAlignment;
          break;
      }

      if (p.sizes.empty())
      {
        Logger::EchoWarning("No sub buffers. Ignoring", __func__);
        continue;
      }

      VkDeviceSize v_offset = 0;
      for (auto &b : p.sizes)
      {
        sub_buffer_t tmp_v = {};
        tmp_v.format = std::get<2>(b);
        tmp_v.size = Align(std::get<0>(b) * std::get<1>(b), tmp_b.sub_buffer_align);
        tmp_v.offset = v_offset;
        tmp_b.size += tmp_v.size;
        v_offset += tmp_b.size;
        tmp_b.sub_buffers.push_back(tmp_v);
      }

      tmp_b.size = Align(tmp_b.size, device->GetPhysicalDeviceProperties().limits.minMemoryMapAlignment);
      mem_size += tmp_b.size;
      tmp_b.offset = b_offset;
      b_offset += tmp_b.size;

      VkBufferCreateInfo buffer_create_info = {};
      buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      buffer_create_info.size = tmp_b.size;
      buffer_create_info.usage = (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT) | (VkBufferUsageFlags) tmp_b.type;
      buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      auto er = vkCreateBuffer(device->GetDevice(), &buffer_create_info, nullptr, &tmp_b.buffer);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Can't create Buffer. Abort", __func__);
        Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
        Abort(tmp_buffers);
        return er;
      }

      bool fail = false;
      for (auto &v : tmp_b.sub_buffers)
      {
        if (v.format != VK_FORMAT_UNDEFINED)
        {
          v.view = CreateBufferView(tmp_b.buffer, v.format, v.offset, v.size);
          if (v.view == VK_NULL_HANDLE)
          {
            fail = true;
            Logger::EchoError("Can't create buffer view. Abort", __func__);
            break;
          }
        }
      }

      if (fail)
      {
        for (auto &v : tmp_b.sub_buffers)
        {
          if (v.view != VK_NULL_HANDLE)
            vkDestroyBufferView(device->GetDevice(), v.view, nullptr);
        }
        return VK_ERROR_UNKNOWN;
      }

      tmp_buffers.push_back(tmp_b);
    }

    if (tmp_buffers.empty())
    {
      Logger::EchoWarning("Nothing to build", __func__);
      return VK_SUCCESS;
    }

    std::lock_guard lock1(buffers_mutex);
    Abort(buffers);

    if (memory != VK_NULL_HANDLE)
      vkFreeMemory(device->GetDevice(), memory, nullptr);

    VkMemoryRequirements mem_req = {};
    VkDeviceSize req_mem_size = 0;

    for (size_t i = 0; i < tmp_buffers.size(); ++i)
    {
      mem_req = {};
      vkGetBufferMemoryRequirements(device->GetDevice(), tmp_buffers[i].buffer, &mem_req);
      req_mem_size += mem_req.size;
    }

    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(device->GetPhysicalDevice(), &properties);

    std::optional<uint32_t> mem_index;
    for (uint32_t i = 0; i < properties.memoryTypeCount; i++) 
    {
      if (mem_req.memoryTypeBits & (1 << i) && 
          (properties.memoryTypes[i].propertyFlags & (VkMemoryPropertyFlags) prebuild_access_config) &&
          (req_mem_size < properties.memoryHeaps[properties.memoryTypes[i].heapIndex].size))
      {
        mem_index = i;
        break;
      }
    }

    if (!mem_index.has_value() || req_mem_size != mem_size)
    {
      Logger::EchoError("No memory index", __func__);
      Abort(tmp_buffers);
      return VK_ERROR_UNKNOWN;
    }

    VkMemoryAllocateInfo memory_allocate_info = 
    {
      VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      0,
      req_mem_size,
      mem_index.value()
    };

    auto er = vkAllocateMemory(device->GetDevice(), &memory_allocate_info, nullptr, &memory);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't allocate memory", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      Abort(tmp_buffers);
      return er;
    }

    bool fail = false;
    for (auto &b : tmp_buffers)
    {
      er = vkBindBufferMemory(device->GetDevice(), b.buffer, memory, b.offset);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Can't bind memory to buffer.");
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
        fail = true;
        break;
      }
    }

    if (fail)
    {
      Abort(tmp_buffers);
      if (memory != VK_NULL_HANDLE)
        vkFreeMemory(device->GetDevice(), memory, nullptr);
    }

    buffers.swap(tmp_buffers);
    access = prebuild_access_config;
    size = mem_size;
    align = device->GetPhysicalDeviceProperties().limits.minMemoryMapAlignment;

    return VK_SUCCESS;
  }

  Array &Array::operator=(Array &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void Array::swap(Array &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(Array &lhs, Array &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }

  Array::Array(const Array &obj)
  {
    if (obj.impl.get() == nullptr)
    {
      Logger::EchoError("Object is empty", __func__);
      return;
    }
      
    std::lock_guard lock(obj.impl->buffers_mutex);
    impl = std::unique_ptr<Array_impl>(new Array_impl(obj.impl->device));

    if (obj.impl->buffers.empty() || obj.impl->memory == VK_NULL_HANDLE) return;

    auto res = impl->StartConfig(obj.impl->access);
    if (res != VK_SUCCESS)
    {
      Logger::EchoError("Can't copy object", __func__);
      return;
    }

    for (auto &b : obj.impl->buffers)
    {
      BufferConfig conf;
      conf.SetType(b.type);
      for (auto &p : b.sub_buffers)
      {
        conf.AddSubBuffer(p.size, 1, p.format);
      }
      res = impl->AddBuffer(conf);
      if (res != VK_SUCCESS)
      {
        Logger::EchoError("Can't copy object", __func__);
        return;
      }
    }
    res = impl->EndConfig();
    if (res != VK_SUCCESS)
    {
      Logger::EchoError("Can't copy object", __func__);
      return;
    }

    void *payload_from = nullptr;
    void *payload_to = nullptr;

    auto er1 = vkMapMemory(impl->device->GetDevice(), impl->memory, 0, impl->size, 0, &payload_to);
    auto er2 = vkMapMemory(obj.impl->device->GetDevice(), obj.impl->memory, 0, obj.impl->size, 0, &payload_from);
    if (er1 != VK_SUCCESS && er1 != VK_ERROR_MEMORY_MAP_FAILED &&
        er2 != VK_SUCCESS && er2 != VK_ERROR_MEMORY_MAP_FAILED) 
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er1), __func__);
      return;
    }

    if (er1 == VK_SUCCESS && er2 == VK_SUCCESS)
    {
      std::memcpy(payload_to, payload_from, impl->size);
      vkUnmapMemory(impl->device->GetDevice(), impl->memory);
      vkUnmapMemory(obj.impl->device->GetDevice(), obj.impl->memory);
    }
    else
    {
      if (er1 == VK_SUCCESS)
        vkUnmapMemory(impl->device->GetDevice(), impl->memory);
      if (er2 == VK_SUCCESS)
        vkUnmapMemory(obj.impl->device->GetDevice(), obj.impl->memory);

      VkDeviceSize offset = 0;
      for (VkDeviceSize i = 0; i < obj.impl->size / obj.impl->align; ++i)
      {
        er1 = vkMapMemory(impl->device->GetDevice(), impl->memory, offset, obj.impl->align, 0, &payload_to);
        er2 = vkMapMemory(obj.impl->device->GetDevice(), obj.impl->memory, offset, obj.impl->align, 0, &payload_from);

        if (er1 != VK_SUCCESS && er1 != VK_ERROR_MEMORY_MAP_FAILED &&
            er2 != VK_SUCCESS && er2 != VK_ERROR_MEMORY_MAP_FAILED) 
        {
          Logger::EchoError("Can't map memory.", __func__);
          Logger::EchoDebug("Return code =" + std::to_string(er1), __func__);
          return;
        }

        std::memcpy(payload_to, payload_from, obj.impl->align);

        vkUnmapMemory(impl->device->GetDevice(), impl->memory);
        vkUnmapMemory(obj.impl->device->GetDevice(), obj.impl->memory);

        offset += obj.impl->align;
      }
    }
  }

  Array &Array::operator=(const Array &obj)
  {
    if (&obj == this) return *this;

    if (obj.impl.get() == nullptr)
    {
      Logger::EchoError("Object is empty", __func__);
      return *this;
    }
      
    std::lock_guard lock(obj.impl->buffers_mutex);
    impl = std::unique_ptr<Array_impl>(new Array_impl(obj.impl->device));

    if (obj.impl->buffers.empty() || obj.impl->memory == VK_NULL_HANDLE) return *this;

    auto res = impl->StartConfig(obj.impl->access);
    if (res != VK_SUCCESS)
    {
      Logger::EchoError("Can't copy object", __func__);
      return *this;
    }

    for (auto &b : obj.impl->buffers)
    {
      BufferConfig conf;
      conf.SetType(b.type);
      for (auto &p : b.sub_buffers)
      {
        conf.AddSubBuffer(p.size, 1, p.format);
      }
      res = impl->AddBuffer(conf);
      if (res != VK_SUCCESS)
      {
        Logger::EchoError("Can't copy object", __func__);
        return *this;
      }
    }
    res = impl->EndConfig();
    if (res != VK_SUCCESS)
    {
      Logger::EchoError("Can't copy object", __func__);
      return *this;
    }

    void *payload_from = nullptr;
    void *payload_to = nullptr;

    auto er1 = vkMapMemory(impl->device->GetDevice(), impl->memory, 0, impl->size, 0, &payload_to);
    auto er2 = vkMapMemory(obj.impl->device->GetDevice(), obj.impl->memory, 0, obj.impl->size, 0, &payload_from);
    if (er1 != VK_SUCCESS && er1 != VK_ERROR_MEMORY_MAP_FAILED &&
        er2 != VK_SUCCESS && er2 != VK_ERROR_MEMORY_MAP_FAILED) 
    {
      Logger::EchoError("Can't map memory.", __func__);
      Logger::EchoDebug("Return code =" + std::to_string(er1), __func__);
      return *this;
    }

    if (er1 == VK_SUCCESS && er2 == VK_SUCCESS)
    {
      std::memcpy(payload_to, payload_from, impl->size);
      vkUnmapMemory(impl->device->GetDevice(), impl->memory);
      vkUnmapMemory(obj.impl->device->GetDevice(), obj.impl->memory);
    }
    else
    {
      if (er1 == VK_SUCCESS)
        vkUnmapMemory(impl->device->GetDevice(), impl->memory);
      if (er2 == VK_SUCCESS)
        vkUnmapMemory(obj.impl->device->GetDevice(), obj.impl->memory);

      VkDeviceSize offset = 0;
      for (VkDeviceSize i = 0; i < obj.impl->size / obj.impl->align; ++i)
      {
        er1 = vkMapMemory(impl->device->GetDevice(), impl->memory, offset, obj.impl->align, 0, &payload_to);
        er2 = vkMapMemory(obj.impl->device->GetDevice(), obj.impl->memory, offset, obj.impl->align, 0, &payload_from);

        if (er1 != VK_SUCCESS && er1 != VK_ERROR_MEMORY_MAP_FAILED &&
            er2 != VK_SUCCESS && er2 != VK_ERROR_MEMORY_MAP_FAILED) 
        {
          Logger::EchoError("Can't map memory.", __func__);
          Logger::EchoDebug("Return code =" + std::to_string(er1), __func__);
          return *this;
        }

        std::memcpy(payload_to, payload_from, obj.impl->align);

        vkUnmapMemory(impl->device->GetDevice(), impl->memory);
        vkUnmapMemory(obj.impl->device->GetDevice(), obj.impl->memory);

        offset += obj.impl->align;
      }
    }

    return *this;
  }
}