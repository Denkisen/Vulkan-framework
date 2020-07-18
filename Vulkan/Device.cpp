#include "Device.h"

#include <map>
#include <cstring>

namespace Vulkan
{
  void Device::Create()
  {
    
    if (queue_flag_bits == Vulkan::QueueType::DrawingType)
    {
      if (!p_device.device_features.geometryShader)
        throw std::runtime_error("Device has no geometry shader.");

      auto ext = Vulkan::Supply::GetPhysicalDeviceExtensions(p_device.device);
      for (auto s : ext)
      {
        bool found = false;
        for (auto &e : Vulkan::Supply::RequiredGraphicDeviceExtensions)
        {
          if (std::string(e) == s)
          {
            found = true;
            break;
          }
        }
        if (!found)
          std::runtime_error("Extension (" + std::string(s) + ") not supported");
      }

      auto swap_chain_details = Vulkan::Supply::GetSwapChainDetails(p_device.device, surface.surface);
      if (swap_chain_details.formats.empty() || swap_chain_details.present_modes.empty())
        throw std::runtime_error("Swap chain does not support any formats or presentation modes.");
    }

    queues = FindFamilyQueues();

    if (queues.empty())
      throw std::runtime_error("No suitable family queues");

    VkDeviceCreateInfo device_create_info = {};
    std::vector<VkDeviceQueueCreateInfo> queue_create_infos;
    for (auto &family : queues)
    {
      VkDeviceQueueCreateInfo queue_create_info = {};  
      queue_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queue_create_info.queueFamilyIndex = family.family;
      queue_create_info.queueCount = 1;
      queue_create_info.pQueuePriorities = &family.queue_priority;
      queue_create_infos.push_back(queue_create_info);
    }

    device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_create_info.pQueueCreateInfos = queue_create_infos.data();
    device_create_info.queueCreateInfoCount = (uint32_t) queue_create_infos.size();
    device_create_info.pEnabledFeatures = &p_device.device_features;
#ifdef DEBUG
    device_create_info.enabledLayerCount = (uint32_t) Vulkan::Supply::ValidationLayers.size();
    device_create_info.ppEnabledLayerNames = Vulkan::Supply::ValidationLayers.data();
#endif
    if (queue_flag_bits == Vulkan::QueueType::DrawingType)
    {
      device_create_info.enabledExtensionCount = (uint32_t) Vulkan::Supply::RequiredGraphicDeviceExtensions.size();
      device_create_info.ppEnabledExtensionNames = Vulkan::Supply::RequiredGraphicDeviceExtensions.data();
    }
    
    if(vkCreateDevice(p_device.device, &device_create_info, nullptr, &device) != VK_SUCCESS)
      throw std::runtime_error("Can't create device.");
#ifdef DEBUG
      std::cout << "Device is ready." << std::endl;
#endif
  }

  std::vector<Queue> Device::FindFamilyQueues()
  {
    std::vector<Queue> ret;
    uint32_t family_queues_count = Vulkan::Supply::GetFamilyQueuesCount(p_device.device);

    if (family_queues_count == 0)
      return ret;

    std::vector<VkQueueFamilyProperties> queue_families(family_queues_count);
    vkGetPhysicalDeviceQueueFamilyProperties(p_device.device, &family_queues_count, queue_families.data());

    switch (queue_flag_bits)
    {
      case QueueType::ComputeType:
      {
        for (uint32_t i = 0; i < family_queues_count; ++i)
        {
          if (queue_families[i].queueFlags & queue_flag_bits)
          {
            Vulkan::Queue q = {};
            q.family = i;
            q.purpose = QueuePurpose::ComputePurpose;
            q.props = queue_families[i];
            q.queue_priority = 1.0f;
            ret.push_back(q);
            break;
          }
        }
      } break;
      case QueueType::DrawingType:
      {
        for (uint32_t i = 0; i < family_queues_count; ++i)
        {
          if (queue_families[i].queueFlags & queue_flag_bits)
          {
            VkBool32 present = false;
            Vulkan::Queue q = {};
            q.family = i;
            q.props = queue_families[i];
            q.queue_priority = 1.0f;
            if (vkGetPhysicalDeviceSurfaceSupportKHR(p_device.device, i, surface.surface, &present) != VK_SUCCESS)
            {
              throw std::runtime_error("Can't check surface support.");
            }

            if (present)
            {
              if (ret.empty())
              {
                q.purpose = QueuePurpose::PresentationAndGraphicPurpose;
                ret.push_back(q);
                break;
              }
              else
              {
                q.purpose = QueuePurpose::PresentationPurpose;
                ret.push_back(q);
              }
            }
            else
            {
              if (ret.empty())
              {
                q.purpose = QueuePurpose::GraphicPurpose;
                ret.push_back(q);
              }
            }
          }
          if (ret.size() == 2)
            break;
        }

        if (ret.size() == 1 && ret[0].purpose != QueuePurpose::PresentationAndGraphicPurpose)
          ret.clear();
      } break;
    }

    return ret;
  }

