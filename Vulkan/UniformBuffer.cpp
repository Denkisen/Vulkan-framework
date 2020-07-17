#include "UniformBuffer.h"
#include <cstring>
#include <optional>

namespace Vulkan
{
  void UniformBuffer::Create(std::shared_ptr<Vulkan::Device> dev, void *data, std::size_t len, uint32_t f_queue)
  {
    if (len == 0 || data == nullptr || dev == nullptr)
      throw std::runtime_error("Data array is empty.");

    buffer_size = len;
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(dev->GetPhysicalDevice(), &properties);

    VkBufferCreateInfo buffer_create_info = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      nullptr,
      0,
      buffer_size,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      1,
      &f_queue
    };

    if (vkCreateBuffer(dev->GetDevice(), &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
      throw std::runtime_error("Can't create Buffer.");

    VkMemoryRequirements mem_req = {};
    vkGetBufferMemoryRequirements(dev->GetDevice(), buffer, &mem_req);

    buffer_size = mem_req.size;

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
    
    std::memcpy(payload, data, len);
    vkUnmapMemory(dev->GetDevice(), buffer_memory);

    if (vkBindBufferMemory(dev->GetDevice(), buffer, buffer_memory, 0) != VK_SUCCESS)
      throw std::runtime_error("Can't bind memory to buffer.");

    device = dev;
    family_queue = f_queue;
    type = StorageType::Uniform; // VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
  }

  UniformBuffer::UniformBuffer(std::shared_ptr<Vulkan::Device> dev, void *data, std::size_t len, uint32_t family_q)
  {
    Create(dev, data, len, family_q);
  }

  UniformBuffer::UniformBuffer(const UniformBuffer &obj)
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      vkFreeMemory(device->GetDevice(), buffer_memory, nullptr);
      vkDestroyBuffer(device->GetDevice(), buffer, nullptr);
      device.reset();
    }

    std::size_t sz = 0;
    void *tmp = Extract(sz);
    Create(obj.device, tmp, buffer_size, obj.family_queue);
    std::free(tmp);
  }

  UniformBuffer& UniformBuffer::operator= (const UniformBuffer &obj)
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      vkFreeMemory(device->GetDevice(), buffer_memory, nullptr);
      vkDestroyBuffer(device->GetDevice(), buffer, nullptr);
      device.reset();
    }
    
    std::size_t sz = 0;
    void *tmp = Extract(sz);
    Create(obj.device, tmp, buffer_size, obj.family_queue);
    std::free(tmp);
    
    return *this;
  }
}