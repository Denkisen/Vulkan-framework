#include "Descriptors.h"
#include "Logger.h"

namespace Vulkan
{

  Descriptors_impl::~Descriptors_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
    Destroy();
  }

  void Descriptors_impl::Destroy() noexcept
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

    build_config_copy.clear();
  }

  Descriptors_impl::Descriptors_impl(std::shared_ptr<Device> dev) noexcept
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    device = dev;
  }

  DescriptorType DescriptorInfo::MapStorageType(StorageType type) noexcept
  {
    switch (type)
    {
      case StorageType::Storage:
      case StorageType::Index:
      case StorageType::Vertex:
        return DescriptorType::BufferStorage;
      case StorageType::Uniform:
        return DescriptorType::BufferUniform;
      case StorageType::TexelStorage:
        return DescriptorType::TexelStorage;
      case StorageType::TexelUniform:
        return DescriptorType::TexelUniform;
    }

    return DescriptorType::BufferStorage;
  }

  VkDescriptorPool Descriptors_impl::CreateDescriptorPool(const PoolConfig &pool_conf) noexcept
  {
    VkDescriptorPool result = VK_NULL_HANDLE;
    if (pool_conf.sizes.empty())
    {
      Logger::EchoWarning("Pool config is empty", __func__);
      return result;
    }

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = pool_conf.max_sets;
    descriptor_pool_create_info.pPoolSizes = pool_conf.sizes.data();
    descriptor_pool_create_info.poolSizeCount = (uint32_t) pool_conf.sizes.size();

    auto er = vkCreateDescriptorPool(device->GetDevice(), &descriptor_pool_create_info, nullptr, &result);

    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't create descriptor pool", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }

    return result;
  }

  DescriptorSetLayout Descriptors_impl::CreateDescriptorSetLayout(const LayoutConfig &info) noexcept
  {
    DescriptorSetLayout result = {};
    try
    {
      VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
      std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings;
      descriptor_set_layout_bindings.reserve(info.info.size());

      for (size_t i = 0; i < info.info.size(); ++i)
      {
        VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
        descriptor_set_layout_binding.binding = i;
        descriptor_set_layout_binding.descriptorType = (VkDescriptorType)info.info[i].type;
        descriptor_set_layout_binding.stageFlags = info.info[i].stage;
        descriptor_set_layout_binding.pImmutableSamplers = nullptr;
        descriptor_set_layout_binding.descriptorCount = 1;
        descriptor_set_layout_bindings.push_back(descriptor_set_layout_binding);
      }

      descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
      descriptor_set_layout_create_info.bindingCount = (uint32_t)descriptor_set_layout_bindings.size();
      descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings.data();

      auto er = vkCreateDescriptorSetLayout(device->GetDevice(), &descriptor_set_layout_create_info, nullptr, &result.layout);
      if (er != VK_SUCCESS)
      {
        Logger::EchoError("Can't create DescriptorSetLayout.", __func__);
        Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      }
    }
    catch (...) { }

    return result;
  }

  VkResult Descriptors_impl::CreateDescriptorSets(const VkDescriptorPool pool, DescriptorSetLayout &layout) noexcept
  {
    VkDescriptorSetAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool = pool;
    alloc_info.descriptorSetCount = 1;
    alloc_info.pSetLayouts = &layout.layout;

    auto er = vkAllocateDescriptorSets(device->GetDevice(), &alloc_info, &layout.set);
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Failed to allocate descriptor sets", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
      return er;
    }

    return VK_SUCCESS;
  }

  VkResult Descriptors_impl::UpdateDescriptorSet(const DescriptorSetLayout &layout, const LayoutConfig &info) noexcept
  {
    try
    {
      std::vector<VkWriteDescriptorSet> descriptor_writes(info.info.size());
      std::pair<uint32_t, uint32_t> count = std::make_pair(0, 0);
      for (auto &t : info.info)
      {
        switch (t.type)
        {
          case DescriptorType::BufferStorage:
          case DescriptorType::BufferUniform:
          case DescriptorType::TexelUniform:
          case DescriptorType::TexelStorage:
            count.first++;
            break;
          case DescriptorType::ImageSamplerCombined:
          case DescriptorType::ImageSampled:
          case DescriptorType::ImageStorage:
            count.second++;
            break;
          case DescriptorType::Sampler:
          default:
            Logger::EchoError("No suitable objects for VkWriteDescriptorSet", __func__);
            return VK_ERROR_UNKNOWN;
        }
      }

      std::vector<VkDescriptorBufferInfo> buffer_infos(count.first);
      std::vector<VkDescriptorImageInfo> image_infos(count.second);
      count = std::make_pair(0, 0);

      for (size_t i = 0; i < info.info.size(); ++i)
      {
        descriptor_writes[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptor_writes[i].dstSet = layout.set;
        descriptor_writes[i].dstBinding = i;
        descriptor_writes[i].dstArrayElement = 0;
        descriptor_writes[i].descriptorCount = 1;
        descriptor_writes[i].descriptorType = (VkDescriptorType)info.info[i].type;

        switch (info.info[i].type)
        {
          case DescriptorType::BufferStorage:
          case DescriptorType::BufferUniform:
          case DescriptorType::TexelUniform:
          case DescriptorType::TexelStorage:
            buffer_infos[count.first].buffer = info.info[i].buffer_info.buffer;
            buffer_infos[count.first].offset = info.info[i].offset;
            buffer_infos[count.first].range = info.info[i].size;

            descriptor_writes[i].pBufferInfo = &buffer_infos[count.first];
            descriptor_writes[i].pImageInfo = VK_NULL_HANDLE;
            descriptor_writes[i].pTexelBufferView = &info.info[i].buffer_info.buffer_view;
            count.first++;
            break;
          case DescriptorType::ImageSamplerCombined:
            image_infos[count.second].imageView = info.info[i].image_info.image_view;
            image_infos[count.second].imageLayout = info.info[i].image_info.image_layout;
            image_infos[count.second].sampler = info.info[i].image_info.sampler;

            descriptor_writes[i].pBufferInfo = VK_NULL_HANDLE;
            descriptor_writes[i].pImageInfo = &image_infos[count.second];
            count.second++;
            break;
          case DescriptorType::ImageSampled:
          case DescriptorType::ImageStorage:
            image_infos[count.second].imageView = info.info[i].image_info.image_view;
            image_infos[count.second].imageLayout = info.info[i].image_info.image_layout;
            image_infos[count.second].sampler = VK_NULL_HANDLE;

            descriptor_writes[i].pBufferInfo = VK_NULL_HANDLE;
            descriptor_writes[i].pImageInfo = &image_infos[count.second];
            count.second++;
            break;
          case DescriptorType::Sampler:
            Logger::EchoError("No suitable objects for VkWriteDescriptorSet", __func__);
            return VK_ERROR_UNKNOWN;
        }
      }

      vkUpdateDescriptorSets(device->GetDevice(), (uint32_t)descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
    }
    catch (...)
    {
      return VK_ERROR_UNKNOWN;
    }
  
    return VK_SUCCESS;
  }

  VkResult Descriptors_impl::AddSetLayoutConfig(const LayoutConfig &config) noexcept
  {
    std::lock_guard lock(build_mutex);
    if (config.info.empty())
    {
      Logger::EchoWarning("Nothing to add", __func__);
      return VK_SUCCESS;
    }

    try 
    { 
      build_config.push_back(config); 
    } 
    catch (...)
    {
      return VK_ERROR_UNKNOWN;
    }

    return VK_SUCCESS;
  }

  VkResult Descriptors_impl::BuildAllSetLayoutConfigs() noexcept
  {
    std::lock_guard lock(build_mutex);
    PoolConfig conf;

    for (auto &b : build_config)
    {
      for (auto &c : b.info)
      {
        conf.AddDescriptorType(c.type);
      }
    }

    if (conf.sizes.empty())
    {
      Logger::EchoWarning("Nothing to build", __func__);
      return VK_SUCCESS;
    }

    conf.max_sets = (uint32_t) build_config.size();

    auto tmp_desc_pool = CreateDescriptorPool(conf);
    if (tmp_desc_pool == VK_NULL_HANDLE)
    {
      Logger::EchoError("Pool not ready", __func__);
      return VK_ERROR_UNKNOWN;
    }

    try 
    {
      std::vector<DescriptorSetLayout> tmp_layouts;
      tmp_layouts.reserve(build_config.size());
      for (auto &b : build_config)
      {
        auto layout = CreateDescriptorSetLayout(b);
        auto er = CreateDescriptorSets(tmp_desc_pool, layout);
        if (er != VK_SUCCESS)
        {
          Logger::EchoError("Can't create CreateDescriptorSets", __func__);
          return er;
        }

        er = UpdateDescriptorSet(layout, b);
        if (er != VK_SUCCESS)
        {
          Logger::EchoError("Can't update CreateDescriptorSets", __func__);
          return er;
        }

        tmp_layouts.push_back(layout);
      }

      std::lock_guard lock1(layouts_mutex);
      Destroy();
      descriptor_pool = tmp_desc_pool;
      layouts.swap(tmp_layouts);
      build_config_copy.swap(build_config);
      build_config.clear();
    }
    catch (...)
    {
      return VK_ERROR_UNKNOWN;
    }

    return VK_SUCCESS;
  }

  void Descriptors_impl::ClearAllSetLayoutConfigs() noexcept
  {
    std::lock_guard lock(build_mutex);
    build_config.clear();
  }

  std::vector<VkDescriptorSetLayout> Descriptors_impl::GetDescriptorSetLayouts() noexcept
  {
    std::lock_guard lock(layouts_mutex);
    try
    {
      std::vector<VkDescriptorSetLayout> result(layouts.size());
      for (size_t i = 0; i < result.size(); ++i)
        result[i] = layouts[i].layout;
      return result;
    }
    catch (...)
    {
      return {};
    }
  }

  std::vector<VkDescriptorSet> Descriptors_impl::GetDescriptorSets() noexcept
  {
    std::lock_guard lock(layouts_mutex);
    try
    {
      std::vector<VkDescriptorSet> result(layouts.size());
      for (size_t i = 0; i < result.size(); ++i)
        result[i] = layouts[i].set;
      return result;
    }
    catch (...)
    {
      return {};
    }
  }

  Descriptors &Descriptors::operator=(const Descriptors &obj) noexcept
  {
    if (&obj == this) return *this;

    if (obj.impl.get() == nullptr)
    {
      Logger::EchoError("Object is empty", __func__);
      return *this;
    }
    
    std::vector<LayoutConfig> back;
    {
      std::scoped_lock lock(obj.impl->layouts_mutex, impl->layouts_mutex, impl->build_mutex);

      if (obj.impl->layouts.empty() || obj.impl->build_config_copy.empty())
        return *this;

      back.swap(impl->build_config);
      try
      {
        impl->build_config = obj.impl->build_config_copy;
      }
      catch (...)
      {
        impl->build_config.swap(back);
        return *this;
      }
    }

    auto er = impl->BuildAllSetLayoutConfigs();
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't copy descriptors layout", __func__);
      std::lock_guard lock1(impl->build_mutex);
      impl->build_config.swap(back);
    }
      
    return *this;
  }

  Descriptors::Descriptors(const Descriptors &obj) noexcept
  {
    if (obj.impl.get() == nullptr)
    {
      Logger::EchoError("Object is empty", __func__);
      return;
    }

    impl = std::unique_ptr<Descriptors_impl>(new Descriptors_impl(obj.impl->device));

    {
      std::lock_guard lock(obj.impl->layouts_mutex);
      if (obj.impl->layouts.empty() || obj.impl->build_config_copy.empty())
      return;

      try
      {
        impl->build_config = obj.impl->build_config_copy;
      }
      catch (...)
      {
        return;
      }
    }
    
    auto er = impl->BuildAllSetLayoutConfigs();
    if (er != VK_SUCCESS)
      Logger::EchoError("Can't copy descriptors layout", __func__);
  }

  Descriptors &Descriptors::operator=(Descriptors &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);

    return *this;
  }

  void Descriptors::swap(Descriptors &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(Descriptors &lhs, Descriptors &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }
}