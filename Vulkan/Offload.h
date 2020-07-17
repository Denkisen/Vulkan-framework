#ifndef __CPU_NW_LIBS_VULKAN_OFFLOAD_H
#define __CPU_NW_LIBS_VULKAN_OFFLOAD_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <mutex>
#include <unistd.h>
#include <thread>
#include <optional>

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
    std::shared_ptr<Vulkan::Device> device;
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
    Offload(std::shared_ptr<Vulkan::Device> dev, const std::vector<IStorage*> &data, const std::string shader_path, const std::string entry_point);
    Offload(std::shared_ptr<Vulkan::Device> dev, const StorageBuffer &data, const std::string shader_path, const std::string entry_point);
    Offload(std::shared_ptr<Vulkan::Device> dev, const std::string shader_path, const std::string entry_point);
    Offload(std::shared_ptr<Vulkan::Device> dev);
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

namespace Vulkan
{
  template <typename T>
  Offload<T>::Offload(std::shared_ptr<Vulkan::Device> dev, const std::vector<IStorage*> &data, const std::string shader_path, const std::string entry_point)
  {
    device = dev;
    queue = dev->GetComputeQueue();
    device_limits = dev->GetLimits();
    buffer = data;
    auto index = dev->GetComputeFamilyQueueIndex();
    if (!index.has_value())
      throw std::runtime_error("No Compute family queue");

    family_queue = index.value();

    compute_shader.shader_filepath = shader_path;
    compute_shader.entry_point = entry_point;
    compute_shader.shader = CreateShader(shader_path);
  }

  template <typename T>
  Offload<T>::Offload(std::shared_ptr<Vulkan::Device> dev, const StorageBuffer &data, const std::string shader_path, const std::string entry_point)
  {
    device = dev;
    queue = dev->GetComputeQueue();
    device_limits = dev->GetLimits();
    buffer = data;
    auto index = dev->GetComputeFamilyQueueIndex();
    if (!index.has_value())
      throw std::runtime_error("No Compute family queue");

    family_queue = index.value();

    compute_shader.shader_filepath = shader_path;
    compute_shader.entry_point = entry_point;
    compute_shader.shader = CreateShader(shader_path);
  }

  template <typename T>
  Offload<T>::Offload(std::shared_ptr<Vulkan::Device> dev, const std::string shader_path, const std::string entry_point)
  {
    device = dev;
    queue = dev->GetComputeQueue();
    device_limits = dev->GetLimits();
    auto index = dev->GetComputeFamilyQueueIndex();
    if (!index.has_value())
      throw std::runtime_error("No Compute family queue");

    family_queue = index.value();

    compute_shader.shader_filepath = shader_path;
    compute_shader.entry_point = entry_point;
    compute_shader.shader = CreateShader(shader_path);
  }

  template <typename T>
  Offload<T>::Offload(std::shared_ptr<Vulkan::Device> dev)
  {
    device = dev;
    queue = dev->GetComputeQueue();
    device_limits = dev->GetLimits();
    auto index = dev->GetComputeFamilyQueueIndex();
    if (!index.has_value())
      throw std::runtime_error("No Compute family queue");

    family_queue = index.value();
  }

  template <typename T>
  Offload<T>::Offload(const Offload<T> &offload)
  {
    device = offload.device;
    family_queue = offload.family_queue;
    device_limits = offload.device_limits;
    queue = offload.queue;
    buffer = offload.buffer;
    pipeline_options = offload.pipeline_options;
    compute_shader.shader_filepath = offload.compute_shader.shader_filepath;
    compute_shader.entry_point = offload.compute_shader.entry_point;
    if (compute_shader.shader_filepath != "")
      compute_shader.shader = CreateShader(compute_shader.shader_filepath);
  }

