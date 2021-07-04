#include "StorageArray.h"

namespace Vulkan
{
  StorageArray_impl::~StorageArray_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
    Clear();
  }

  void StorageArray_impl::Abort(std::vector<buffer_t> &buffs) const noexcept
  {
    for (auto &obj : buffs)
    {
      for (auto &v : obj.sub_buffers)
      {
        if (v.view != VK_NULL_HANDLE)
          vkDestroyBufferView(device->GetDevice(), v.view, nullptr);
      }

      if (obj.buffer != VK_NULL_HANDLE)
        vkDestroyBuffer(device->GetDevice(), obj.buffer, nullptr);
    }
  }

  void StorageArray_impl::Clear() noexcept
  {
    Abort(buffers);
    if (memory != VK_NULL_HANDLE)
      vkFreeMemory(device->GetDevice(), memory, nullptr);

    buffers.clear();
  }

  StorageArray_impl::StorageArray_impl(std::shared_ptr<Device> dev)
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
    align = device->GetPhysicalDeviceProperties().limits.minMemoryMapAlignment;
  }

  VkBufferView StorageArray_impl::CreateBufferView(const VkBuffer buffer, const VkFormat format, const VkDeviceSize offset, const VkDeviceSize size)
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

  VkResult StorageArray_impl::StartConfig(const HostVisibleMemory val) noexcept
  {
    prebuild_access_config = val;
    prebuild_config.clear();

    return VK_SUCCESS;
  }

  VkResult StorageArray_impl::AddBuffer(const BufferConfig params)
  {
    BufferConfig tmp;
    tmp.sizes.reserve(params.sizes.size());
    for (const auto &p : params.sizes) 
    {
      if (std::get<0>(p) != 0 && std::get<1>(p) != 0)
        tmp.sizes.push_back(p);
    }
    tmp.buffer_type = params.buffer_type;
    tmp.sizes.shrink_to_fit();

    if (tmp.sizes.empty())
      Logger::EchoWarning("No sub buffers to process", __func__);
    else
      prebuild_config.push_back(tmp);

    return VK_SUCCESS;
  }

  VkResult StorageArray_impl::EndConfig()
  {
    if (prebuild_config.empty())
    {
      Logger::EchoWarning("Nothing to build", __func__);
      return VK_SUCCESS;
    }

    std::vector<buffer_t> tmp_buffers;
    tmp_buffers.reserve(prebuild_config.size());
    for (auto& p : prebuild_config)
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

      tmp_b.sub_buffers.reserve(p.sizes.size());
      VkDeviceSize raw_buff_size = 0;
      for (auto& b : p.sizes)
      {
        sub_buffer_t tmp_v = {};
        tmp_v.elements = std::get<0>(b);
        tmp_v.format = std::get<2>(b);
        tmp_v.size = Misc::Align(std::get<0>(b) * std::get<1>(b), tmp_b.sub_buffer_align);
        raw_buff_size += tmp_v.size;
        tmp_b.sub_buffers.push_back(tmp_v);
      }

      VkBufferCreateInfo buffer_create_info = {};
      buffer_create_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
      buffer_create_info.size = raw_buff_size;
      buffer_create_info.usage = (VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT) | (VkBufferUsageFlags)tmp_b.type;
      buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

      auto er = vkCreateBuffer(device->GetDevice(), &buffer_create_info, nullptr, &tmp_b.buffer);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Can't create Buffer. Abort", __func__);
        Logger::EchoDebug("Return code =" + std::to_string(er), __func__);
        Abort(tmp_buffers);
        return er;
      }

      tmp_buffers.push_back(tmp_b);
    }

    tmp_buffers.shrink_to_fit();
    if (tmp_buffers.empty())
    {
      Logger::EchoWarning("Nothing to build", __func__);
      return VK_SUCCESS;
    }

    Abort(buffers);

    if (memory != VK_NULL_HANDLE)
      vkFreeMemory(device->GetDevice(), memory, nullptr);

    VkDeviceSize req_mem_size = 0;
    VkMemoryRequirements mem_req = VkMemoryRequirements{ 0, 0, 0 };
    VkDeviceSize offset = 0;
    for (auto& obj : tmp_buffers)
    {
      VkMemoryRequirements mem_req_tmp = {};
      vkGetBufferMemoryRequirements(device->GetDevice(), obj.buffer, &mem_req_tmp);
      if (mem_req.memoryTypeBits != 0 && mem_req_tmp.memoryTypeBits != mem_req.memoryTypeBits)
      {
        Logger::EchoWarning("Memory types are not equal", __func__);
      }
      obj.offset = offset;
      obj.size = mem_req_tmp.size;
      VkDeviceSize v_offset = 0;
      for (auto &s : obj.sub_buffers)
      {
        s.offset = v_offset;
        v_offset += s.size;
      }
      offset += mem_req_tmp.size;

      mem_req = mem_req_tmp;
      req_mem_size += mem_req_tmp.size;
    }

    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(device->GetPhysicalDevice(), &properties);

    std::optional<uint32_t> mem_index;
    for (uint32_t i = 0; i < properties.memoryTypeCount; i++)
    {
      if (mem_req.memoryTypeBits & (1 << i) &&
        (properties.memoryTypes[i].propertyFlags & (VkMemoryPropertyFlags)prebuild_access_config) &&
        (req_mem_size < properties.memoryHeaps[properties.memoryTypes[i].heapIndex].size))
      {
        mem_index = i;
        break;
      }
    }

    if (!mem_index.has_value())
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
        mem_index.value() };

    auto er = vkAllocateMemory(device->GetDevice(), &memory_allocate_info, nullptr, &memory);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't allocate memory", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      Abort(tmp_buffers);
      return er;
    }

    bool fail = false;
    for (auto& bf : tmp_buffers)
    {
      auto er = vkBindBufferMemory(device->GetDevice(), bf.buffer, memory, bf.offset);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Can't bind memory to buffer.");
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
        fail = true;
        break;
      }

      for (auto& sb : bf.sub_buffers)
      {
        if (sb.format != VK_FORMAT_UNDEFINED)
        {
          sb.view = CreateBufferView(bf.buffer, sb.format, sb.offset, sb.size);
          if (sb.view == VK_NULL_HANDLE)
          {
            fail = true;
            Logger::EchoError("Can't create buffer view. Abort", __func__);
            break;
          }
        }
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
    size = req_mem_size;

    return VK_SUCCESS;
  }

  StorageArray &StorageArray::operator=(StorageArray &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void StorageArray::swap(StorageArray &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(StorageArray &lhs, StorageArray &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }

  StorageArray::StorageArray(const StorageArray &obj)
  {
    if (obj.impl.get() == nullptr)
    {
      Logger::EchoError("Object is empty", __func__);
      return;
    }
      
    impl = std::unique_ptr<StorageArray_impl>(new StorageArray_impl(obj.impl->device));

    if (obj.impl->buffers.empty() || obj.impl->memory == VK_NULL_HANDLE) return;

    auto res = impl->StartConfig(obj.impl->access);
    if (res != VK_SUCCESS)
    {
      Logger::EchoError("Can't copy object", __func__);
      return;
    }

    for (auto& b : obj.impl->buffers)
    {
      BufferConfig conf;
      conf.SetType(b.type);
      for (auto& p : b.sub_buffers)
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

    void* payload_from = nullptr;
    void* payload_to = nullptr;

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

    impl->EndConfig();
  }

  StorageArray &StorageArray::operator=(const StorageArray &obj)
  {
    if (&obj == this) return *this;

    if (obj.impl.get() == nullptr)
    {
      Logger::EchoError("Object is empty", __func__);
      return *this;
    }
      
    impl = std::unique_ptr<StorageArray_impl>(new StorageArray_impl(obj.impl->device));

    if (obj.impl->buffers.empty() || obj.impl->memory == VK_NULL_HANDLE) return *this;

    auto res = impl->StartConfig(obj.impl->access);
    if (res != VK_SUCCESS)
    {
      Logger::EchoError("Can't copy object", __func__);
      return *this;
    }

    for (auto& b : obj.impl->buffers)
    {
      BufferConfig conf;
      conf.SetType(b.type);
      for (auto& p : b.sub_buffers)
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

    void* payload_from = nullptr;
    void* payload_to = nullptr;

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

    impl->EndConfig();
    return *this;
  }
}