  void Device::CreateSurface()
  {
    Vulkan::Instance instance;
    if (glfwCreateWindowSurface(instance.GetInstance(), surface.window, nullptr, &surface.surface) != VK_SUCCESS)
      throw std::runtime_error("Can't create surface");
  }

  Device::Device(uint32_t device_index, Vulkan::QueueType queue_flags, GLFWwindow *window)
  {
    queue_flag_bits = queue_flags;
    surface.window = window;
    if (surface.window != nullptr && queue_flag_bits == Vulkan::QueueType::DrawingType)
      CreateSurface();
    
    Instance instance;
    std::vector<VkPhysicalDevice> devices = Vulkan::Supply::GetAllPhysicalDevices(instance.GetInstance());

    if (devices.empty())
      throw std::runtime_error("No Physical Devices.");
    if (device_index >= devices.size())
      throw std::runtime_error("Invalid device index.");

    p_device.device = devices[device_index];
    p_device.device_features = Vulkan::Supply::GetPhysicalDeviceFeatures(p_device.device);
    p_device.device_index = device_index;
    vkGetPhysicalDeviceProperties(p_device.device, &p_device.device_properties);
    Create();
  }

  Device::Device(Vulkan::PhysicalDeviceType type, Vulkan::QueueType queue_flags, GLFWwindow *window)
  {
    queue_flag_bits = queue_flags;
    surface.window = window;
    if (surface.window != nullptr && queue_flag_bits == Vulkan::QueueType::DrawingType)
      CreateSurface();

    Instance instance;
    auto devices = Vulkan::Supply::GetPhysicalDevicesByType(instance.GetInstance(), (VkPhysicalDeviceType) type);

    if (devices.empty())
      throw std::runtime_error("devices.empty() : No Physical Devices.");

    std::multimap<uint32_t, PhysicalDevice> ranking;
    for (size_t i = 0; i < devices.size(); ++i)
    {
      PhysicalDevice p;
      uint32_t rank = 0;      
      p.device = devices[i];
      p.device_index = i;
      vkGetPhysicalDeviceProperties(p.device, &p.device_properties);
      p.device_features = Vulkan::Supply::GetPhysicalDeviceFeatures(p.device);

      switch (queue_flag_bits)
      {
        case Vulkan::QueueType::DrawingType:      
          if (!p.device_features.geometryShader)
          {
            continue;
          }
          rank++;
          rank += p.device_properties.limits.maxImageDimension2D;
          break;
        case Vulkan::QueueType::ComputeType:
          rank++;
          rank += p.device_properties.limits.maxComputeSharedMemorySize;
          break;
      }
      rank += p.device_properties.limits.maxUniformBufferRange;
      rank += p.device_properties.limits.maxStorageBufferRange;
      rank += p.device_properties.limits.maxMemoryAllocationCount;
      rank += p.device_properties.limits.maxBoundDescriptorSets;
      
#ifdef DEBUG
      std::cout << std::string(p.device_properties.deviceName) << " rank :" << rank << std::endl;
#endif
      ranking.insert(std::pair<uint32_t, PhysicalDevice>(rank, p));
    }

    if (ranking.empty())
      throw std::runtime_error("ranking.empty() : No Physical Devices.");

    for (auto &dev : ranking)
    {
      try
      {
        p_device = dev.second;
#ifdef DEBUG
        std::cout << "Try to create " << std::string(p_device.device_properties.deviceName) << " rank :" << dev.first << std::endl;
#endif
        Create();
      }
      catch(const std::exception& e)
      {
        std::cout << e.what() << '\n';
      }
    }
  }