  template <typename T>
  Offload<T>& Offload<T>::operator= (const Offload<T> &obj)
  {
    stop = true;
    std::lock_guard<std::mutex> lock(work_mutex);
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      if (compute_shader.shader != VK_NULL_HANDLE)
      {
        vkDestroyShaderModule(device->GetDevice(), compute_shader.shader, nullptr);
        compute_shader.shader = VK_NULL_HANDLE;
        compute_shader.shader_filepath = "";
      }
      if (pipeline_layout != VK_NULL_HANDLE)
      {
        vkDestroyPipelineLayout(device->GetDevice(), pipeline_layout, nullptr);
        pipeline_layout = VK_NULL_HANDLE;
      }
      if (pipeline != VK_NULL_HANDLE)
      {
        vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
      }
      if (command_pool != VK_NULL_HANDLE)
      {
        vkDestroyCommandPool(device->GetDevice(), command_pool, nullptr);
        command_pool = VK_NULL_HANDLE;
      }

      device = obj.device;
      family_queue = obj.family_queue;
      device_limits = obj.device_limits;
      queue = obj.queue;
      buffer = obj.buffer;
      pipeline_options = obj.pipeline_options;
      compute_shader.shader_filepath = obj.compute_shader.shader_filepath;
      compute_shader.entry_point = obj.compute_shader.entry_point;
      if (compute_shader.shader_filepath != "")
        compute_shader.shader = CreateShader(compute_shader.shader_filepath);
    }
    else
      std::runtime_error("No Device.");
    return *this;
  }

  template <typename T>
  Offload<T>& Offload<T>::operator= (const std::vector<IStorage*> &obj)
  {
    stop = true;
    std::lock_guard<std::mutex> lock(work_mutex);
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      if (pipeline_layout != VK_NULL_HANDLE)
      {
        vkDestroyPipelineLayout(device->GetDevice(), pipeline_layout, nullptr);
        pipeline_layout = VK_NULL_HANDLE;
      }
      if (pipeline != VK_NULL_HANDLE)
      {
        vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
      }
      if (command_pool != VK_NULL_HANDLE)
      {
        vkDestroyCommandPool(device->GetDevice(), command_pool, nullptr);
        command_pool = VK_NULL_HANDLE;
      }

      buffer = obj;
    }
    else
      std::runtime_error("No Device.");
    return *this;
  } 

  template <typename T>
  Offload<T>& Offload<T>::operator= (const StorageBuffer &obj)
  {
    stop = true;
    std::lock_guard<std::mutex> lock(work_mutex);
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      if (pipeline_layout != VK_NULL_HANDLE)
      {
        vkDestroyPipelineLayout(device->GetDevice(), pipeline_layout, nullptr);
        pipeline_layout = VK_NULL_HANDLE;
      }
      if (pipeline != VK_NULL_HANDLE)
      {
        vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
      }
      if (command_pool != VK_NULL_HANDLE)
      {
        vkDestroyCommandPool(device->GetDevice(), command_pool, nullptr);
        command_pool = VK_NULL_HANDLE;
      }

      buffer = obj;
    }
    else
      std::runtime_error("No Device.");
    return *this;
  }

  template <typename T>
  void Offload<T>::Run(std::size_t x, std::size_t y, std::size_t z)
  {
    stop = false;
    std::lock_guard<std::mutex> lock(work_mutex);

    if (device == nullptr || device->GetDevice() == VK_NULL_HANDLE)
      std::runtime_error("No Device.");
    if (compute_shader.shader == VK_NULL_HANDLE)
      std::runtime_error("No Compute Shader.");

    if (pipeline_layout == VK_NULL_HANDLE)
    {
      pipeline_layout = Supply::CreatePipelineLayout(device->GetDevice(), {buffer.GetDescriptorSetLayout()});
      pipeline = CreatePipeline(compute_shader.shader, compute_shader.entry_point, pipeline_layout);
      command_pool = Supply::CreateCommandPool(device->GetDevice(), family_queue);  
      command_buffer = Supply::CreateCommandBuffer(device->GetDevice(), command_pool);
    }

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
      throw std::runtime_error("Can't begin command buffer.");

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    VkDescriptorSet set = buffer.GetDescriptorSet();
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &set, 0, nullptr);
    vkCmdDispatch(command_buffer, x, y, z);
    
    if (vkEndCommandBuffer(command_buffer) != VK_SUCCESS)
      throw std::runtime_error("Can't end command buffer.");

    for (size_t i = 0; i < pipeline_options.DispatchTimes; ++i)
    {
      VkSubmitInfo submit_info = {};
      submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit_info.commandBufferCount = 1;
      submit_info.pCommandBuffers = &command_buffer;

      VkFence fence;
      VkFenceCreateInfo fence_create_info = {};
      fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fence_create_info.flags = 0;

      if (vkCreateFence(device->GetDevice(), &fence_create_info, nullptr, &fence) != VK_SUCCESS)
        throw std::runtime_error("Can't create fence.");
      if (vkQueueSubmit(queue, 1, &submit_info, fence) != VK_SUCCESS)
        throw std::runtime_error("Can't submit queue.");
      if (vkWaitForFences(device->GetDevice(), 1, &fence, VK_TRUE, 100000000000) != VK_SUCCESS)
        throw std::runtime_error("WaitForFences error");
      
      for (auto &opt : pipeline_options.DispatchEndEvents)
      {
        std::size_t len = 0;
        void *ptr = buffer.Extract(len, opt.index);
        opt.OnDispatchEndEvent(i, opt.index, buffer.GetStorageTypeByIndex(opt.index), ptr, len);
        buffer.UpdateValue(ptr, len, opt.index);
        std::free(ptr);
      }
      
      vkDestroyFence(device->GetDevice(), fence, nullptr);
      if (stop) break;
    }
    stop = false;
  }

  template <typename T>
  void Offload<T>::SetPipelineOptions(const OffloadPipelineOptions options)
  {
    std::lock_guard<std::mutex> lock(work_mutex);
    pipeline_options.DispatchTimes = options.DispatchTimes > 0 ? options.DispatchTimes : 1;
    for (Vulkan::UpdateBufferOpt opt : options.DispatchEndEvents)
    {
      if (opt.OnDispatchEndEvent != nullptr)
      {
        pipeline_options.DispatchEndEvents.push_back(opt);
      }
    }
  }

  template <typename T>
  VkShaderModule Offload<T>::CreateShader(const std::string shader_path)
  {
    VkShaderModule result = VK_NULL_HANDLE;
    Supply::LoadPrecompiledShaderFromFile(device->GetDevice(), shader_path, result);
    return result;
  }

  template <typename T>
  void Offload<T>::SetShader(const std::string shader_path, const std::string entry_point)
  {
    compute_shader.shader_filepath = shader_path;
    compute_shader.entry_point = entry_point;
    if (compute_shader.shader == VK_NULL_HANDLE)
      compute_shader.shader = CreateShader(shader_path);
    else
    {
      std::lock_guard<std::mutex> lock(work_mutex);
      if (compute_shader.shader != VK_NULL_HANDLE)
      {
        vkDestroyShaderModule(device->GetDevice(), compute_shader.shader, nullptr);
        compute_shader.shader = VK_NULL_HANDLE;
      }
      if (pipeline_layout != VK_NULL_HANDLE)
      {
        vkDestroyPipelineLayout(device->GetDevice(), pipeline_layout, nullptr);
        pipeline_layout = VK_NULL_HANDLE;
      }
      if (pipeline != VK_NULL_HANDLE)
      {
        vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
      }
      if (command_pool != VK_NULL_HANDLE)
      {
        vkDestroyCommandPool(device->GetDevice(), command_pool, nullptr);
        command_pool = VK_NULL_HANDLE;
      }

      compute_shader.shader = CreateShader(shader_path);
    }
  }

  template <typename T>
  VkPipeline Offload<T>::CreatePipeline(const VkShaderModule shader, const std::string entry_point, const VkPipelineLayout layout)
  {
    VkPipeline result = VK_NULL_HANDLE;
    VkPipelineShaderStageCreateInfo shader_stage_create_info = {};
    shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shader_stage_create_info.module = shader;
    shader_stage_create_info.pName = entry_point.c_str();

    VkComputePipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_create_info.stage = shader_stage_create_info;
    pipeline_create_info.layout = layout;

    if (vkCreateComputePipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("Can't create compute pipelines.");

    return result;
  }

  template <typename T>
  void Offload<T>::Free()
  {
    stop = true;
    std::lock_guard<std::mutex> lock(work_mutex);
    if (device != nullptr && device->GetDevice() != VK_NULL_HANDLE)
    {
      if (compute_shader.shader != VK_NULL_HANDLE)
      {
        vkDestroyShaderModule(device->GetDevice(), compute_shader.shader, nullptr);
        compute_shader.shader = VK_NULL_HANDLE;
        compute_shader.shader_filepath = "";
      }
      if (pipeline_layout != VK_NULL_HANDLE)
      {
        vkDestroyPipelineLayout(device->GetDevice(), pipeline_layout, nullptr);
        pipeline_layout = VK_NULL_HANDLE;
      }
      if (pipeline != VK_NULL_HANDLE)
      {
        vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
        pipeline = VK_NULL_HANDLE;
      }
      if (command_pool != VK_NULL_HANDLE)
      {
        vkDestroyCommandPool(device->GetDevice(), command_pool, nullptr);
        command_pool = VK_NULL_HANDLE;
      }
    }
    device.reset();
  }
}

#endif