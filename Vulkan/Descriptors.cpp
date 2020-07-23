#include "Descriptors.h"

namespace Vulkan
{
  void Descriptors::Destroy()
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      for (auto &l : descriptor_set_layouts)
      {
        if (l != VK_NULL_HANDLE)
        {
          vkDestroyDescriptorSetLayout(device->GetDevice(), l, nullptr);
          l = VK_NULL_HANDLE;
        }
      }
      descriptor_set_layouts.clear();

      if (descriptor_pool != VK_NULL_HANDLE)
      {
        vkDestroyDescriptorPool(device->GetDevice(), descriptor_pool, nullptr);
        descriptor_pool = VK_NULL_HANDLE;
      }
    }
    device.reset();
  }

  Descriptors::~Descriptors()
  {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
    Destroy();
  }

  Descriptors::Descriptors(std::shared_ptr<Vulkan::Device> dev)
  {
    if (dev == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
      throw std::runtime_error("Devise is nullptr");
    device = dev;
  }

  VkDescriptorPool Descriptors::CreateDescriptorPool(SDataLayout layout)
  {
    VkDescriptorPool result = VK_NULL_HANDLE;

    if (layout.buffers.empty())
      throw std::runtime_error("Error: layout.buffers.empty()");
    
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (auto i = layout.buffers.begin(); i != layout.buffers.end(); ++i)
    {
      VkDescriptorPoolSize sz = {};
      sz.type = Vulkan::Supply::StorageTypeToDescriptorType(i->first);
      sz.descriptorCount = i->second;
      pool_sizes.push_back(sz);
    }

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = descriptor_pool_capacity;
    descriptor_pool_create_info.pPoolSizes = pool_sizes.data();
    descriptor_pool_create_info.poolSizeCount = (uint32_t) pool_sizes.size();

    if (vkCreateDescriptorPool(device->GetDevice(), &descriptor_pool_create_info, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("Can't creat DescriptorPool.");

    return result;
  }

  std::vector<VkDescriptorSetLayout> Descriptors::CreateDescriptorSetLayout(SDataLayout data_layout, bool layout_per_set)
  {
    std::vector<VkDescriptorSetLayout> result;
    std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;

    if (data_layout.layout.empty())
      throw std::runtime_error("Error: layout.buffers.empty()");

    if (!layout_per_set)
    {
      descriptor_set_layout_bindings.resize(data_layout.layout.size());
      for (std::size_t i = 0; i < data_layout.layout.size(); ++i)
      {
        descriptor_set_layout_bindings[i].binding = i;
        descriptor_set_layout_bindings[i].descriptorType = Vulkan::Supply::StorageTypeToDescriptorType(std::get<0>(data_layout.layout[i]));
        descriptor_set_layout_bindings[i].stageFlags = std::get<1>(data_layout.layout[i]);
        descriptor_set_layout_bindings[i].descriptorCount = std::get<2>(data_layout.layout[i]);
      }
    }
    else
    {
      descriptor_set_layout_bindings.resize(1);
      descriptor_set_layout_bindings[0].binding = 0;
      descriptor_set_layout_bindings[0].descriptorType = Vulkan::Supply::StorageTypeToDescriptorType(std::get<0>(data_layout.layout[0]));
      descriptor_set_layout_bindings[0].stageFlags = std::get<1>(data_layout.layout[0]);
      descriptor_set_layout_bindings[0].descriptorCount = std::get<2>(data_layout.layout[0]);
      for (std::size_t i = 1; i < data_layout.layout.size(); ++i)
      {
        if (std::get<1>(data_layout.layout[i]) != descriptor_set_layout_bindings[0].stageFlags && 
          descriptor_set_layout_bindings[0].stageFlags != VK_SHADER_STAGE_ALL_GRAPHICS)
        {
          descriptor_set_layout_bindings[0].stageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        }
        if (descriptor_set_layout_bindings[0].descriptorType != Vulkan::Supply::StorageTypeToDescriptorType(std::get<0>(data_layout.layout[i])))
          throw std::runtime_error("All buffers in one set layout must have same descriptorType");
      }
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.bindingCount = (uint32_t) descriptor_set_layout_bindings.size();
    descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings.data();

    VkDescriptorSetLayout tmp = VK_NULL_HANDLE;
    if (vkCreateDescriptorSetLayout(device->GetDevice(), &descriptor_set_layout_create_info, nullptr, &tmp) != VK_SUCCESS)
      throw std::runtime_error("Can't create DescriptorSetLayout.");

    if (layout_per_set)
    {
      for (size_t i = 0; i < data_layout.layout.size(); ++i)
      {
        result.push_back(tmp);
      }
    }
    else
    {
      result.push_back(tmp);
    }

    return result;
  }

  std::vector<VkDescriptorSet> Descriptors::CreateDescriptorSets(VkDescriptorPool pool, std::vector<VkDescriptorSetLayout> layouts)
  {
    std::vector<VkDescriptorSet> result(descriptor_pool_capacity);

    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO; 
    descriptor_set_allocate_info.descriptorPool = pool;
    descriptor_set_allocate_info.descriptorSetCount = descriptor_pool_capacity;
    descriptor_set_allocate_info.pSetLayouts = layouts.data();

    if (vkAllocateDescriptorSets(device->GetDevice(), &descriptor_set_allocate_info, result.data()) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate DescriptorSets.");

    return result;
  }

  void Descriptors::Add(std::shared_ptr<IStorage> buffer, VkShaderStageFlagBits stage)
  {
    buffers.push_back(std::make_tuple(buffer->Type(), stage, 1));
  }

  void Descriptors::Clear()
  {
    buffers.clear();
  }

  void Descriptors::Build(bool set_per_buffer)
  {
    SDataLayout layout;
    descriptor_pool_capacity = set_per_buffer ? 0 : 1;
    for (size_t i = 0; i < buffers.size(); ++i)
    {
      layout.layout.push_back(buffers[i]);
      auto it = layout.buffers.find(std::get<0>(buffers[i]));
      if (it != layout.buffers.end())
      {
        it->second++;
      }
      else
      {
        layout.buffers.insert(std::make_pair(std::get<0>(buffers[i]), 1));
      }
      if (set_per_buffer)
        descriptor_pool_capacity++;
    }

    Destroy();
    descriptor_sets.clear();
    descriptor_pool = CreateDescriptorPool(layout);
    descriptor_set_layouts = CreateDescriptorSetLayout(layout, set_per_buffer);
    descriptor_sets = CreateDescriptorSets(descriptor_pool, descriptor_set_layouts);
  }
}