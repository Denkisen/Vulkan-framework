#include "Array.h"
#include <cstring>

namespace Vulkan
{
  template class Array<int>;
  template class Array<float>;
  template class Array<double>;
  template class Array<unsigned>;

  template <typename T>
  void Array<T>::Create(VkDevice dev, VkPhysicalDevice p_dev, T *data, size_t len, uint32_t f_queue)
  {
    if (len == 0 || data == nullptr || p_dev == VK_NULL_HANDLE)
      throw std::runtime_error("Data array is empty.");

    buffer_size = len * sizeof(T);
    VkPhysicalDeviceMemoryProperties properties;
    vkGetPhysicalDeviceMemoryProperties(p_dev, &properties);

    size_t memory_type_index = VK_MAX_MEMORY_TYPES;

    for (size_t i = 0; i < properties.memoryTypeCount; i++) 
    {
      if ((VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT & properties.memoryTypes[i].propertyFlags) &&
         (VK_MEMORY_PROPERTY_HOST_COHERENT_BIT & properties.memoryTypes[i].propertyFlags) &&
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

    if (vkAllocateMemory(dev, &memory_allocate_info, nullptr, &buffer_memory) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate memory");
    
    void *payload = nullptr;
    if (vkMapMemory(dev, buffer_memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");
    
    std::memcpy(payload, data, buffer_size);
    vkUnmapMemory(dev, buffer_memory);

    VkBufferCreateInfo buffer_create_info = {
      VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      0,
      0,
      buffer_size,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
      VK_SHARING_MODE_EXCLUSIVE,
      1,
      &f_queue
    };

    if (vkCreateBuffer(dev, &buffer_create_info, nullptr, &buffer) != VK_SUCCESS)
      throw std::runtime_error("Can't create Buffer.");

    VkMemoryRequirements mem_req = {};
    vkGetBufferMemoryRequirements(dev, buffer, &mem_req);
    if (buffer_size < mem_req.size)
      throw std::runtime_error("Buffer is to small, minimum is " + std::to_string(mem_req.size / sizeof(T)));

    if (vkBindBufferMemory(dev, buffer, buffer_memory, 0) != VK_SUCCESS)
      throw std::runtime_error("Can't bind memory to buffer.");

    device = dev;
    family_queue = f_queue;
    p_device = p_dev;
    type = StorageType::Default; // VK_BUFFER_USAGE_STORAGE_BUFFER_BIT
  }

  template <typename T>
  void Array<T>::Create(Device &dev, T *data, size_t len)
  {
    Create(dev.device, dev.p_device, data, len, dev.family_queue);
  }

  template <typename T>
  Array<T>::Array(Device &dev, std::vector<T> &data)
  {
    if (data.size() == 0)
      throw std::runtime_error("Data array is empty.");
    
    Create(dev, data.data(), data.size());
  }

  template <typename T>
  Array<T>::Array(Device &dev, T *data, size_t len)
  {
    Create(dev, data, len);
  }

  template <typename T> 
  Array<T>::Array(const Array<T> &array)
  {
    if (device != VK_NULL_HANDLE)
    {
      vkFreeMemory(device, buffer_memory, nullptr);
      vkDestroyBuffer(device, buffer, nullptr);
      device = VK_NULL_HANDLE;
    }

    std::vector<T> data(array.Extract());
    Create(array.device, array.p_device, data.data(), data.size(), array.family_queue);
  }

  template <typename T> 
  Array<T>& Array<T>::operator= (const Array<T> &obj)
  {
    if (device != VK_NULL_HANDLE)
    {
      vkFreeMemory(device, buffer_memory, nullptr);
      vkDestroyBuffer(device, buffer, nullptr);
      device = VK_NULL_HANDLE;
    }
    
    std::vector<T> data(obj.Extract());
    Create(obj.device, obj.p_device, data.data(), data.size(), obj.family_queue);

    return *this;
  }
  
  template <typename T> 
  std::vector<T> Array<T>::Extract() const
  {
    void *payload = nullptr;
    if (vkMapMemory(device, buffer_memory, 0, VK_WHOLE_SIZE, 0, &payload) != VK_SUCCESS)
      throw std::runtime_error("Can't map memory.");
    
    std::vector<T> data(buffer_size / sizeof(T));
    std::copy((T *)payload, &((T *)payload)[data.size()], data.begin());
    vkUnmapMemory(device, buffer_memory);

    return data;
  }
}