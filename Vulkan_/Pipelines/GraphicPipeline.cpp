#include "GraphicPipeline.h"
#include "../Misc.h"

namespace Vulkan
{
  GraphicPipeline_impl::~GraphicPipeline_impl() noexcept
  {
    Logger::EchoDebug("", __func__);
    for (auto &obj : shaders)
    { 
      if (obj.shader != VK_NULL_HANDLE) 
        vkDestroyShaderModule(device->GetDevice(), obj.shader, nullptr);
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

  GraphicPipeline_impl::GraphicPipeline_impl(const std::shared_ptr<Device> dev, const std::shared_ptr<SwapChain> swapchain, 
                                             const std::shared_ptr<RenderPass> render_pass, const GraphicPipelineConfig &params)
  {
    if (dev.get() == nullptr || !dev->IsValid())
    {
      Logger::EchoError("Device is empty", __func__);
      return;
    }

    if (swapchain.get() == nullptr || !swapchain->IsValid())
    {
      Logger::EchoError("SwapChain is empty", __func__);
      return;
    }

    if (render_pass.get() == nullptr || !render_pass->IsValid())
    {
      Logger::EchoError("RenderPass is empty", __func__);
      return;
    }

    device = dev;
    this->swapchain = swapchain;
    this->render_pass = render_pass;
    init_config = params;
    if (device->CheckSampleCountSupport(init_config.sample_count) != VK_TRUE)
    {
      Logger::EchoWarning("Given samples count is not supported", __func__);
      init_config.sample_count = VK_SAMPLE_COUNT_1_BIT;
    }
    
    auto er = Create();
    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't create pipeline", __func__);
      return;
    }
  }

  VkResult GraphicPipeline_impl::ReCreate()
  {
    vkDeviceWaitIdle(device->GetDevice());
    return Create();
  }

  VkResult GraphicPipeline_impl::Create()
  {
    SetupVertexInput();
    SetupInputAssembly();
    SetupViewportState();
    SetupRasterizer();
    SetupMultisampling();
    SetupDynamicState();
    SetupColorBlending();
    SetupDepthStencil();
    SetupTessellation();

    BuildLayout();
    BuildShaders();

    VkGraphicsPipelineCreateInfo pipeline_create_info = {};
    pipeline_create_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_create_info.layout = pipeline_layout;
    pipeline_create_info.renderPass = render_pass->GetRenderPass();
    pipeline_create_info.subpass = init_config.subpass;
    pipeline_create_info.basePipelineHandle = init_config.base_pipeline;
    pipeline_create_info.flags = init_config.base_pipeline != VK_NULL_HANDLE ? VK_PIPELINE_CREATE_DERIVATIVE_BIT : VK_PIPELINE_CREATE_ALLOW_DERIVATIVES_BIT;
    pipeline_create_info.basePipelineIndex = -1;
    pipeline_create_info.stageCount = (uint32_t) stages_config.stage_infos.size();
    pipeline_create_info.pStages = stages_config.stage_infos.data();

    pipeline_create_info.pVertexInputState = &stages_config.vertex_input_info;
    pipeline_create_info.pInputAssemblyState = &stages_config.input_assembly;
    pipeline_create_info.pViewportState = &stages_config.viewport_state;
    pipeline_create_info.pRasterizationState = &stages_config.rasterizer;
    pipeline_create_info.pMultisampleState = &stages_config.multisampling;
    pipeline_create_info.pDepthStencilState = &stages_config.depth_stencil;
    pipeline_create_info.pColorBlendState = &stages_config.color_blending;
    pipeline_create_info.pTessellationState = &stages_config.tessellation;

    if (stages_config.dynamic_states.empty())
      pipeline_create_info.pDynamicState = nullptr;
    else
      pipeline_create_info.pDynamicState = &stages_config.dynamic_state;

    if (pipeline != VK_NULL_HANDLE)
    {
      vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
      pipeline = VK_NULL_HANDLE;
    }

    auto er = vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_create_info, nullptr, &pipeline);

    if (er != VK_SUCCESS)
    {
      Logger::EchoError("Can't create pipeline", __func__);
      Logger::EchoDebug("Return code = " + std::to_string(er), __func__);
    }

    return er;
  }

