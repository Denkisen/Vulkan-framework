#include "Device.h"

namespace Vulkan
{
  Device_impl::~Device_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
    if (device != VK_NULL_HANDLE)
    {
      vkDestroyDevice(device, nullptr);
      device = VK_NULL_HANDLE;
    }
  }

  VkDeviceSize Device_impl::GetPhisicalDevicesCount()
  {
    uint32_t device_count = 0;
    vkEnumeratePhysicalDevices(Instance::GetInstance(), &device_count, nullptr);
    return (VkDeviceSize) device_count;
  }

  std::vector<VkPhysicalDevice> Device_impl::GetAllPhysicalDevices()
  {
    std::vector<VkPhysicalDevice> ret(GetPhisicalDevicesCount(), VK_NULL_HANDLE);
    uint32_t count = (uint32_t) ret.size();
    if (ret.empty() || vkEnumeratePhysicalDevices(Instance::GetInstance(), &count, ret.data()) != VK_SUCCESS) 
      ret.clear();

    return ret;
  }

  std::vector<std::string> Device_impl::GetPhysicalDeviceExtensions(const VkPhysicalDevice &device)
  {
    uint32_t count = 0;
    std::vector<std::string> ret;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS)
      return ret;

    std::vector<VkExtensionProperties> available_extensions(count);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, available_extensions.data()) != VK_SUCCESS)
      return ret;
    
    for (size_t i = 0; i < count; ++i)
    {
      ret.push_back(available_extensions[i].extensionName);
    }

    return ret;
  }

  std::vector<Queue> Device_impl::FindFamilyQueues() const
  {
    std::vector<Queue> ret;
    uint32_t family_queues_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(p_device.device, &family_queues_count, nullptr);

    if (family_queues_count == 0)
      return ret;

    std::vector<VkQueueFamilyProperties> queue_families(family_queues_count);
    vkGetPhysicalDeviceQueueFamilyProperties(p_device.device, &family_queues_count, queue_families.data());

    ret.resize(3);
    ret[0].purpose = QueuePurpose::GraphicPurpose;
    ret[1].purpose = QueuePurpose::PresentationPurpose;
    ret[2].purpose = QueuePurpose::ComputePurpose;

    for (VkDeviceSize i = 0; i < family_queues_count; ++i)
    {
      if (queue_families[i].queueFlags & (VkQueueFlags)QueueType::ComputeType && !ret[2].family.has_value())
      {
        ret[2].family = i;
        ret[2].props = queue_families[i];
        ret[2].queue_priority = 1.0f;
      }

      if (queue_families[i].queueFlags & (VkQueueFlags)QueueType::DrawingType)
      {
        VkBool32 present = false;
        if (!ret[0].family.has_value())
        {
          ret[0].family = i;
          ret[0].props = queue_families[i];
          ret[0].queue_priority = 1.0f;
        }

        if (queue_flag_bits != QueueType::ComputeType && vkGetPhysicalDeviceSurfaceSupportKHR(p_device.device, i, surface->GetSurface(), &present) != VK_SUCCESS)
        {
          Logger::EchoError("Can't check surface support", __func__);
          ret.clear();
          return ret;
        }

        if (present && !ret[1].family.has_value())
        {
          ret[1].family = i;
          ret[1].props = queue_families[i];
          ret[1].queue_priority = 1.0f;
        }
      }

      bool done = true;
      for (size_t j = 0; j < 3; ++j)
      {
        if (queue_flag_bits == QueueType::ComputeType)
        {
          if (j == 0 && j == 1)
            continue;
        }
        if (queue_flag_bits == QueueType::DrawingType)
        {
          if (j == 2)
            continue;
        }
        if (!ret[i].family.has_value())
          done = false;
      }
      if (done)
        break;
    }

    if (queue_flag_bits == QueueType::ComputeType)
    {
      if (!ret[2].family.has_value())
        ret.clear();
    }
    if (queue_flag_bits == QueueType::DrawingType)
    {
      if (!ret[0].family.has_value() || !ret[1].family.has_value())
        ret.clear();
    }
    if (queue_flag_bits == QueueType::DrawingAndComputeType)
    {
      if (!ret[0].family.has_value() || !ret[1].family.has_value() || !ret[2].family.has_value())
        ret.clear();
    }

    return ret;
  }

  Device_impl::Device_impl(const DeviceConfig params)
  {
    surface = params.surface;
    queue_flag_bits = params.queue_flags;

    auto devices = GetAllPhysicalDevices();

    if (devices.empty())
    {
      Logger::EchoError("No devices has found", __func__);
      return;
    };

    // Prio: Name > index > type
    if (!params.device_name.empty())
    {
      Logger::EchoDebug("Looking for device with name = " + params.device_name, __func__);
      for (VkDeviceSize i = 0; i < devices.size(); ++i)
      {
        PhysicalDevice p;
        p.device = devices[i];
        vkGetPhysicalDeviceProperties(p.device, &p.device_properties);
        vkGetPhysicalDeviceFeatures(p.device, &p.device_features);
        p.device_index = i;
        if (params.device_name == std::string(p.device_properties.deviceName))
        {
          p_device = p;
          Logger::EchoInfo("Found device with index = " + std::to_string(i), __func__);
          auto dev = Create(params.p_device_features);
          if (dev != VK_NULL_HANDLE)
          {
            device = dev;
            break;
          }
          else
          {
            Logger::EchoWarning("Can't create device with index = " + std::to_string(i), __func__);
            p_device = {};
          }
        }
      }
    }

    if (device == VK_NULL_HANDLE && params.device_index.has_value())
    {
      if (params.device_index >= devices.size())
        Logger::EchoDebug("No device with index = " + std::to_string(params.device_index.value()), __func__);
      else
      {
        PhysicalDevice p;
        p.device = devices[params.device_index.value()];
        vkGetPhysicalDeviceProperties(p.device, &p.device_properties);
        vkGetPhysicalDeviceFeatures(p.device, &p.device_features);
        p.device_index = params.device_index.value();
        p_device = p;
        Logger::EchoInfo("Found device with index = " + std::to_string(p.device_index), __func__);
        auto dev = Create(params.p_device_features);
        if (dev != VK_NULL_HANDLE)
        {
          device = dev;
        }
        else
        {
          Logger::EchoWarning("Can't create device with index = " + std::to_string(p.device_index), __func__);
          p_device = {};
        }
      }
    }

    if (device == VK_NULL_HANDLE)
    {
      std::multimap<VkDeviceSize, PhysicalDevice> ranking;
      auto adv = ranking.end();
      for (size_t i = 0; i < devices.size(); ++i)
      {
        PhysicalDevice p;
        VkDeviceSize rank = 0;
        p.device = devices[i];
        p.device_index = i;
        vkGetPhysicalDeviceProperties(p.device, &p.device_properties);
        vkGetPhysicalDeviceFeatures(p.device, &p.device_features);

        if ((VkPhysicalDeviceType)params.p_device_type != p.device_properties.deviceType)
          continue;

        switch (queue_flag_bits)
        {
        case QueueType::ComputeType:
          rank++;
          rank += p.device_properties.limits.maxComputeSharedMemorySize;
          break;
        case QueueType::DrawingAndComputeType:
          rank += p.device_properties.limits.maxComputeSharedMemorySize;
        case QueueType::DrawingType:
          if (!p.device_features.geometryShader || !p.device_features.samplerAnisotropy)
          {
            rank = 0;
            continue;
          }
          rank++;
          rank += p.device_properties.limits.maxImageDimension2D;
          break;
        }
        rank += p.device_properties.limits.maxUniformBufferRange;
        rank += p.device_properties.limits.maxStorageBufferRange;
        rank += p.device_properties.limits.maxMemoryAllocationCount;
        rank += p.device_properties.limits.maxBoundDescriptorSets;

        adv = ranking.insert(adv, std::pair<VkDeviceSize, PhysicalDevice>(rank, p));
      }

      if (ranking.empty())
      {
        Logger::EchoError("No devices has found", __func__);
        return;
      }

      for (auto& it : ranking)
      {
        p_device = it.second;

        auto dev = Create(params.p_device_features);
        if (dev != VK_NULL_HANDLE)
        {
          device = dev;
          break;
        }
        else
        {
          Logger::EchoWarning("Can't create device with index = " + std::to_string(p_device.device_index), __func__);
          p_device = {};
        }
      }
    }

    if (device == VK_NULL_HANDLE || p_device.device == VK_NULL_HANDLE)
    {
      p_device = {};
      device = VK_NULL_HANDLE;
      Logger::EchoError("No suitable devices", __func__);
    }
  }

  VkDevice Device_impl::Create(const VkPhysicalDeviceFeatures features)
  {
    VkDevice res = VK_NULL_HANDLE;

    for (VkDeviceSize i = 0; i < sizeof(VkPhysicalDeviceFeatures) / sizeof(VkBool32); ++i)
    {
      VkBool32 req_val = ((VkBool32*)&features)[i];
      VkBool32 p_val = ((VkBool32*)&p_device.device_features)[i];
      if (req_val && !p_val)
      {
        Logger::EchoError("Device has no feature", __func__);
        return res;
      }
    }

    if (queue_flag_bits == QueueType::DrawingType || queue_flag_bits == QueueType::DrawingAndComputeType)
    {
      auto ext = GetPhysicalDeviceExtensions(p_device.device);
      for (auto s : Misc::RequiredGraphicDeviceExtensions)
      {
        bool found = false;
        for (auto& e : ext)
        {
          if (std::string(e) == s)
          {
            found = true;
            break;
          }
        }
        if (!found)
        {
          Logger::EchoError("Extension (" + std::string(s) + ") not supported", __func__);
          return res;
        }
      }

      auto swap_chain_details = Misc::GetSwapChainDetails(p_device.device, surface->GetSurface());
      if (swap_chain_details.formats.empty() || swap_chain_details.present_modes.empty())
      {
        Logger::EchoError("Swap chain does not support any formats or presentation modes", __func__);
        return res;
      }
    }

    queues = FindFamilyQueues();
    if (queues.empty())
    {
      Logger::EchoError("No suitable family queues", __func__);
      return res;
    }

    std::map<VkDeviceSize, Queue> min_queues;
    auto adv = min_queues.end();
    for (auto& q : queues)
    {
      if (q.family.has_value())
        adv = min_queues.insert(adv, { q.family.value(), q });
    }

    VkDeviceCreateInfo device_create_info = {};
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    for (auto& family : min_queues)
    {
      VkDeviceQueueCreateInfo queue_create_info = {};
      queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_info.queueFamilyIndex = family.first;
      queue_create_info.queueCount = 1;
      queue_create_info.pQueuePriorities = &family.second.queue_priority;
      queue_create_infos.push_back(queue_create_info);
    }

    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.queueCreateInfoCount = (uint32_t)queue_create_infos.size();
    device_create_info.pEnabledFeatures = &p_device.device_features;
    device_create_info.enabledLayerCount = (uint32_t)Misc::RequiredLayers.size();
    device_create_info.ppEnabledLayerNames = Misc::RequiredLayers.data();

    if (queue_flag_bits == QueueType::DrawingType || queue_flag_bits == QueueType::DrawingAndComputeType)
    {
      device_create_info.enabledExtensionCount = (uint32_t)Misc::RequiredGraphicDeviceExtensions.size();
      device_create_info.ppEnabledExtensionNames = Misc::RequiredGraphicDeviceExtensions.data();
    }

    auto er = vkCreateDevice(p_device.device, &device_create_info, nullptr, &res);

    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Device has not created", __func__);
      Logger::EchoDebug("Device name = " + std::string(p_device.device_properties.deviceName), __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }
    else
    {
      req_p_device_features = features;
      Logger::EchoDebug("Device has created, name = " + std::string(p_device.device_properties.deviceName), __func__);
    }
    
    return res;
  }

  VkQueue Device_impl::GetGraphicQueue() const
  {
    if (queue_flag_bits == QueueType::DrawingType || queue_flag_bits == QueueType::DrawingAndComputeType)
    {
      for (auto &q : queues)
      {
        if ((q.purpose == QueuePurpose::PresentationAndGraphicPurpose || 
            q.purpose == QueuePurpose::GraphicPurpose) && q.family.has_value())
        {
          VkQueue res = VK_NULL_HANDLE;
          vkGetDeviceQueue(device, q.family.value(), 0, &res);
          return res;
        }
      }
    }
    
    return VK_NULL_HANDLE;
  }

  VkQueue Device_impl::GetPresentQueue() const
  {
    if (queue_flag_bits == QueueType::DrawingType || queue_flag_bits == QueueType::DrawingAndComputeType)
    {
      for (auto &q : queues)
      {
        if ((q.purpose == QueuePurpose::PresentationAndGraphicPurpose || 
            q.purpose == QueuePurpose::PresentationPurpose) && q.family.has_value())
        {
          VkQueue res = VK_NULL_HANDLE;
          vkGetDeviceQueue(device, q.family.value(), 0, &res);
          return res;
        }
      }
    }
    
    return VK_NULL_HANDLE;
  }

  VkQueue Device_impl::GetComputeQueue() const
  {
    if (queue_flag_bits == QueueType::ComputeType || queue_flag_bits == QueueType::DrawingAndComputeType)
    {
      for (auto &q : queues)
      {
        if (q.purpose == QueuePurpose::ComputePurpose && q.family.has_value())
        {
          VkQueue res = VK_NULL_HANDLE;
          vkGetDeviceQueue(device, q.family.value(), 0, &res);
          return res;
        }
      }
    }
    
    return VK_NULL_HANDLE;
  }

  std::optional<uint32_t> Device_impl::GetGraphicFamilyQueueIndex() const
  {
    std::optional<uint32_t> ret;
    if (queue_flag_bits == QueueType::DrawingType || queue_flag_bits == QueueType::DrawingAndComputeType)
    {
      for (auto &q : queues)
      {
        if ((q.purpose == QueuePurpose::PresentationAndGraphicPurpose || 
            q.purpose == QueuePurpose::GraphicPurpose) && q.family.has_value())
        {
          ret = q.family.value();
          break;
        }
      }
    }
    
    return ret;
  }

  std::optional<uint32_t> Device_impl::GetPresentFamilyQueueIndex() const
  {
    std::optional<uint32_t> ret;
    if (queue_flag_bits == QueueType::DrawingType || queue_flag_bits == QueueType::DrawingAndComputeType)
    {
      for (auto &q : queues)
      {
        if ((q.purpose == QueuePurpose::PresentationAndGraphicPurpose || 
            q.purpose == QueuePurpose::PresentationPurpose) && q.family.has_value())
        {
          ret = q.family.value();
          break;
        }
      }
    }
    
    return ret;
  }

  std::optional<uint32_t> Device_impl::GetComputeFamilyQueueIndex() const
  {
    std::optional<uint32_t> ret;

    if (queue_flag_bits == QueueType::ComputeType || queue_flag_bits == QueueType::DrawingAndComputeType)
    {
      for (auto &q : queues)
      {
        if (q.purpose == QueuePurpose::ComputePurpose && q.family.has_value())
        {
          ret = q.family.value();
          break;
        }
      }
    }
    
    return ret;
  }

  VkFormatProperties Device_impl::GetFormatProperties(const VkFormat format) const
  {
    VkFormatProperties format_properties;
    vkGetPhysicalDeviceFormatProperties(p_device.device, format, &format_properties);

    return format_properties;
  }

  bool Device_impl::CheckMultisampling(VkSampleCountFlagBits x) const noexcept
  {
    return (p_device.device_properties.limits.framebufferColorSampleCounts & 
            p_device.device_properties.limits.framebufferDepthSampleCounts) & x;
  }

  VkQueue Device_impl::GetQueueFormFamilyIndex(const uint32_t index) const
  {
    VkQueue q;
    vkGetDeviceQueue(device, index, 0, &q);
    return q;
  }

  Device &Device::operator=(const Device &obj)
  {
    if (&obj == this) return *this;
    DeviceConfig conf;
    conf.SetDeviceIndex(obj.impl->p_device.device_index);
    conf.SetDeviceType((PhysicalDeviceType) obj.impl->p_device.device_properties.deviceType);
    conf.SetQueueType(obj.impl->queue_flag_bits);
    conf.SetRequiredDeviceFeatures(obj.impl->req_p_device_features);
    conf.SetSurface(obj.impl->surface);
    impl = std::unique_ptr<Device_impl>(new Device_impl(conf));

    return *this;
  }

  Device::Device(const Device &obj)
  {
    DeviceConfig conf;
    conf.SetDeviceIndex(obj.impl->p_device.device_index);
    conf.SetDeviceType((PhysicalDeviceType) obj.impl->p_device.device_properties.deviceType);
    conf.SetQueueType(obj.impl->queue_flag_bits);
    conf.SetRequiredDeviceFeatures(obj.impl->req_p_device_features);
    conf.SetSurface(obj.impl->surface);
    impl = std::unique_ptr<Device_impl>(new Device_impl(conf));
  }

  Device &Device::operator=(Device &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);
    return *this;
  }

  void Device::swap(Device &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(Device &lhs, Device &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }
}