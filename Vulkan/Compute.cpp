#include "Compute.h"

namespace Vulkan
{
  Compute::Compute(std::shared_ptr<Vulkan::Device> dev, const std::string shader_path, const std::string entry_point)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
      std::runtime_error("Device is nullptr.");
    
    device = dev;
    descriptors = std::make_unique<Descriptors>(device);
    command_pool = std::make_unique<CommandPool>(device, dev->GetComputeFamilyQueueIndex().value());

    compute_shader.shader_filepath = shader_path;
    compute_shader.entry_point = entry_point;
    compute_shader.shader = CreateShader(shader_path);
  }

  Compute::Compute(std::shared_ptr<Vulkan::Device> dev)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
      std::runtime_error("Device is nullptr.");

    device = dev;
    descriptors = std::make_unique<Descriptors>(device);
    command_pool = std::make_unique<CommandPool>(device, device->GetComputeFamilyQueueIndex().value());
  }

  Compute::Compute(const Compute &obj)
  {
    device = obj.device;
    descriptors = std::make_unique<Descriptors>(device);
    command_pool = std::make_unique<CommandPool>(device, device->GetComputeFamilyQueueIndex().value());
    pipeline_options = obj.pipeline_options;
    compute_shader.shader_filepath = obj.compute_shader.shader_filepath;
    compute_shader.entry_point = obj.compute_shader.entry_point;
    if (compute_shader.shader_filepath != "")
      compute_shader.shader = CreateShader(compute_shader.shader_filepath);
  }

  Compute& Compute::operator= (const Compute &obj)
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

      device = obj.device;
      descriptors = std::make_unique<Descriptors>(device);
      command_pool = std::make_unique<CommandPool>(device, device->GetComputeFamilyQueueIndex().value());
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

  Compute& Compute::operator= (const std::vector<std::shared_ptr<IBuffer>> &obj)
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

      descriptors->ClearDescriptorSetLayout(0);
      for (size_t i = 0; i < obj.size(); ++i)
      {
        descriptors->Add(0, i, obj[i], VK_SHADER_STAGE_COMPUTE_BIT);
      }
    }
    else
      std::runtime_error("No Device.");
    return *this;
  } 

  void Compute::Run(std::size_t x, std::size_t y, std::size_t z)
  {
    stop = false;
    std::lock_guard<std::mutex> lock(work_mutex);

    if (device == nullptr || device->GetDevice() == VK_NULL_HANDLE)
      std::runtime_error("No Device.");
    if (compute_shader.shader == VK_NULL_HANDLE)
      std::runtime_error("No Compute Shader.");

    if (pipeline_layout == VK_NULL_HANDLE)
    {
      descriptors->BuildAll();    
      pipeline_layout = Supply::CreatePipelineLayout(device->GetDevice(), descriptors->GetDescriptorSetLayouts());
      pipeline = CreatePipeline(compute_shader.shader, compute_shader.entry_point, pipeline_layout);
    }

    command_pool->BeginCommandBuffer(0, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    command_pool->BindPipeline(0, pipeline, VK_PIPELINE_BIND_POINT_COMPUTE);
    command_pool->BindDescriptorSets(0, pipeline_layout, VK_PIPELINE_BIND_POINT_COMPUTE, descriptors->GetDescriptorSets(), {}, 0);
    command_pool->Dispatch(0, x, y, z);
    command_pool->EndCommandBuffer(0);

    for (size_t i = 0; i < pipeline_options.DispatchTimes; ++i)
    {
      VkSubmitInfo submit_info = {};
      submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
      submit_info.commandBufferCount = 1;
      submit_info.pCommandBuffers = &(*command_pool)[0];

      VkFence fence;
      VkFenceCreateInfo fence_create_info = {};
      fence_create_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
      fence_create_info.flags = 0;

      if (vkCreateFence(device->GetDevice(), &fence_create_info, nullptr, &fence) != VK_SUCCESS)
        throw std::runtime_error("Can't create fence.");
      if (vkQueueSubmit(device->GetComputeQueue(), 1, &submit_info, fence) != VK_SUCCESS)
        throw std::runtime_error("Can't submit queue.");
      if (vkWaitForFences(device->GetDevice(), 1, &fence, VK_TRUE, 100000000000) != VK_SUCCESS)
        throw std::runtime_error("WaitForFences error");
      
      for (auto &opt : pipeline_options.DispatchEndEvents)
      {
        opt.OnDispatchEndEvent(i, 0, opt.index);
      }
      
      vkDestroyFence(device->GetDevice(), fence, nullptr);
      if (stop) break;
    }
    stop = false;
  }

  void Compute::SetPipelineOptions(const ComputePipelineOptions options)
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

  VkShaderModule Compute::CreateShader(const std::string shader_path)
  {
    VkShaderModule result = VK_NULL_HANDLE;
    Supply::LoadPrecompiledShaderFromFile(device->GetDevice(), shader_path, result);
    return result;
  }

  void Compute::SetShader(const std::string shader_path, const std::string entry_point)
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

      compute_shader.shader = CreateShader(shader_path);
    }
  }

  VkPipeline Compute::CreatePipeline(const VkShaderModule shader, const std::string entry_point, const VkPipelineLayout layout)
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

  void Compute::Free()
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
    }
    command_pool.reset();
    descriptors.reset();
    device.reset();
  }
}