  void GraphicPipeline_impl::BuildShaders()
  {
    if (build_shaders)
    {
      for (auto &obj : shaders)
      { 
        if (obj.shader != VK_NULL_HANDLE) 
          vkDestroyShaderModule(device->GetDevice(), obj.shader, nullptr);
      }
      
      shaders.reserve(init_config.shader_infos.size());
      stages_config.stage_infos.reserve(init_config.shader_infos.size());

      for (auto &obj : init_config.shader_infos)
      {
        shaders.push_back({});
        stages_config.stage_infos.push_back({});
        auto i = shaders.size() - 1;
        shaders[i].entry = obj.second.entry;
        shaders[i].shader = Misc::LoadPrecompiledShaderFromFile(device->GetDevice(), obj.second.file_path.string());
        stages_config.stage_infos[i].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages_config.stage_infos[i].pNext = nullptr;
        stages_config.stage_infos[i].pSpecializationInfo = nullptr;
        stages_config.stage_infos[i].module = shaders[i].shader;
        stages_config.stage_infos[i].stage = (VkShaderStageFlagBits) obj.second.type;
        stages_config.stage_infos[i].pName = shaders[i].entry.c_str();
        stages_config.stage_infos[i].flags = 0;
      }
      
      build_shaders = false;
    }
  }

  void GraphicPipeline_impl::BuildLayout()
  {
    if (pipeline_layout == VK_NULL_HANDLE || build_layout)
    {
      if (pipeline_layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device->GetDevice(), pipeline_layout, nullptr);

      pipeline_layout = Misc::CreatePipelineLayout(device->GetDevice(), init_config.desc_layouts);
      build_layout = false;
    }
  }

  void GraphicPipeline_impl::SetupVertexInput() noexcept
  {
    stages_config.vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    if (!init_config.input_attributes.empty() && !init_config.input_bindings.empty())
    {
      stages_config.vertex_input_info.vertexBindingDescriptionCount = (uint32_t) init_config.input_bindings.size();
      stages_config.vertex_input_info.pVertexBindingDescriptions = init_config.input_bindings.data();
      stages_config.vertex_input_info.vertexAttributeDescriptionCount = (uint32_t) init_config.input_attributes.size();
      stages_config.vertex_input_info.pVertexAttributeDescriptions = init_config.input_attributes.data();
    }
    else
    {
      stages_config.vertex_input_info.vertexBindingDescriptionCount = 0;
      stages_config.vertex_input_info.pVertexBindingDescriptions = nullptr;
      stages_config.vertex_input_info.vertexAttributeDescriptionCount = 0;
      stages_config.vertex_input_info.pVertexAttributeDescriptions = nullptr;  
    }
  }

  void GraphicPipeline_impl::SetupInputAssembly() noexcept
  {
    stages_config.input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    stages_config.input_assembly.topology = init_config.primitive_topology;
    stages_config.input_assembly.primitiveRestartEnable = VK_FALSE;
  }

  void GraphicPipeline_impl::SetupViewports()
  {
    stages_config.viewports.clear();

    VkViewport port = {};
    port.x = 0.0f;
    port.y = 0.0f;
    port.width = (float) swapchain->GetExtent().width;
    port.height = (float) swapchain->GetExtent().height;
    port.minDepth = 0.0f;
    port.maxDepth = 1.0f;

    stages_config.viewports.push_back(port);
  }

  void GraphicPipeline_impl::SetupScissors()
  {
    stages_config.scissors.clear();

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = swapchain->GetExtent();

    stages_config.scissors.push_back(scissor);
  }