  Device::Device(std::string device_name, Vulkan::QueueType queue_flags, GLFWwindow *window)
  {
    queue_flag_bits = queue_flags;
    surface.window = window;
    if (surface.window != nullptr && queue_flag_bits == Vulkan::QueueType::DrawingType)
      CreateSurface();

    Instance instance;
    std::vector<VkPhysicalDevice> devices = Vulkan::Supply::GetAllPhysicalDevices(instance.GetInstance());

    if (devices.empty())
      throw std::runtime_error("No Physical Devices.");
    
    for (size_t i = 0; i < devices.size(); ++i)
    {
      PhysicalDevice p;
      p.device = devices[i];
      vkGetPhysicalDeviceProperties(p.device, &p.device_properties);
      p.device_features = Vulkan::Supply::GetPhysicalDeviceFeatures(p.device);
      p.device_index = i;
      if (device_name == std::string(p.device_properties.deviceName))
      {
        p_device = p;
        Create();
        return;
      }
    }
    throw std::runtime_error("No device with given name.");
  }

  Device::~Device()
  {
#ifdef DEBUG
    std::cout << __func__ << std::endl;
#endif
    if (device != VK_NULL_HANDLE)
      vkDestroyDevice(device, nullptr);
    device = VK_NULL_HANDLE;

    if (surface.surface != VK_NULL_HANDLE)
    {
      Vulkan::Instance instance;
      vkDestroySurfaceKHR(instance.GetInstance(), surface.surface, nullptr);
      surface.surface = VK_NULL_HANDLE;
    }
  }

  VkQueue Device::GetGraphicQueue()
  {
    if (queue_flag_bits == QueueType::DrawingType)
    {
      for (auto &q : queues)
      {
        if (q.purpose == QueuePurpose::PresentationAndGraphicPurpose || 
            q.purpose == QueuePurpose::GraphicPurpose)
        {
          return Vulkan::Supply::GetQueueFormFamilyIndex(device, q.family);
        }
      }
    }
    
    return VK_NULL_HANDLE;
  }

  VkQueue Device::GetPresentQueue()
  {
    if (queue_flag_bits == QueueType::DrawingType)
    {
      for (auto &q : queues)
      {
        if (q.purpose == QueuePurpose::PresentationAndGraphicPurpose || 
            q.purpose == QueuePurpose::PresentationPurpose)
        {
          return Vulkan::Supply::GetQueueFormFamilyIndex(device, q.family);
        }
      }
    }
    
    return VK_NULL_HANDLE;
  }

  VkQueue Device::GetComputeQueue()
  {
    if (queue_flag_bits == QueueType::ComputeType)
    {
      for (auto &q : queues)
      {
        if (q.purpose == QueuePurpose::ComputePurpose)
        {
          return Vulkan::Supply::GetQueueFormFamilyIndex(device, q.family);
        }
      }
    }
    
    return VK_NULL_HANDLE;
  }

  std::optional<uint32_t> Device::GetGraphicFamilyQueueIndex()
  {
    std::optional<uint32_t> ret;
    if (queue_flag_bits == QueueType::DrawingType)
    {
      for (auto &q : queues)
      {
        if (q.purpose == QueuePurpose::PresentationAndGraphicPurpose || 
            q.purpose == QueuePurpose::GraphicPurpose)
        {
          ret = q.family;
          break;
        }
      }
    }
    
    return ret;
  }

  std::optional<uint32_t> Device::GetPresentFamilyQueueIndex()
  {
    std::optional<uint32_t> ret;
    if (queue_flag_bits == QueueType::DrawingType)
    {
      for (auto &q : queues)
      {
        if (q.purpose == QueuePurpose::PresentationAndGraphicPurpose || 
            q.purpose == QueuePurpose::PresentationPurpose)
        {
          ret = q.family;
          break;
        }
      }
    }
    
    return ret;
  }

  std::optional<uint32_t> Device::GetComputeFamilyQueueIndex()
  {
    std::optional<uint32_t> ret;

    if (queue_flag_bits == QueueType::ComputeType)
    {
      for (auto &q : queues)
      {
        if (q.purpose == QueuePurpose::ComputePurpose)
        {
          ret = q.family;
          break;
        }
      }
    }
    
    return ret;
  }

  std::pair<uint32_t, uint32_t> Device::GetWindowSize()
  {
    std::pair<uint32_t, uint32_t> ret;
    if (surface.window == nullptr)
      return ret;

    int w, h;
    glfwGetFramebufferSize(surface.window, &w, &h);
    ret = std::make_pair<uint32_t, uint32_t> ((uint32_t) w, (uint32_t) h);

    return ret;
  }
}