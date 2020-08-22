#ifndef __CPU_NW_VULKAN_COMPUTE_H
#define __CPU_NW_VULKAN_COMPUTE_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <unistd.h>
#include <optional>
#include <memory>
#include <functional>

#include "Instance.h"
#include "Device.h"
#include "Descriptors.h"
#include "CommandPool.h"

namespace Vulkan
{
  typedef std::function<void(std::size_t iteration, std::size_t index, std::size_t element)> DispatchEndEvent;

  struct UpdateBufferOpt
  {
    std::size_t index = 0;
    Vulkan::DispatchEndEvent OnDispatchEndEvent = nullptr;
  };

  struct ComputePipelineOptions
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

  class Compute
  {
  private:
    std::mutex work_mutex;
    std::shared_ptr<Vulkan::Device> device;
    std::unique_ptr<Vulkan::Descriptors> descriptors;
    std::unique_ptr<Vulkan::CommandPool> command_pool;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    ShaderStruct compute_shader;
    Vulkan::ComputePipelineOptions pipeline_options = {};
    Vulkan::BufferLock buffer_lock;
    bool stop = false;
    VkShaderModule CreateShader(const std::string shader_path); 
    VkPipeline CreatePipeline(const VkShaderModule shader, const std::string entry_point, const VkPipelineLayout layout);
    void Free();
  public:
    Compute() = delete;
    Compute(std::shared_ptr<Vulkan::Device> dev, const std::string shader_path, const std::string entry_point);
    Compute(std::shared_ptr<Vulkan::Device> dev);
    Compute(const Compute &obj);
    Compute& operator= (const Compute &obj);
    Compute& operator= (const std::vector<std::shared_ptr<IBuffer>> &obj);
    void Run(std::size_t x, std::size_t y, std::size_t z);
    void SetPipelineOptions(const ComputePipelineOptions options);
    void SetShader(const std::string shader_path, const std::string entry_point);
    VkCommandPool GetCommandPool() { return command_pool->GetCommandPool(); }
    ~Compute()
    {
#ifdef DEBUG
      std::cout << __func__ << std::endl;
#endif
      Free();
    }
  };
}

#endif