  void GraphicPipeline_impl::SetupViewportState()
  {
    SetupViewports();
    SetupScissors();

    stages_config.viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    stages_config.viewport_state.viewportCount = (uint32_t) stages_config.viewports.size();
    stages_config.viewport_state.pViewports = stages_config.viewports.data();
    stages_config.viewport_state.scissorCount = (uint32_t) stages_config.scissors.size();
    stages_config.viewport_state.pScissors = stages_config.scissors.data();
  }

  void GraphicPipeline_impl::SetupRasterizer() noexcept
  {
    stages_config.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    stages_config.rasterizer.depthClampEnable = VK_FALSE;
    stages_config.rasterizer.rasterizerDiscardEnable = VK_FALSE;
    stages_config.rasterizer.polygonMode = init_config.polygon_mode;
    stages_config.rasterizer.lineWidth = 1.0f;
    stages_config.rasterizer.cullMode = init_config.cull_mode;
    stages_config.rasterizer.frontFace = init_config.front_face;
    stages_config.rasterizer.depthBiasEnable = init_config.use_depth_bias;
    stages_config.rasterizer.depthBiasConstantFactor = 0.0f;
    stages_config.rasterizer.depthBiasClamp = 0.0f;
    stages_config.rasterizer.depthBiasSlopeFactor = 0.0f;
  }

  void GraphicPipeline_impl::SetupMultisampling() noexcept
  {
    stages_config.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    stages_config.multisampling.sampleShadingEnable = init_config.use_sample_shading;
    stages_config.multisampling.rasterizationSamples = init_config.sample_count;
    stages_config.multisampling.minSampleShading = init_config.min_sample_shading;
    stages_config.multisampling.pSampleMask = nullptr;
    stages_config.multisampling.alphaToCoverageEnable = VK_FALSE;
    stages_config.multisampling.alphaToOneEnable = VK_FALSE;
  }

  void GraphicPipeline_impl::SetupColorBlending()
  {
    stages_config.color_blend_attachments.clear();

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    stages_config.color_blend_attachments.push_back(color_blend_attachment);

    stages_config.color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    stages_config.color_blending.logicOpEnable = VK_FALSE;
    stages_config.color_blending.logicOp = VK_LOGIC_OP_COPY;
    stages_config.color_blending.attachmentCount = (uint32_t) stages_config.color_blend_attachments.size();
    stages_config.color_blending.pAttachments = stages_config.color_blend_attachments.size() > 0 ? stages_config.color_blend_attachments.data() : nullptr;
    stages_config.color_blending.blendConstants[0] = 0.0f;
    stages_config.color_blending.blendConstants[1] = 0.0f;
    stages_config.color_blending.blendConstants[2] = 0.0f;
    stages_config.color_blending.blendConstants[3] = 0.0f;
  }

  void GraphicPipeline_impl::SetupDynamicState()
  {
    stages_config.dynamic_states.clear();

    stages_config.dynamic_states.reserve(init_config.dynamic_states.size());
    std::copy(init_config.dynamic_states.begin(), init_config.dynamic_states.end(), std::back_inserter(stages_config.dynamic_states));

    stages_config.dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    stages_config.dynamic_state.dynamicStateCount = (uint32_t) stages_config.dynamic_states.size();
    stages_config.dynamic_state.pDynamicStates = stages_config.dynamic_states.size() > 0 ? stages_config.dynamic_states.data() : nullptr;
  }

  void GraphicPipeline_impl::SetupDepthStencil() noexcept
  {
    stages_config.depth_stencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    stages_config.depth_stencil.depthTestEnable = init_config.use_depth_testing;
    stages_config.depth_stencil.depthWriteEnable = init_config.use_depth_testing;
    stages_config.depth_stencil.depthCompareOp = VK_COMPARE_OP_LESS;
    stages_config.depth_stencil.depthBoundsTestEnable = VK_FALSE;
    stages_config.depth_stencil.minDepthBounds = 0.0f;
    stages_config.depth_stencil.maxDepthBounds = 1.0f;
    stages_config.depth_stencil.stencilTestEnable = VK_FALSE;
    stages_config.depth_stencil.front = {};
    stages_config.depth_stencil.back = {};
  }

