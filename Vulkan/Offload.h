#ifndef __CPU_NW_LIBS_VULKAN_OFFLOAD_H
#define __CPU_NW_LIBS_VULKAN_OFFLOAD_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

#include "Instance.h"
#include "Device.h"
#include "Array.h"
#include "UniformBuffer.h"

namespace Vulkan
{
  typedef void (*DispatchEndEvent)(const size_t iteration, const size_t index, Vulkan::IStorage &buff);

  struct UpdateBufferOpt
  {
    size_t index = 0;
    Vulkan::DispatchEndEvent OnDispatchEndEvent = nullptr;
  };

  struct OffloadPipelineOptions
  {
    size_t DispatchTimes = 1;
    std::vector<Vulkan::UpdateBufferOpt> DispatchEndEvents;
  };
  
  template <typename T> class Offload
  {
  private:
    VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkShaderModule compute_shader = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkPhysicalDeviceLimits device_limits = {};
    std::vector<IStorage*> buffers;
    Vulkan::OffloadPipelineOptions pipeline_options = {};
    bool stop = false; 
  public:
    Offload() = default;
    Offload(Device &dev, std::vector<IStorage*> &data, std::string shader_path);
    ~Offload()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
      if (device != VK_NULL_HANDLE)
      {
        stop = true;
        vkDestroyShaderModule(device, compute_shader, nullptr);
        vkDestroyDescriptorPool(device, descriptor_pool, nullptr);
        vkDestroyDescriptorSetLayout(device, descriptor_set_layout, nullptr);
        vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
        vkDestroyPipeline(device, pipeline, nullptr);
        vkDestroyCommandPool(device, command_pool, nullptr);
      }
      device = VK_NULL_HANDLE;
    }
    void Run(size_t x, size_t y, size_t z);
    void SetPipelineOptions(OffloadPipelineOptions options);
  };
}

#endif