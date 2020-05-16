#ifndef __CPU_NW_LIBS_VULKAN_OFFLOAD_H
#define __CPU_NW_LIBS_VULKAN_OFFLOAD_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>

#include "Instance.h"
#include "Device.h"
#include "StorageBuffer.h"

namespace Vulkan
{
  typedef void (*DispatchEndEvent)(const std::size_t iteration, const std::size_t index, void *buff, const std::size_t length);

  struct UpdateBufferOpt
  {
    std::size_t index = 0;
    Vulkan::DispatchEndEvent OnDispatchEndEvent = nullptr;
  };

  struct OffloadPipelineOptions
  {
    std::size_t DispatchTimes = 1;
    std::vector<Vulkan::UpdateBufferOpt> DispatchEndEvents;
  };
  
  template <typename T> class Offload
  {
  private:
    StorageBuffer buffer;
    VkDevice device = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    VkShaderModule compute_shader = VK_NULL_HANDLE;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    VkPhysicalDeviceLimits device_limits = {};
    Vulkan::OffloadPipelineOptions pipeline_options = {};
    bool stop = false;
    VkShaderModule CreateShader(std::string shader_path); 
    VkPipelineLayout CreatePipelineLayout(VkDescriptorSetLayout layout);
    VkPipeline CreatePipeline(const VkShaderModule shader, const std::string entry_point, const VkPipelineLayout layout);
    VkCommandPool CreateCommandPool(uint32_t family_queue);
    VkCommandBuffer CreateCommandBuffer(VkCommandPool pool);
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
        if (compute_shader != VK_NULL_HANDLE)
        {
          vkDestroyShaderModule(device, compute_shader, nullptr);
          compute_shader = VK_NULL_HANDLE;
        }
        if (pipeline_layout != VK_NULL_HANDLE)
        {
          vkDestroyPipelineLayout(device, pipeline_layout, nullptr);
          pipeline_layout = VK_NULL_HANDLE;
        }
        if (pipeline != VK_NULL_HANDLE)
        {
          vkDestroyPipeline(device, pipeline, nullptr);
          pipeline = VK_NULL_HANDLE;
        }
        if (command_pool != VK_NULL_HANDLE)
        {
          vkDestroyCommandPool(device, command_pool, nullptr);
          command_pool = VK_NULL_HANDLE;
        }
      }
      device = VK_NULL_HANDLE;
    }
    void Run(std::size_t x, std::size_t y, std::size_t z);
    void SetPipelineOptions(OffloadPipelineOptions options);
  };
}

#endif