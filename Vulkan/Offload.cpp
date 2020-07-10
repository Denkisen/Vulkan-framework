#include "Offload.h"

#include <unistd.h>
#include <thread>
#include <optional>

namespace Vulkan
{
  template class Offload<int>;
  template class Offload<float>;
  template class Offload<double>;
  template class Offload<unsigned>;

  template <typename T>
  Offload<T>::Offload(Device &dev, const std::vector<IStorage*> &data, const std::string shader_path, const std::string entry_point)
  {
    device = dev.GetDevice();
    queue = dev.GetComputeQueue();
    device_limits = dev.GetLimits();
    buffer = data;
    auto index = dev.GetComputeFamilyQueueIndex();
    if (!index.has_value())
      throw std::runtime_error("No Compute family queue");

    family_queue = index.value();

    compute_shader.shader_filepath = shader_path;
    compute_shader.entry_point = entry_point;
    compute_shader.shader = CreateShader(shader_path);
  }

  template <typename T>
  Offload<T>::Offload(Device &dev, const StorageBuffer &data, const std::string shader_path, const std::string entry_point)
  {
    device = dev.GetDevice();
    queue = dev.GetComputeQueue();
    device_limits = dev.GetLimits();
    buffer = data;
    auto index = dev.GetComputeFamilyQueueIndex();
    if (!index.has_value())
      throw std::runtime_error("No Compute family queue");

    family_queue = index.value();

    compute_shader.shader_filepath = shader_path;
    compute_shader.entry_point = entry_point;
    compute_shader.shader = CreateShader(shader_path);
  }

  template <typename T>
  Offload<T>::Offload(Device &dev, const std::string shader_path, const std::string entry_point)
  {
    device = dev.GetDevice();
    queue = dev.GetComputeQueue();
    device_limits = dev.GetLimits();
    auto index = dev.GetComputeFamilyQueueIndex();
    if (!index.has_value())
      throw std::runtime_error("No Compute family queue");

    family_queue = index.value();

    compute_shader.shader_filepath = shader_path;
    compute_shader.entry_point = entry_point;
    compute_shader.shader = CreateShader(shader_path);
  }

  template <typename T>
  Offload<T>::Offload(Device &dev)
  {
    device = dev.GetDevice();
    queue = dev.GetComputeQueue();
    device_limits = dev.GetLimits();
    auto index = dev.GetComputeFamilyQueueIndex();
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
    if (device != VK_NULL_HANDLE)
    {
      if (compute_shader.shader != VK_NULL_HANDLE)
      {
        vkDestroyShaderModule(device, compute_shader.shader, nullptr);
        compute_shader.shader = VK_NULL_HANDLE;
        compute_shader.shader_filepath = "";
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
    if (device != VK_NULL_HANDLE)
    {
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
    if (device != VK_NULL_HANDLE)
    {
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

    if (device == VK_NULL_HANDLE)
      std::runtime_error("No Device.");
    if (compute_shader.shader == VK_NULL_HANDLE)
      std::runtime_error("No Compute Shader.");

    if (pipeline_layout == VK_NULL_HANDLE)
    {
      pipeline_layout = CreatePipelineLayout(buffer.GetDescriptorSetLayout());
      pipeline = CreatePipeline(compute_shader.shader, compute_shader.entry_point, pipeline_layout);
      command_pool = CreateCommandPool(family_queue);  
      command_buffer = CreateCommandBuffer(command_pool);
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

      if (vkCreateFence(device, &fence_create_info, nullptr, &fence) != VK_SUCCESS)
        throw std::runtime_error("Can't create fence.");
      if (vkQueueSubmit(queue, 1, &submit_info, fence) != VK_SUCCESS)
        throw std::runtime_error("Can't submit queue.");
      if (vkWaitForFences(device, 1, &fence, VK_TRUE, 100000000000) != VK_SUCCESS)
        throw std::runtime_error("WaitForFences error");
      
      for (auto &opt : pipeline_options.DispatchEndEvents)
      {
        std::size_t len = 0;
        void *ptr = buffer.Extract(len, opt.index);
        opt.OnDispatchEndEvent(i, opt.index, buffer.GetStorageTypeByIndex(opt.index), ptr, len);
        buffer.UpdateValue(ptr, len, opt.index);
        std::free(ptr);
      }
      
      vkDestroyFence(device, fence, nullptr);
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
    Supply::LoadPrecompiledShaderFromFile(device, shader_path, result);
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
        vkDestroyShaderModule(device, compute_shader.shader, nullptr);
        compute_shader.shader = VK_NULL_HANDLE;
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

      compute_shader.shader = CreateShader(shader_path);
    }
  }

  template <typename T>
  VkPipelineLayout Offload<T>::CreatePipelineLayout(const VkDescriptorSetLayout layout)
  {
    VkPipelineLayout result = VK_NULL_HANDLE;
    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &layout;
    if (vkCreatePipelineLayout(device, &pipeline_layout_create_info, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("Can't create pipeline layout.");

    return result;
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

    if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("Can't create compute pipelines.");

    return result;
  }

  template <typename T>
  VkCommandPool Offload<T>::CreateCommandPool(const uint32_t family_queue)
  {
    VkCommandPool result = VK_NULL_HANDLE;
    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = family_queue;

    if (vkCreateCommandPool(device, &command_pool_create_info, nullptr, &result) != VK_SUCCESS)
      throw std::runtime_error("Can't create command pool."); 

    return result;
  }

  template <typename T>
  VkCommandBuffer Offload<T>::CreateCommandBuffer(const VkCommandPool pool)
  {
    VkCommandBuffer result = VK_NULL_HANDLE;
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = pool; 
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1; 
    if (vkAllocateCommandBuffers(device, &command_buffer_allocate_info, &result) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate command buffers.");
    
    return result;
  }

  template <typename T>
  void Offload<T>::Free()
  {
    stop = true;
    std::lock_guard<std::mutex> lock(work_mutex);
    if (device != VK_NULL_HANDLE)
    {
      if (compute_shader.shader != VK_NULL_HANDLE)
      {
        vkDestroyShaderModule(device, compute_shader.shader, nullptr);
        compute_shader.shader = VK_NULL_HANDLE;
        compute_shader.shader_filepath = "";
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
}
