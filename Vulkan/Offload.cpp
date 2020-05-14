#include "Offload.h"

#include <unistd.h>
#include <thread>

namespace Vulkan
{
  template class Offload<int>;
  template class Offload<float>;
  template class Offload<double>;
  template class Offload<unsigned>;

  template <typename T>
  Offload<T>::Offload(Device &dev, std::vector<IStorage*> &data, std::string shader_path)
  {
    buffers = data;
    device = dev.device;
    queue = dev.queue;
    device_limits = dev.device_limits;
    size_t uniform_buffers = 0;
    size_t storage_buffers = 0;
    std::vector<VkDescriptorSetLayoutBinding> descriptor_set_layout_bindings(buffers.size());
    for (size_t i = 0; i < buffers.size(); ++i)
    {
      VkDescriptorSetLayoutBinding descriptor_set_layout_binding = {};
      descriptor_set_layout_binding.binding = i;
      switch ((*buffers[i]).type)
      {
      case StorageType::Default :
        descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        storage_buffers++;
        break;
      case StorageType::Uniform :
        descriptor_set_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniform_buffers++;
        break;
      }
      descriptor_set_layout_binding.descriptorCount = 1;
      descriptor_set_layout_binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
      descriptor_set_layout_bindings[i] = descriptor_set_layout_binding;
    }

    VkDescriptorSetLayoutCreateInfo descriptor_set_layout_create_info = {};
    descriptor_set_layout_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptor_set_layout_create_info.bindingCount = (uint32_t) descriptor_set_layout_bindings.size(); // only a single binding in this descriptor set layout. 
    descriptor_set_layout_create_info.pBindings = descriptor_set_layout_bindings.data(); 

    if (vkCreateDescriptorSetLayout(dev.device, &descriptor_set_layout_create_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
      throw std::runtime_error("Can't create DescriptorSetLayout.");

    std::vector<VkDescriptorPoolSize> pool_sizes(2);
    pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount = (uint32_t) uniform_buffers;

    pool_sizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    pool_sizes[1].descriptorCount = (uint32_t) storage_buffers;

    VkDescriptorPoolCreateInfo descriptor_pool_create_info = {};
    descriptor_pool_create_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    descriptor_pool_create_info.maxSets = 1;
    descriptor_pool_create_info.poolSizeCount = pool_sizes.size();
    descriptor_pool_create_info.pPoolSizes = pool_sizes.data();

    if (vkCreateDescriptorPool(dev.device, &descriptor_pool_create_info, nullptr, &descriptor_pool) != VK_SUCCESS)
      throw std::runtime_error("Can't creat DescriptorPool.");
    
    VkDescriptorSetAllocateInfo descriptor_set_allocate_info = {};
    descriptor_set_allocate_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO; 
    descriptor_set_allocate_info.descriptorPool = descriptor_pool; // pool to allocate from.
    descriptor_set_allocate_info.descriptorSetCount = 1; // allocate a single descriptor set.
    descriptor_set_allocate_info.pSetLayouts = &descriptor_set_layout;

    if (vkAllocateDescriptorSets(dev.device, &descriptor_set_allocate_info, &descriptor_set) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate DescriptorSets.");

    std::vector<VkDescriptorBufferInfo> descriptor_buffer_infos(buffers.size());
    std::vector<VkWriteDescriptorSet> write_descriptor_set(buffers.size());

    for (size_t i = 0; i < buffers.size(); ++i)
    {
      VkDescriptorBufferInfo descriptor_buffer_info = {};
      descriptor_buffer_info.buffer = (*buffers[i]).buffer;
      descriptor_buffer_info.offset = 0;
      descriptor_buffer_info.range = VK_WHOLE_SIZE;
      descriptor_buffer_infos[i] = descriptor_buffer_info;

      VkWriteDescriptorSet write_descriptor = {};
      write_descriptor.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
      write_descriptor.dstSet = descriptor_set;
      write_descriptor.dstBinding = i;
      write_descriptor.descriptorCount = 1;
      
      switch ((*buffers[i]).type)
      {
      case StorageType::Default :
        write_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        break;
      case StorageType::Uniform :
        write_descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        break;
      }

      write_descriptor.pBufferInfo = &descriptor_buffer_infos[i];
      write_descriptor_set[i] = write_descriptor;
    }

    vkUpdateDescriptorSets(dev.device, (uint32_t) write_descriptor_set.size(), write_descriptor_set.data(), 0, nullptr);

    if (Supply::LoadPrecompiledShaderFromFile(dev.device, shader_path, compute_shader) != VK_SUCCESS)
      throw std::runtime_error("Can't Shader from file.");

    VkPipelineShaderStageCreateInfo shader_stage_create_info = {};
    shader_stage_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shader_stage_create_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shader_stage_create_info.module = compute_shader;
    shader_stage_create_info.pName = "main";

    VkPipelineLayoutCreateInfo pipeline_layout_create_info = {};
    pipeline_layout_create_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipeline_layout_create_info.setLayoutCount = 1;
    pipeline_layout_create_info.pSetLayouts = &descriptor_set_layout; 
    if (vkCreatePipelineLayout(dev.device, &pipeline_layout_create_info, nullptr, &pipeline_layout) != VK_SUCCESS)
      throw std::runtime_error("Can't create pipeline layout.");

    VkComputePipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipeline_create_info.stage = shader_stage_create_info;
    pipeline_create_info.layout = pipeline_layout;

    if (vkCreateComputePipelines(dev.device, VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline) != VK_SUCCESS)
      throw std::runtime_error("Can't create compute pipelines.");

    VkCommandPoolCreateInfo command_pool_create_info = {};
    command_pool_create_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    command_pool_create_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    command_pool_create_info.queueFamilyIndex = dev.family_queue;

    if (vkCreateCommandPool(dev.device, &command_pool_create_info, nullptr, &command_pool) != VK_SUCCESS)
      throw std::runtime_error("Can't create command pool.");    
      
    VkCommandBufferAllocateInfo command_buffer_allocate_info = {};
    command_buffer_allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    command_buffer_allocate_info.commandPool = command_pool; 
    command_buffer_allocate_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    command_buffer_allocate_info.commandBufferCount = 1; 
    if (vkAllocateCommandBuffers(dev.device, &command_buffer_allocate_info, &command_buffer) != VK_SUCCESS)
      throw std::runtime_error("Can't allocate command buffers.");
  }

  template <typename T>
  void Offload<T>::Run(size_t x, size_t y, size_t z)
  {
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;

    if (vkBeginCommandBuffer(command_buffer, &begin_info) != VK_SUCCESS)
      throw std::runtime_error("Can't begin command buffer.");

    vkCmdBindPipeline(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline);
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline_layout, 0, 1, &descriptor_set, 0, nullptr);
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
        opt.OnDispatchEndEvent(i, opt.index, *buffers[opt.index]);
      }
      
      vkDestroyFence(device, fence, nullptr);
    }
  }

  template <typename T>
  void Offload<T>::SetPipelineOptions(OffloadPipelineOptions options)
  {
    pipeline_options.DispatchTimes = options.DispatchTimes > 0 ? options.DispatchTimes : 1;
    for (Vulkan::UpdateBufferOpt opt : options.DispatchEndEvents)
    {
      if (opt.OnDispatchEndEvent != nullptr)
      {
        if (opt.index < buffers.size())
        {
          pipeline_options.DispatchEndEvents.push_back(opt);
        }
      }
    }
  }

}
