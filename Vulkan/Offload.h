#ifndef __CPU_NW_LIBS_VULKAN_OFFLOAD_H
#define __CPU_NW_LIBS_VULKAN_OFFLOAD_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <mutex>

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

  struct ShaderStruct
  {
    VkShaderModule shader = VK_NULL_HANDLE;
    std::string shader_filepath = "";
    std::string entry_point = "main";
  };
  
  template <typename T> class Offload
  {
  private:
    std::mutex work_mutex;
    StorageBuffer buffer;
    VkDevice device = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    ShaderStruct compute_shader;
    VkCommandPool command_pool = VK_NULL_HANDLE;
    VkCommandBuffer command_buffer = VK_NULL_HANDLE;
    VkQueue queue = VK_NULL_HANDLE;
    uint32_t family_queue = -1;
    VkPhysicalDeviceLimits device_limits = {};
    Vulkan::OffloadPipelineOptions pipeline_options = {};
    bool stop = false;
    VkShaderModule CreateShader(std::string shader_path); 
    VkPipelineLayout CreatePipelineLayout(VkDescriptorSetLayout layout);
    VkPipeline CreatePipeline(const VkShaderModule shader, const std::string entry_point, const VkPipelineLayout layout);
    VkCommandPool CreateCommandPool(uint32_t family_queue);
    VkCommandBuffer CreateCommandBuffer(VkCommandPool pool);
    void Free();
  public:
    Offload() = delete;
    Offload(Device &dev, std::vector<IStorage*> &data, const std::string shader_path, const std::string entry_point);
    Offload(Device &dev, const std::string shader_path, const std::string entry_point);
    Offload(Device &dev);
    Offload(const Offload<T> &offload);
    Offload<T>& operator= (const Offload<T> &obj);
    Offload<T>& operator= (const std::vector<IStorage*> &obj);   
    void Run(std::size_t x, std::size_t y, std::size_t z);
    void SetPipelineOptions(OffloadPipelineOptions options);
    void SetShader(const std::string shader_path, const std::string entry_point);
    ~Offload()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
      Free();
    }
  };
}

#endif