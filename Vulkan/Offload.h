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
  typedef void (*DispatchEndEvent)(const std::size_t iteration, const std::size_t index, const Vulkan::StorageType type, void *buff, const std::size_t length);

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
    VkShaderModule CreateShader(const std::string shader_path); 
    VkPipeline CreatePipeline(const VkShaderModule shader, const std::string entry_point, const VkPipelineLayout layout);
    void Free();
  public:
    Offload() = delete;
    Offload(Device &dev, const std::vector<IStorage*> &data, const std::string shader_path, const std::string entry_point);
    Offload(Device &dev, const StorageBuffer &data, const std::string shader_path, const std::string entry_point);
    Offload(Device &dev, const std::string shader_path, const std::string entry_point);
    Offload(Device &dev);
    Offload(const Offload<T> &offload);
    Offload<T>& operator= (const Offload<T> &obj);
    Offload<T>& operator= (const std::vector<IStorage*> &obj);  
    Offload<T>& operator= (const StorageBuffer &obj);  
    void Run(std::size_t x, std::size_t y, std::size_t z);
    void SetPipelineOptions(const OffloadPipelineOptions options);
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