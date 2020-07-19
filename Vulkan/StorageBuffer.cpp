#include "StorageBuffer.h"

namespace Vulkan
{
  StorageBuffer::StorageBuffer(std::shared_ptr<Vulkan::Device> dev)
  {
    device = dev;
  }

  StorageBuffer::StorageBuffer(const StorageBuffer &obj)
  {
    Clear();
    device = obj.device;
    DataLayout layout = GetDataLayout(obj.buffers);
    descriptor_set_layout = CreateDescriptorSetLayout(layout);
    descriptor_pool = CreateDescriptorPool(layout);
    descriptor_set = CreateDescriptorSet(descriptor_pool, descriptor_set_layout);
    buffers = UpdateDescriptorSet(descriptor_set, obj.buffers);
  }

  StorageBuffer::StorageBuffer(std::shared_ptr<Vulkan::Device> dev, std::vector<IStorage*> &data)
  {
    if (!IsDataVectorValid(data))
      throw std::runtime_error(std::string(__func__) + ": Storage array is not valid.");
    device = dev;
    DataLayout layout = GetDataLayout(data);
    descriptor_set_layout = CreateDescriptorSetLayout(layout);
    descriptor_pool = CreateDescriptorPool(layout);
    descriptor_set = CreateDescriptorSet(descriptor_pool, descriptor_set_layout);
    buffers = UpdateDescriptorSet(descriptor_set, data);
  }
  
  StorageBuffer& StorageBuffer::operator= (const StorageBuffer &obj)
  {
    Clear();
    device = obj.device;
    DataLayout layout = GetDataLayout(obj.buffers);
    descriptor_set_layout = CreateDescriptorSetLayout(layout);
    descriptor_pool = CreateDescriptorPool(layout);
    descriptor_set = CreateDescriptorSet(descriptor_pool, descriptor_set_layout);
    buffers = UpdateDescriptorSet(descriptor_set, obj.buffers);
  
    return *this;
  }

  StorageBuffer& StorageBuffer::operator= (const std::vector<IStorage*> &obj)
  {
    if (!IsDataVectorValid(obj))
      throw std::runtime_error(std::string(__func__) + ": Storage array is not valid.");
    DataLayout layout = GetDataLayout(obj);
    if (descriptor_set == VK_NULL_HANDLE || device != obj[0]->device)
    {
      Clear();
      device = obj[0]->device;
      descriptor_set_layout = CreateDescriptorSetLayout(layout);
      descriptor_pool = CreateDescriptorPool(layout);
      descriptor_set = CreateDescriptorSet(descriptor_pool, descriptor_set_layout);
    }

    buffers = UpdateDescriptorSet(descriptor_set, obj); 

    return *this;
  }

  StorageBuffer::~StorageBuffer()
  {
#ifdef DEBUG
    std::cout << __func__ << std::endl;
#endif
    Clear();
  }