  void GraphicPipeline_impl::SetupTessellation() noexcept
  {
    stages_config.tessellation.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
    stages_config.tessellation.flags = 0;
    stages_config.tessellation.patchControlPoints = 10;
  }

  VkResult GraphicPipeline_impl::AddDescriptorSetLayout(const VkDescriptorSetLayout layout)
  {
    if (layout != VK_NULL_HANDLE) 
      init_config.desc_layouts.push_back(layout);
  
    build_layout = true;

    return VK_SUCCESS;
  }

  VkResult GraphicPipeline_impl::AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> layouts)
  {
    std::copy_if(layouts.begin(), layouts.end(), std::back_inserter(init_config.desc_layouts), [](const auto &obj) { return obj != VK_NULL_HANDLE; });
    build_layout = true;

    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::ClearDescriptorSetLayouts() noexcept
  {
    init_config.desc_layouts.clear();
    build_layout = true;

    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::SetBasePipeline(const VkPipeline pipeline) noexcept
  {
    init_config.base_pipeline = pipeline;
    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::AddShader(const ShaderType type, const std::filesystem::path file_path, const std::string entry)
  {
    init_config.shader_infos[type] = {entry, file_path, type}; 
    build_shaders = true;

    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::SetPolygonMode(const VkPolygonMode mode) noexcept
  {
    init_config.polygon_mode = mode;
    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::SetPrimitiveTopology(const VkPrimitiveTopology topology) noexcept
  {
    init_config.primitive_topology = topology;
    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::SetFace(const VkFrontFace face) noexcept
  {
    init_config.front_face = face;
    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::SetCullMode(const VkCullModeFlags mode) noexcept
  {
    init_config.cull_mode = mode;
    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::UseDepthTesting(const VkBool32 val) noexcept
  {
    init_config.use_depth_testing = val;
    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::UseDepthBias(const VkBool32 val) noexcept
  {
    init_config.use_depth_bias = val;
    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::SetSamplesCount(const VkSampleCountFlagBits val) noexcept
  {
    if (device->CheckSampleCountSupport(val) == VK_TRUE)
    {
      init_config.sample_count = val;
    }
    else
    {
      Logger::EchoWarning("Given samples count is not supported", __func__);
    }
    
    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::UseSampleShading(const VkBool32 val) noexcept
  {
    init_config.use_sample_shading = val;
    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::SetMinSampleShading(const float val) noexcept
  {
    init_config.min_sample_shading = val;
    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::AddInputBinding(const GraphicPipelineConfig::InputBinding conf)
  {
    init_config.input_bindings.push_back(conf.binding_desc);
    std::copy(conf.attribute_desc.begin(), conf.attribute_desc.end(), std::back_inserter(init_config.input_attributes));

    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::ClearInputBindings() noexcept
  {
    init_config.input_attributes.clear();
    init_config.input_bindings.clear();
    return VK_SUCCESS;
  }
  
  VkResult GraphicPipeline_impl::AddDynamicState(const VkDynamicState state)
  {
    init_config.dynamic_states.insert(state);
    return VK_SUCCESS;
  }

  GraphicPipeline &GraphicPipeline::operator=(GraphicPipeline &&obj) noexcept
  {
    if (&obj == this) return *this;

    impl = std::move(obj.impl);

    return *this;
  }

  void GraphicPipeline::swap(GraphicPipeline &obj) noexcept
  {
    if (&obj == this) return;

    impl.swap(obj.impl);
  }

  void swap(GraphicPipeline &lhs, GraphicPipeline &rhs) noexcept
  {
    if (&lhs == &rhs) return;

    lhs.swap(rhs);
  }
}