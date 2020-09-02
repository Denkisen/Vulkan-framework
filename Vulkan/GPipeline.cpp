#include "GPipeline.h"

#include <algorithm>

namespace Vulkan
{
  GPipeline::~GPipeline()
  {
#ifdef DEBUG
    std::cout << __func__ << std::endl;
#endif
    for (size_t i = 0; i < pipelines.size(); ++i)
    {
      DestroyPipeline(i);
    }
  }

  void GPipeline::DestroyPipeline(const size_t index)
  {
    vkDeviceWaitIdle(device->GetDevice());
    if (pipelines[index] != VK_NULL_HANDLE)
      vkDestroyPipeline(device->GetDevice(), pipelines[index], nullptr);
    pipelines[index] = VK_NULL_HANDLE;

    if (pipeline_layouts[index] != VK_NULL_HANDLE)
      vkDestroyPipelineLayout(device->GetDevice(), pipeline_layouts[index], nullptr);
    pipeline_layouts[index] = VK_NULL_HANDLE;

    for (auto &shader : configs[index].shaders)
    if (shader.second != VK_NULL_HANDLE)
      vkDestroyShaderModule(device->GetDevice(), shader.second, nullptr);
    configs[index].shaders.clear();

    pipeline_create_infos[index] = {};
    configs[index] = {};
  }

  GPipeline::GPipeline(std::shared_ptr<Vulkan::Device> dev, std::shared_ptr<Vulkan::SwapChain> swapchain, std::shared_ptr<Vulkan::RenderPass> render_pass)
  {
    if (dev.get() == nullptr || dev->GetDevice() == VK_NULL_HANDLE)
      throw std::runtime_error("Invalid device pointer.");

    if (swapchain.get() == nullptr || swapchain->GetSwapChain() == VK_NULL_HANDLE)
      throw std::runtime_error("Invalid swapchain pointer.");

    if (render_pass.get() == nullptr || render_pass->GetRenderPass() == VK_NULL_HANDLE)
      throw std::runtime_error("Invalid render_pass pointer.");

    device = dev;
    this->swapchain = swapchain;
    this->render_pass = render_pass;
    type = PipelineType::Graphic;
  }

