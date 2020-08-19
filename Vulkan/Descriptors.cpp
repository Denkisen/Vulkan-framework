#include "Descriptors.h"

#include <algorithm>

namespace Vulkan
{
  Vulkan::DescriptorType Descriptors::MapStorageType(Vulkan::StorageType type)
  {
    switch (type)
    {
      case Vulkan::StorageType::Storage:
      case Vulkan::StorageType::Index:
      case Vulkan::StorageType::Vertex:
        return Vulkan::DescriptorType::BufferStorage;
      case Vulkan::StorageType::Uniform:
        return Vulkan::DescriptorType::BufferUniform;
      default:
        throw std::runtime_error("Unknown StorageType");
    }
  }

  void Descriptors::Destroy()
  {
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      for (auto &layout : layouts)
      {
        if (layout.layout != VK_NULL_HANDLE)
        {
          vkDestroyDescriptorSetLayout(device->GetDevice(), layout.layout, nullptr);
          layout.layout = VK_NULL_HANDLE;
        }
      }
      layouts.clear();

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
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
      throw std::runtime_error("Devise is nullptr");
    device = dev;
  }

  VkDescriptorPool Descriptors::CreateDescriptorPool(const std::map<Vulkan::DescriptorType, uint32_t> pool_conf, const uint32_t max_sets)
  {
    VkDescriptorPool result = VK_NULL_HANDLE;

    if (pool_conf.empty())
      throw std::runtime_error("Error: pool_conf.empty()");
    
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (auto i = pool_conf.begin(); i != pool_conf.end(); ++i)
    {
      VkDescriptorPoolSize sz = {};
      sz.type = (VkDescriptorType) i->first;
      sz.descriptorCount = i->second;
      pool_sizes.push_back(sz);
    }

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = max_sets;
    descriptor_pool_create_info.pPoolSizes = pool_sizes.data();
    descriptor_pool_create_info.poolSizeCount = (uint32_t) pool_sizes.size();

    if (vkCreateDescriptorPool(device->GetDevice(), &descriptor_pool_create_info, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("Can't creat DescriptorPool.");

    return result;
  }

  DescriptorSetLayout Descriptors::CreateDescriptorSets(const VkDescriptorPool pool, const DescriptorSetLayout layout)
  {
    DescriptorSetLayout result = layout;
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &result.layout;

    if (vkAllocateDescriptorSets(device->GetDevice(), &alloc_info, &result.set) != VK_SUCCESS)
      throw std::runtime_error("failed to allocate descriptor sets!");

    return result;
  }

  DescriptorSetLayout Descriptors::CreateDescriptorSetLayout(const std::vector<DescriptorInfo> info)
  {
    DescriptorSetLayout result = {};
    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;
    for (auto &t : info)
    {
      VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
      descriptor_set_layout_binding.binding = t.binding;
      descriptor_set_layout_binding.descriptorType = (VkDescriptorType) t.type;
      descriptor_set_layout_binding.stageFlags = t.stage;
      descriptor_set_layout_binding.pImmutableSamplers = nullptr;
      descriptor_set_layout_binding.descriptorCount = 1;
      descriptor_set_layout_bindings.push_back(descriptor_set_layout_binding);
    }

    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.bindingCount = (uint32_t) descriptor_set_layout_bindings.size();
    descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings.data();

    if (vkCreateDescriptorSetLayout(device->GetDevice(), &descriptor_set_layout_create_info, nullptr, &result.layout) != VK_SUCCESS)
      throw std::runtime_error("Can't create DescriptorSetLayout.");

    return result;
  }

  void Descriptors::UpdateDescriptorSet(const DescriptorSetLayout layout, const std::vector<DescriptorInfo> info)
  {
    std::vector<VkWriteDescriptorSet> descriptor_writes(info.size());
    std::pair<uint32_t, uint32_t> count = std::make_pair(0, 0);
    for (auto &t : info)
    {
      switch (t.type)
      {
        case DescriptorType::BufferStorage:
        case DescriptorType::BufferUniform:
          count.first++;
          break;
        case DescriptorType::ImageSamplerCombined:
        case DescriptorType::ImageSampled:
        case DescriptorType::ImageStorage:
          count.second++;
          break;
        case DescriptorType::Sampler:
        default:
          throw std::runtime_error("No suitable objects for VkWriteDescriptorSet");
      }
    }
    std::vector<VkDescriptorBufferInfo> buffer_infos(count.first);
    std::vector<VkDescriptorImageInfo> image_infos(count.second);
    count = std::make_pair(0, 0);

    for (size_t i = 0; i < info.size(); ++i)
    { 
      descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      descriptor_writes[i].dstSet = layout.set;
      descriptor_writes[i].dstBinding = info[i].binding;
      descriptor_writes[i].dstArrayElement = 0;
      descriptor_writes[i].descriptorCount = 1;
      descriptor_writes[i].descriptorType = (VkDescriptorType) info[i].type;

      switch (info[i].type)
      {
        case DescriptorType::BufferStorage:
        case DescriptorType::BufferUniform:
          buffer_infos[count.first].buffer = info[i].buffer;
          buffer_infos[count.first].offset = 0;
          buffer_infos[count.first].range = VK_WHOLE_SIZE;

          descriptor_writes[i].pBufferInfo = &buffer_infos[count.first];
          descriptor_writes[i].pImageInfo = VK_NULL_HANDLE;
          count.first++;
          break;
        case DescriptorType::ImageSamplerCombined:
          image_infos[count.second].imageView = info[i].image_view;
          image_infos[count.second].imageLayout = info[i].image_layout;
          image_infos[count.second].sampler = info[i].sampler;

          descriptor_writes[i].pBufferInfo = VK_NULL_HANDLE;
          descriptor_writes[i].pImageInfo = &image_infos[count.second];
          count.second++;
          break;
        case DescriptorType::ImageSampled:
        case DescriptorType::ImageStorage:
          image_infos[count.second].imageView = info[i].image_view;
          image_infos[count.second].imageLayout = info[i].image_layout;
          image_infos[count.second].sampler = VK_NULL_HANDLE;

          descriptor_writes[i].pBufferInfo = VK_NULL_HANDLE;
          descriptor_writes[i].pImageInfo = &image_infos[count.second];
          count.second++;
          break;
        case DescriptorType::Sampler:
          throw std::runtime_error("Not implemented.");
        default:
          throw std::runtime_error("No suitable objects for VkWriteDescriptorSet");
      }
    }

    vkUpdateDescriptorSets(device->GetDevice(), (uint32_t) descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
  }

  void Descriptors::ClearDescriptorSetLayout(const uint32_t index)
  {
    if (index < build_info.size())
      build_info[index] = std::make_pair(true, std::vector<DescriptorInfo>());
    else
      build_info.resize(index + 1, std::make_pair(true, std::vector<DescriptorInfo>()));
  }

  void Descriptors::Add(const uint32_t set_index, const uint32_t binding, const std::shared_ptr<IBuffer> buffer, const VkShaderStageFlags stage)
  {
    if (set_index >= build_info.size())
      throw std::runtime_error("build_info count is less then " + std::to_string(set_index));

    if (build_info[set_index].first == false)
      throw std::runtime_error("Layout is not editable.");
    
    if (buffer->GetBuffer() == VK_NULL_HANDLE)
      throw std::runtime_error("Invalid buffer pointer.");

    DescriptorInfo info = {};
    info.buffer = buffer->GetBuffer();
    info.binding = binding;
    info.stage = stage;
    info.type = MapStorageType(buffer->Type());
    build_info[set_index].second.push_back(info);
  }

  void Descriptors::Add(const uint32_t set_index, const uint32_t binding, const std::shared_ptr<Image> image, const std::shared_ptr<Sampler> sampler, const VkShaderStageFlags stage)
  {
    if (set_index >= build_info.size())
      throw std::runtime_error("build_info count is less then " + std::to_string(set_index));
    
    if (build_info[set_index].first == false)
      throw std::runtime_error("Layout is not editable.");

    if (image->GetImageView() == VK_NULL_HANDLE)
      throw std::runtime_error("Invalid ImageView pointer.");

    DescriptorInfo info = {};
    info.image_view = image->GetImageView();
    info.image_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    info.stage = stage;
    info.binding = binding;
    if (image->Type() == ImageType::Sampled)
    {
      if (sampler->GetSampler() == VK_NULL_HANDLE)
      {
        info.type = DescriptorType::ImageSampled;
      }
      else
      {
        info.sampler = sampler->GetSampler();
        info.type = DescriptorType::ImageSamplerCombined;
      }
    }
    else
      info.type = DescriptorType::ImageStorage;

    build_info[set_index].second.push_back(info);
  }

  void Descriptors::BuildAll()
  {
    std::map<Vulkan::DescriptorType, uint32_t> config;
    for (auto &info : build_info)
    {
      if (info.first == true && !info.second.empty())
      {
        for (auto &item : info.second)
        {
          auto it = config.find(item.type);
          if (it == config.end())
            config.insert(std::make_pair(item.type, 1));
          else
            it->second += 1;
        }
      }
    }

    if (config.empty()) return;
    Destroy();
    pool_config = config;
    descriptor_pool = CreateDescriptorPool(pool_config, build_info.size());

    for (size_t i = 0; i < build_info.size(); ++i)
    {
      if (build_info[i].first == true && !build_info[i].second.empty())
      {
        auto layout = CreateDescriptorSetLayout(build_info[i].second);
        layout = CreateDescriptorSets(descriptor_pool, layout);
        UpdateDescriptorSet(layout, build_info[i].second);
        layouts.push_back(layout);
        build_info[i].first = false;
      }
    }
  }

  std::vector<VkDescriptorSetLayout> Descriptors::GetDescriptorSetLayouts() const
  {
    std::vector<VkDescriptorSetLayout> result(layouts.size());
    for (size_t i = 0; i < result.size(); ++i)
      result[i] = layouts[i].layout;
    return result;
  }

  std::vector<VkDescriptorSet> Descriptors::GetDescriptorSets() const
  {
    std::vector<VkDescriptorSet> result(layouts.size());
    for (size_t i = 0; i < result.size(); ++i)
      result[i] = layouts[i].set;
    return result;
  }
}