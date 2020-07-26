#include "Descriptors.h"

namespace Vulkan
{
  void Descriptors::Destroy()
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      for (auto &info : buffers_info)
      {
        for (auto descriptor_set_layout : info.layouts)
        {
          if (descriptor_set_layout != VK_NULL_HANDLE)
          {
            vkDestroyDescriptorSetLayout(device->GetDevice(), descriptor_set_layout, nullptr);
            descriptor_set_layout = VK_NULL_HANDLE;
          }
        }
      }
      buffers_info.clear();

      if (descriptor_pool != VK_NULL_HANDLE)
      {
        vkDestroyDescriptorPool(device->GetDevice(), descriptor_pool, nullptr);
        descriptor_pool = VK_NULL_HANDLE;
      }
    }
  }

  Descriptors::~Descriptors()
  {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
    Destroy();
    device.reset();
  }

  Descriptors::Descriptors(std::shared_ptr<Vulkan::Device> dev)
  {
    if (dev == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
      throw std::runtime_error("Devise is nullptr");
    device = dev;
  }

  VkDescriptorPool Descriptors::CreateDescriptorPool()
  {
    VkDescriptorPool result = VK_NULL_HANDLE;

    if (pool_config.empty())
      throw std::runtime_error("Error: pool_config.empty()");
    
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (auto i = pool_config.begin(); i != pool_config.end(); ++i)
    {
      VkDescriptorPoolSize sz = {};
      sz.type = Vulkan::Supply::StorageTypeToDescriptorType(i->first);
      sz.descriptorCount = i->second;
      pool_sizes.push_back(sz);
    }

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = sets_to_allocate;
    descriptor_pool_create_info.pPoolSizes = pool_sizes.data();
    descriptor_pool_create_info.poolSizeCount = (uint32_t) pool_sizes.size();

    if (vkCreateDescriptorPool(device->GetDevice(), &descriptor_pool_create_info, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("Can't creat DescriptorPool.");

    return result;
  }

  void Descriptors::CreateDescriptorSetLayouts(DescriptorInfo &info)
  {
    if (info.multiple_layouts_one_binding)
    {
      info.layouts.resize(info.buffers.size());
      for (size_t i = 0; i < info.layouts.size(); ++i)
      {
        VkDescriptorSetLayoutBinding descriptor_set_layout_binding;
        descriptor_set_layout_binding.binding = 0;
        descriptor_set_layout_binding.descriptorType = Vulkan::Supply::StorageTypeToDescriptorType(info.types[i]);
        descriptor_set_layout_binding.descriptorCount = 1;
        descriptor_set_layout_binding.stageFlags = info.stage;
        
        VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
        descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        descriptor_set_layout_create_info.bindingCount = 1;
        descriptor_set_layout_create_info.pBindings = &descriptor_set_layout_binding;

        if (vkCreateDescriptorSetLayout(device->GetDevice(), &descriptor_set_layout_create_info, nullptr, &info.layouts[i]) != VK_SUCCESS)
          throw std::runtime_error("Can't create DescriptorSetLayout.");
      }
    }
    else
    {
      info.layouts.resize(1);
      std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(info.buffers.size());

      for (size_t i = 0; i < descriptor_set_layout_bindings.size(); ++i)
      {
        descriptor_set_layout_bindings[i].binding = i;
        descriptor_set_layout_bindings[i].descriptorType = Vulkan::Supply::StorageTypeToDescriptorType(info.types[i]);
        descriptor_set_layout_bindings[i].descriptorCount = 1;
        descriptor_set_layout_bindings[i].stageFlags = info.stage;
      }

      VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
      descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      descriptor_set_layout_create_info.bindingCount = (uint32_t) descriptor_set_layout_bindings.size();
      descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings.data();

      if (vkCreateDescriptorSetLayout(device->GetDevice(), &descriptor_set_layout_create_info, nullptr, &info.layouts[0]) != VK_SUCCESS)
        throw std::runtime_error("Can't create DescriptorSetLayout.");
    }

    sets_to_allocate += info.layouts.size();

    for (size_t i = 0; i < info.types.size(); ++i)
    {
      auto it = pool_config.find(info.types[i]);
      if (it == pool_config.end())
      {
        pool_config.insert(std::make_pair(info.types[i], 1));
      }
      else
      {
        it->second += 1;
      }
    }
  }

  void Descriptors::CreateDescriptorSets(DescriptorInfo &info)
  {
    info.sets.resize(info.layouts.size());
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = descriptor_pool;
    alloc_info.descriptorSetCount = (uint32_t) info.layouts.size();
    alloc_info.pSetLayouts = info.layouts.data();

    if (vkAllocateDescriptorSets(device->GetDevice(), &alloc_info, info.sets.data()) != VK_SUCCESS)
      throw std::runtime_error("failed to allocate descriptor sets!");
  }

  void Descriptors::UpdateDescriptorSet(DescriptorInfo &info)
  {
    std::vector<VkDescriptorBufferInfo> buffer_infos;
    std::vector<VkWriteDescriptorSet> descriptor_writes;

    buffer_infos.resize(info.buffers.size());
    descriptor_writes.resize(info.buffers.size());
    for (size_t i = 0; i < info.buffers.size(); ++i)
    {
      buffer_infos[i].buffer = info.buffers[i];
      buffer_infos[i].offset = 0;
      buffer_infos[i].range = VK_WHOLE_SIZE;

      descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      if (info.multiple_layouts_one_binding)
      {
        descriptor_writes[i].dstSet = info.sets[i];
        descriptor_writes[i].dstBinding = 0;
      }
      else
      {
        descriptor_writes[i].dstSet = info.sets[0];
        descriptor_writes[i].dstBinding = i;
      }
      descriptor_writes[i].dstArrayElement = 0; // index in binding
      descriptor_writes[i].descriptorCount = 1; //  <= binding.descriptorCount
      descriptor_writes[i].descriptorType = Vulkan::Supply::StorageTypeToDescriptorType(info.types[i]);
      descriptor_writes[i].pBufferInfo = &buffer_infos[i]; // array of descriptorCount
    }

    vkUpdateDescriptorSets(device->GetDevice(), (uint32_t) descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
  }

  void Descriptors::Add(std::vector<std::shared_ptr<IStorage>> data, VkShaderStageFlags stage, bool multiple_layouts_one_binding)
  {
    build_buffers_info.push_back({});
    build_buffers_info[build_buffers_info.size() - 1].stage = stage;
    build_buffers_info[build_buffers_info.size() - 1].multiple_layouts_one_binding = multiple_layouts_one_binding;
    build_buffers_info[build_buffers_info.size() - 1].buffers.resize(data.size());
    build_buffers_info[build_buffers_info.size() - 1].types.resize(data.size());

    for (size_t i = 0; i < data.size(); ++i)
    {
      build_buffers_info[build_buffers_info.size() - 1].buffers[i] = data[i]->GetBuffer();
      build_buffers_info[build_buffers_info.size() - 1].types[i] = data[i]->Type();
    }
  }

  void Descriptors::Clear()
  {
    build_buffers_info.clear();
  }

  void Descriptors::Build()
  {
    Destroy();
    pool_config.clear();
    buffers_info = build_buffers_info;

    for (size_t i = 0; i < buffers_info.size(); ++i)
    {
      CreateDescriptorSetLayouts(buffers_info[i]);
    }

    descriptor_pool = CreateDescriptorPool();

    for (size_t i = 0; i < buffers_info.size(); ++i)
    {
      CreateDescriptorSets(buffers_info[i]);
      UpdateDescriptorSet(buffers_info[i]);
    }
  }
}