  const VkPipeline GPipeline::GetPipeline(const PipelineLock pipeline_lock) const
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    return pipelines[pipeline_lock.index.value()];
  }

  const VkPipelineLayout GPipeline::GetPipelineLayout(const PipelineLock pipeline_lock) const
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    return pipeline_layouts[pipeline_lock.index.value()];
  }

  void GPipeline::SetupVertexInput(const size_t index)
  {
    configs[index].stage_structs.vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    if (!configs[index].attributes.empty() && !configs[index].bindings.empty())
    {
      configs[index].stage_structs.vertex_input_info.vertexBindingDescriptionCount = (uint32_t) configs[index].bindings.size();
      configs[index].stage_structs.vertex_input_info.pVertexBindingDescriptions = configs[index].bindings.data();
      configs[index].stage_structs.vertex_input_info.vertexAttributeDescriptionCount = (uint32_t) configs[index].attributes.size();
      configs[index].stage_structs.vertex_input_info.pVertexAttributeDescriptions = configs[index].attributes.data(); 
    }
    else
    {
      configs[index].stage_structs.vertex_input_info.vertexBindingDescriptionCount = 0;
      configs[index].stage_structs.vertex_input_info.pVertexBindingDescriptions = nullptr;
      configs[index].stage_structs.vertex_input_info.vertexAttributeDescriptionCount = 0;
      configs[index].stage_structs.vertex_input_info.pVertexAttributeDescriptions = nullptr;  
    }
  }

  void GPipeline::SetupInputAssembly(const size_t index)
  {
    configs[index].stage_structs.input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    configs[index].stage_structs.input_assembly.topology = configs[index].primitive_topology;
    configs[index].stage_structs.input_assembly.primitiveRestartEnable = VK_FALSE;
  }

  void GPipeline::SetupViewports(const size_t index)
  {
    configs[index].stage_structs.viewports.clear();

    VkViewport port = {};
    port.x = 0.0f;
    port.y = 0.0f;
    port.width = (float) this->swapchain->GetExtent().width;
    port.height = (float) this->swapchain->GetExtent().height;
    port.minDepth = 0.0f;
    port.maxDepth = 1.0f;

    configs[index].stage_structs.viewports.push_back(port);
  }

  void GPipeline::SetupScissors(const size_t index)
  {
    configs[index].stage_structs.scissors.clear();

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = this->swapchain->GetExtent();

    configs[index].stage_structs.scissors.push_back(scissor);
  }

  void GPipeline::SetupViewportState(const size_t index)
  {
    SetupViewports(index);
    SetupScissors(index);

    configs[index].stage_structs.viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    configs[index].stage_structs.viewport_state.viewportCount = (uint32_t) configs[index].stage_structs.viewports.size();
    configs[index].stage_structs.viewport_state.pViewports = configs[index].stage_structs.viewports.data();
    configs[index].stage_structs.viewport_state.scissorCount = (uint32_t) configs[index].stage_structs.scissors.size();
    configs[index].stage_structs.viewport_state.pScissors = configs[index].stage_structs.scissors.data();
  }

  void GPipeline::SetupRasterizer(const size_t index)
  {
    configs[index].stage_structs.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    configs[index].stage_structs.rasterizer.depthClampEnable = VK_FALSE;
    configs[index].stage_structs.rasterizer.rasterizerDiscardEnable = VK_FALSE;
    configs[index].stage_structs.rasterizer.polygonMode = configs[index].polygon_mode;
    configs[index].stage_structs.rasterizer.lineWidth = 1.0f;
    configs[index].stage_structs.rasterizer.cullMode = configs[index].cull_mode;
    configs[index].stage_structs.rasterizer.frontFace = configs[index].face;
    configs[index].stage_structs.rasterizer.depthBiasEnable = configs[index].depth_bias_enable;
    configs[index].stage_structs.rasterizer.depthBiasConstantFactor = 0.0f;
    configs[index].stage_structs.rasterizer.depthBiasClamp = 0.0f;
    configs[index].stage_structs.rasterizer.depthBiasSlopeFactor = 0.0f;
  }

  void GPipeline::SetupMultisampling(const size_t index)
  {
    configs[index].stage_structs.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    configs[index].stage_structs.multisampling.sampleShadingEnable = configs[index].supersampling.first;
    configs[index].stage_structs.multisampling.rasterizationSamples = configs[index].multisampling;
    configs[index].stage_structs.multisampling.minSampleShading = configs[index].supersampling.second;
    configs[index].stage_structs.multisampling.pSampleMask = nullptr;
    configs[index].stage_structs.multisampling.alphaToCoverageEnable = VK_FALSE;
    configs[index].stage_structs.multisampling.alphaToOneEnable = VK_FALSE;
  }

  void GPipeline::SetupColorBlending(const size_t index)
  {
    configs[index].stage_structs.color_blend_attachments.clear();

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    configs[index].stage_structs.color_blend_attachments.push_back(color_blend_attachment);

    configs[index].stage_structs.color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    configs[index].stage_structs.color_blending.logicOpEnable = VK_FALSE;
    configs[index].stage_structs.color_blending.logicOp = VK_LOGIC_OP_COPY;
    configs[index].stage_structs.color_blending.attachmentCount = (uint32_t) configs[index].stage_structs.color_blend_attachments.size();
    configs[index].stage_structs.color_blending.pAttachments = configs[index].stage_structs.color_blend_attachments.data();
    configs[index].stage_structs.color_blending.blendConstants[0] = 0.0f;
    configs[index].stage_structs.color_blending.blendConstants[1] = 0.0f;
    configs[index].stage_structs.color_blending.blendConstants[2] = 0.0f;
    configs[index].stage_structs.color_blending.blendConstants[3] = 0.0f;
  }

  void GPipeline::SetupDynamicState(const size_t index)
  {
    configs[index].stage_structs.dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    configs[index].stage_structs.dynamic_state.dynamicStateCount = (uint32_t) configs[index].stage_structs.dynamic_states.size();
    configs[index].stage_structs.dynamic_state.pDynamicStates = configs[index].stage_structs.dynamic_states.data();
  }

  void GPipeline::SetupDepthStencil(const size_t index)
  {
    configs[index].stage_structs.depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    configs[index].stage_structs.depth_stencil.depthTestEnable = configs[index].use_depth_testing;
    configs[index].stage_structs.depth_stencil.depthWriteEnable = configs[index].use_depth_testing;
    configs[index].stage_structs.depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    configs[index].stage_structs.depth_stencil.depthBoundsTestEnable = VK_FALSE;
    configs[index].stage_structs.depth_stencil.minDepthBounds = 0.0f;
    configs[index].stage_structs.depth_stencil.maxDepthBounds = 1.0f;
    configs[index].stage_structs.depth_stencil.stencilTestEnable = VK_FALSE;
    configs[index].stage_structs.depth_stencil.front = {};
    configs[index].stage_structs.depth_stencil.back = {};
  }

  void GPipeline::SetupTessellation(const size_t index)
  {
    configs[index].stage_structs.tessellation.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    configs[index].stage_structs.tessellation.flags = 0;
    configs[index].stage_structs.tessellation.patchControlPoints = 10;
  }

  void GPipeline::SetDynamicState(const size_t index, const VkDynamicState state, const VkBool32 enable)
  {
    auto st = configs[index].stage_structs.dynamic_states.begin();
    auto ed = configs[index].stage_structs.dynamic_states.end();

    if (auto it = std::find(st, ed, state); it != ed)
    {
      if (!enable) configs[index].stage_structs.dynamic_states.erase(it);
    }
    else
      if (enable) configs[index].stage_structs.dynamic_states.push_back(state);
  }

  void GPipeline::SetupPipeline(const size_t index)
  {
    SetupVertexInput(index);
    SetupInputAssembly(index);
    SetupViewportState(index);
    SetupRasterizer(index);
    SetupMultisampling(index);
    SetupColorBlending(index);
    SetupDynamicState(index);
    SetupDepthStencil(index);

    if (pipelines[index] != VK_NULL_HANDLE)
      vkDestroyPipeline(device->GetDevice(), pipelines[index], nullptr);
    pipelines[index] = VK_NULL_HANDLE;

    if (configs[index].build_layout)
    {
      if (pipeline_layouts[index] != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device->GetDevice(), pipeline_layouts[index], nullptr);
      pipeline_layouts[index] = Supply::CreatePipelineLayout(device->GetDevice(), configs[index].descriptor_set_layouts);
      configs[index].build_layout = false;
    }

    if (configs[index].build_shaders)
    {
      for (auto &shader : configs[index].shaders)
        if (shader.second != VK_NULL_HANDLE)
          vkDestroyShaderModule(device->GetDevice(), shader.second, nullptr);
      configs[index].shaders.clear();
      SetupShaderInfos(index);
      configs[index].build_shaders = false;
    }

    pipeline_create_infos[index].sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_infos[index].layout = pipeline_layouts[index];
    pipeline_create_infos[index].renderPass = render_pass->GetRenderPass();
    pipeline_create_infos[index].subpass = 0;
    pipeline_create_infos[index].flags = 0;
    pipeline_create_infos[index].basePipelineHandle = VK_NULL_HANDLE;
    pipeline_create_infos[index].basePipelineIndex = -1;
    pipeline_create_infos[index].stageCount = (uint32_t) configs[index].stage_structs.stage_infos.size();
    pipeline_create_infos[index].pStages = configs[index].stage_structs.stage_infos.data();

    pipeline_create_infos[index].pVertexInputState = &configs[index].stage_structs.vertex_input_info;
    pipeline_create_infos[index].pInputAssemblyState = &configs[index].stage_structs.input_assembly;
    pipeline_create_infos[index].pViewportState = &configs[index].stage_structs.viewport_state;
    pipeline_create_infos[index].pRasterizationState = &configs[index].stage_structs.rasterizer;
    pipeline_create_infos[index].pMultisampleState = &configs[index].stage_structs.multisampling;
    pipeline_create_infos[index].pDepthStencilState = &configs[index].stage_structs.depth_stencil;
    pipeline_create_infos[index].pColorBlendState = &configs[index].stage_structs.color_blending;
    pipeline_create_infos[index].pTessellationState = &configs[index].stage_structs.tessellation;

    if (configs[index].stage_structs.dynamic_states.empty())
      pipeline_create_infos[index].pDynamicState = nullptr;
    else
      pipeline_create_infos[index].pDynamicState = &configs[index].stage_structs.dynamic_state;
  }

  void GPipeline::SetupShaderInfos(const size_t index)
  {
    configs[index].stage_structs.stage_infos.clear();

    for (auto &shader : configs[index].shaders)
      if (shader.second != VK_NULL_HANDLE)
        vkDestroyShaderModule(device->GetDevice(), shader.second, nullptr);
    configs[index].shaders.clear();

    for (size_t i = 0; i < configs[index].shader_infos.size(); ++i)
    {
      if (configs[index].shaders.find(configs[index].shader_infos[i].type) != configs[index].shaders.end())
        continue;
      else
        configs[index].shaders[configs[index].shader_infos[i].type] = VK_NULL_HANDLE;

      configs[index].stage_structs.stage_infos.push_back({});
      size_t ind = configs[index].stage_structs.stage_infos.size() - 1;
      Supply::CreateShaderStageInfo(device->GetDevice(), configs[index].shader_infos[i], 
                                    configs[index].shaders[configs[index].shader_infos[i].type], 
                                    (VkShaderStageFlagBits) configs[index].shader_infos[i].type, 
                                    configs[index].stage_structs.stage_infos[ind]);
    }
  }

  PipelineLock GPipeline::OrderPipelineLock()
  {
    std::lock_guard<std::mutex> lock(block_mutex);
    PipelineLock result;
    result.index = blocks.size();
    blocks[blocks.size()] = std::make_shared<std::mutex>();
    result.lock = blocks[result.index.value()];
    return result;
  }

  void GPipeline::ReleasePipelineLock(PipelineLock &pipeline_lock)
  {
    std::lock_guard<std::mutex> lock(block_mutex);
    if (!pipeline_lock.index.has_value())
      return;
    if (pipeline_lock.lock.get() == nullptr)
      return;

    pipeline_lock.lock->lock();
    pipeline_lock.lock.reset();
    blocks.erase(pipeline_lock.index.value());
    pipeline_lock.index.reset();
  }

  void GPipeline::BeginPipeline(const PipelineLock pipeline_lock)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    if (configs.size() <= pipeline_lock.index.value())
    {
      configs.resize(configs.size() + 1);
      pipeline_create_infos.resize(pipeline_create_infos.size() + 1);
      pipelines.resize(pipelines.size() + 1);
      pipeline_layouts.resize(pipeline_layouts.size() + 1);
    }
    else
    {
      DestroyPipeline(pipeline_lock.index.value());
    }
  }

  void GPipeline::EndPipeline(const size_t index, const Vulkan::PipelineInheritance inheritance, const PipelineLock deriv_lock)
  {
    std::string errors = "";

    if (configs[index].shader_infos.empty())
      errors += "Shaders are not specified.\n";
    if (configs[index].descriptor_set_layouts.empty())
      errors += "Descriptor set layouts are not specified.\n";
    if (!device->CheckMultisampling(configs[index].multisampling))
      errors += "Device is not supporting given samples count.\n";

    if (errors != "")
      throw std::runtime_error(errors);

    SetupPipeline(index);

    configs[index].ready_to_build = true;

    switch (inheritance)
    {
      case PipelineInheritance::Base:
        pipeline_create_infos[index].flags = VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
        if (vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_infos[index], 
                                      nullptr, &pipelines[index]) != VK_SUCCESS) 
          throw std::runtime_error("Failed to create graphics pipeline!");
        break;
      case PipelineInheritance::Derivative:
      {
        if (!deriv_lock.index.has_value() || deriv_lock.lock.get() == nullptr)
          throw std::runtime_error("Buffer lock is empty.");

        if (configs[deriv_lock.index.value()].inheritance != PipelineInheritance::Base)
          throw std::runtime_error("Pipeline is not Base.");

        if (pipelines[deriv_lock.index.value()] == VK_NULL_HANDLE)
          throw std::runtime_error("Base pipeline is VK_NULL_HANDLE.");

        std::lock_guard<std::mutex> lock(*deriv_lock.lock.get());
        
        pipeline_create_infos[index].flags = VK_PIPELINE_CREATE_DERIVATIVE_BIT;
        pipeline_create_infos[index].basePipelineHandle = pipelines[deriv_lock.index.value()];
        if (vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_infos[index], 
                                      nullptr, &pipelines[index]) != VK_SUCCESS) 
          throw std::runtime_error("Failed to create graphics pipeline!");
        configs[deriv_lock.index.value()].index_of_base = deriv_lock;
      } break;
      case PipelineInheritance::None:
        if (vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_infos[index], 
                                      nullptr, &pipelines[index]) != VK_SUCCESS) 
          throw std::runtime_error("Failed to create graphics pipeline!");
        break;
    }

    configs[index].inheritance = inheritance;
  }

  void GPipeline::EndPipeline(const PipelineLock pipeline_lock, const Vulkan::PipelineInheritance inheritance, const PipelineLock deriv_lock)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());
    EndPipeline(pipeline_lock.index.value(), inheritance, deriv_lock);
  }

  void GPipeline::ReBuildPipeline(const PipelineLock pipeline_lock)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    pipeline_create_infos[pipeline_lock.index.value()] = {};
    configs[pipeline_lock.index.value()].ready_to_build = false;

    EndPipeline(pipeline_lock.index.value(), configs[pipeline_lock.index.value()].inheritance, configs[pipeline_lock.index.value()].index_of_base);
  }

  void GPipeline::SetShaders(const PipelineLock pipeline_lock, const std::vector<Vulkan::ShaderInfo> &shader_infos)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    configs[pipeline_lock.index.value()].shader_infos = shader_infos;
    configs[pipeline_lock.index.value()].ready_to_build = false;
    configs[pipeline_lock.index.value()].build_shaders = true;
  }

  void GPipeline::UseDepthTesting(const PipelineLock pipeline_lock, const VkBool32 enable)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    configs[pipeline_lock.index.value()].use_depth_testing = enable;
    configs[pipeline_lock.index.value()].ready_to_build = false;
  }

  void GPipeline::SetVertexInputBindingDescription(const PipelineLock pipeline_lock, const std::vector<VkVertexInputBindingDescription> &binding_description, const std::vector<VkVertexInputAttributeDescription> &attribute_descriptions)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    configs[pipeline_lock.index.value()].attributes = attribute_descriptions;
    configs[pipeline_lock.index.value()].bindings = binding_description;
    configs[pipeline_lock.index.value()].ready_to_build = false;
  }

  void GPipeline::SetDescriptorsSetLayouts(const PipelineLock pipeline_lock, const std::vector<VkDescriptorSetLayout> &layouts)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    configs[pipeline_lock.index.value()].descriptor_set_layouts = layouts;
    configs[pipeline_lock.index.value()].build_layout = true;
    configs[pipeline_lock.index.value()].ready_to_build = false;
  }

  void GPipeline::SetMultisampling(const PipelineLock pipeline_lock, const VkSampleCountFlagBits count)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    configs[pipeline_lock.index.value()].multisampling = count;
    configs[pipeline_lock.index.value()].ready_to_build = false;
  }

  void GPipeline::SetSupersampling(const PipelineLock pipeline_lock, const VkBool32 enable, const float sample_shading)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    configs[pipeline_lock.index.value()].supersampling = {enable, sample_shading};
    configs[pipeline_lock.index.value()].ready_to_build = false;
  }

  void GPipeline::SetPolygonMode(const PipelineLock pipeline_lock, const VkPolygonMode val)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    configs[pipeline_lock.index.value()].polygon_mode = val;
    configs[pipeline_lock.index.value()].ready_to_build = false;
  }

  void GPipeline::SetPrimitiveTopology(const PipelineLock pipeline_lock, const VkPrimitiveTopology val)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    configs[pipeline_lock.index.value()].primitive_topology = val;
    configs[pipeline_lock.index.value()].ready_to_build = false;
  }

  void GPipeline::SetFrontFace(const PipelineLock pipeline_lock, const VkFrontFace val)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    configs[pipeline_lock.index.value()].face = val;
    configs[pipeline_lock.index.value()].ready_to_build = false;
  }

  void GPipeline::SetCullMode(const PipelineLock pipeline_lock, const VkCullModeFlags val)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    configs[pipeline_lock.index.value()].cull_mode = val;
    configs[pipeline_lock.index.value()].ready_to_build = false;
  }

  void GPipeline::EnableDynamicStateDepthBias(const PipelineLock pipeline_lock, const VkBool32 enable)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    configs[pipeline_lock.index.value()].depth_bias_enable = enable;
    SetDynamicState(pipeline_lock.index.value(), VK_DYNAMIC_STATE_DEPTH_BIAS, enable);
    configs[pipeline_lock.index.value()].ready_to_build = false;
  }

  void GPipeline::EnableDynamicStateViewport(const PipelineLock pipeline_lock, const VkBool32 enable)
  {
    if (!pipeline_lock.index.has_value() || pipeline_lock.lock.get() == nullptr)
      throw std::runtime_error("Buffer lock is empty.");

    std::lock_guard<std::mutex> lock(*pipeline_lock.lock.get());

    SetDynamicState(pipeline_lock.index.value(), VK_DYNAMIC_STATE_VIEWPORT, enable);
    configs[pipeline_lock.index.value()].ready_to_build = false;
  }
}