  void StorageBuffer::Clear()
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      if (descriptor_pool != VK_NULL_HANDLE)
      {
        vkDestroyDescriptorPool(device->GetDevice(), descriptor_pool, nullptr);
        descriptor_pool = VK_NULL_HANDLE;
      }
      if (descriptor_set_layout != VK_NULL_HANDLE)
      {
        vkDestroyDescriptorSetLayout(device->GetDevice(), descriptor_set_layout, nullptr);
        descriptor_set_layout = VK_NULL_HANDLE;
      }
    }
    device.reset();
  }

  VkDescriptorSetLayout StorageBuffer::CreateDescriptorSetLayout(DataLayout data_layout)
  {
    VkDescriptorSetLayout result = VK_NULL_HANDLE;
    std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(data_layout.layout.size());
    for (std::size_t i = 0; i < data_layout.layout.size(); ++i)
    {
      VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
      descriptor_set_layout_binding.binding = i;
      switch (data_layout.layout[i])
      {
      case StorageType::Default :
        descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        break;
      case StorageType::Uniform :
        descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        break;
      case StorageType::Vertex :
        break;
      default:
        std::runtime_error("Unsupported storage type");
      }
      descriptor_set_layout_binding.descriptorCount = 1;
      descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
      descriptor_set_layout_bindings[i] = descriptor_set_layout_binding;
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.bindingCount = (uint32_t) descriptor_set_layout_bindings.size(); // only a single binding in this descriptor set layout. 
    descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings.data(); 

    if (vkCreateDescriptorSetLayout(device->GetDevice(), &descriptor_set_layout_create_info, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("Can't create DescriptorSetLayout.");
    
    return result;
  }
  
  VkDescriptorPool StorageBuffer::CreateDescriptorPool(DataLayout data_layout)
  {
    VkDescriptorPool result = VK_NULL_HANDLE;
    std::vector<VkDescriptorPoolSize> pool_sizes(2);
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = (uint32_t) data_layout.uniform_buffers;

    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_sizes[1].descriptorCount = (uint32_t) data_layout.storage_buffers;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = pool_sizes.size();
    descriptor_pool_create_info.pPoolSizes = pool_sizes.data();

    if (vkCreateDescriptorPool(device->GetDevice(), &descriptor_pool_create_info, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("Can't creat DescriptorPool.");

    return result;
  }

  VkDescriptorSet StorageBuffer::CreateDescriptorSet(VkDescriptorPool pool, VkDescriptorSetLayout layout)
  {
    VkDescriptorSet result = VK_NULL_HANDLE;
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO; 
    descriptor_set_allocate_info.descriptorPool = pool; // pool to allocate from.
    descriptor_set_allocate_info.descriptorSetCount = 1; // allocate a single descriptor set.
    descriptor_set_allocate_info.pSetLayouts = &layout;

    if (vkAllocateDescriptorSets(device->GetDevice(), &descriptor_set_allocate_info, &result) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate DescriptorSets.");
    
    return result;
  }

  std::vector<IStorage*> StorageBuffer::UpdateDescriptorSet(VkDescriptorSet set, const std::vector<IStorage*> &data)
  {
    std::vector<IStorage*> result = data;
    std::vector<VkDescriptorBufferInfo> descriptor_buffer_infos(result.size());
    std::vector<VkWriteDescriptorSet> write_descriptor_set(result.size());
    
    for (std::size_t i = 0; i < result.size(); ++i)
    {
      VkDescriptorBufferInfo descriptor_buffer_info = {};
      descriptor_buffer_info.buffer = result[i]->src_buffer;
      descriptor_buffer_info.offset = 0;
      descriptor_buffer_info.range = VK_WHOLE_SIZE;
      descriptor_buffer_infos[i] = descriptor_buffer_info;

      VkWriteDescriptorSet write_descriptor = {};
      write_descriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write_descriptor.dstSet = set;
      write_descriptor.dstBinding = i;
      write_descriptor.descriptorCount = 1;
      
      switch (result[i]->Type())
      {
        case StorageType::Default :
          write_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
          break;
        case StorageType::Uniform :
          write_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
          break;
        default:
          std::runtime_error("Unsupported storage type");
      }

      write_descriptor.pBufferInfo = &descriptor_buffer_infos[i];
      write_descriptor_set[i] = write_descriptor;
    }

    vkUpdateDescriptorSets(device->GetDevice(), (uint32_t) write_descriptor_set.size(), write_descriptor_set.data(), 0, nullptr);

    return result;
  }

  DataLayout StorageBuffer::GetDataLayout(const std::vector<IStorage*> &data)
  {
    DataLayout result = { std::vector<StorageType>(data.size()), 0, 0};
    for (std::size_t i = 0; i < data.size(); ++i)
    {
      result.layout[i] = data[i]->Type();
      switch (data[i]->Type())
      {
        case StorageType::Default :
          result.storage_buffers++;
          break;
        case StorageType::Uniform :
          result.uniform_buffers++;
          break;
        default:
          std::runtime_error("Unsupported storage type");
      }
    }
    return result;
  }

  void* StorageBuffer::Extract(std::size_t &length, const std::size_t index)
  {
    if (index < buffers.size())
    {
      return buffers[index]->Extract(length);
    }
    return nullptr;
  }

  void StorageBuffer::UpdateValue(void *data_ptr, const std::size_t length, const std::size_t index)
  {
    if (index < buffers.size())
    {
      buffers[index]->Update(data_ptr, length);
    }
  }

  bool StorageBuffer::IsDataVectorValid(const std::vector<IStorage*> &data)
  {
    bool result = true;
    if (data.size() == 0) return false;
    VkDevice dev = data[0]->device->GetDevice();

    for (std::size_t i = 0; i < data.size(); ++i)
    {
      if (data[i]->device->GetDevice() != dev)
        return false;
    }

    return result;
  }
}