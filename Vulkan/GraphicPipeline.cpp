#include "GraphicPipeline.h"

namespace Vulkan
{
  void GraphicPipeline::Destroy()
  {
    if (pipeline != VK_NULL_HANDLE)
      vkDestroyPipeline(device->GetDevice(), pipeline, nullptr);
    pipeline = VK_NULL_HANDLE;

    if (pipeline_layout != VK_NULL_HANDLE)
      vkDestroyPipelineLayout(device->GetDevice(), pipeline_layout, nullptr);
    pipeline_layout = VK_NULL_HANDLE;

    if (vertex_shader != VK_NULL_HANDLE)
      vkDestroyShaderModule(device->GetDevice(), vertex_shader, nullptr);
    vertex_shader = VK_NULL_HANDLE;

    if (fragment_shader != VK_NULL_HANDLE)
      vkDestroyShaderModule(device->GetDevice(), fragment_shader, nullptr);
    fragment_shader = VK_NULL_HANDLE;

    if (compute_shader != VK_NULL_HANDLE)
      vkDestroyShaderModule(device->GetDevice(), compute_shader, nullptr);
    compute_shader = VK_NULL_HANDLE;
  }

  GraphicPipeline::~GraphicPipeline()
  {
#ifdef DEBUG
    std::cout << __func__ << std::endl;
#endif
    Destroy();
  }

  GraphicPipeline::GraphicPipeline(std::shared_ptr<Vulkan::Device> dev, std::shared_ptr<Vulkan::SwapChain> swapchain, std::shared_ptr<Vulkan::RenderPass> render_pass)
  {
    if (dev == nullptr && dev->GetDevice() != VK_NULL_HANDLE)
      throw std::runtime_error("Device pointer is not valid.");

    this->swapchain = swapchain;
    if (this->swapchain == nullptr && swapchain->GetSwapChain() != VK_NULL_HANDLE)
      throw std::runtime_error("Swapchain pointer is not valid.");

    this->render_pass = render_pass;
    if (this->render_pass == nullptr && render_pass->GetRenderPass() != VK_NULL_HANDLE)
      throw std::runtime_error("RenderPass pointer is not valid.");

    device = dev;
  }

  void GraphicPipeline::Create()
  {
    CreateShaderStageInfos();
    bool is_graphic = false;
    if (vertex_shader != VK_NULL_HANDLE && fragment_shader != VK_NULL_HANDLE)
      is_graphic = true;
    else if (compute_shader == VK_NULL_HANDLE)
      throw std::runtime_error("Can't recognise pipeline type");

    pipeline_layout = Supply::CreatePipelineLayout(device->GetDevice(), std::vector<VkDescriptorSetLayout>());

    if (is_graphic)
    {
      BuildGraphicPipeline();
    }
    else
      throw std::runtime_error("Compute pipeline is not implemented.");
  }

  void GraphicPipeline::CreateShaderStageInfos()
  {
    stage_infos.resize(0);
    for (auto &s : shader_infos)
    {
      switch (s.type)
      {
        case ShaderType::Vertex:
          if (vertex_shader == VK_NULL_HANDLE)
          {
            stage_infos.push_back({});
            Supply::CreateShaderStageInfo(device->GetDevice(), s, vertex_shader, VK_SHADER_STAGE_VERTEX_BIT, stage_infos[stage_infos.size() - 1]);
          }
          break;
        case ShaderType::Fragment:
          if (fragment_shader == VK_NULL_HANDLE)
          {
            stage_infos.push_back({});
            Supply::CreateShaderStageInfo(device->GetDevice(), s, fragment_shader, VK_SHADER_STAGE_FRAGMENT_BIT, stage_infos[stage_infos.size() - 1]);
          }
          break;
        case ShaderType::Compute:
          if (compute_shader == VK_NULL_HANDLE)
          {
            stage_infos.push_back({});
            Supply::CreateShaderStageInfo(device->GetDevice(), s, compute_shader, VK_SHADER_STAGE_COMPUTE_BIT, stage_infos[stage_infos.size() - 1]);
          }
          break;
      }
    }
  }

  void GraphicPipeline::BuildGraphicPipeline()
  {
    GraphicPipelineStageStructs pipeline_stage_struct = {};
    FillGraphicPipelineStageStructs(pipeline_stage_struct);

    VkGraphicsPipelineCreateInfo pipeline_info = {};
    pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipeline_info.stageCount = (uint32_t) stage_infos.size();
    pipeline_info.pStages = stage_infos.data();

    pipeline_info.pVertexInputState = &pipeline_stage_struct.vertex_input_info;
    pipeline_info.pInputAssemblyState = &pipeline_stage_struct.input_assembly;
    pipeline_info.pViewportState = &pipeline_stage_struct.viewport_state;
    pipeline_info.pRasterizationState = &pipeline_stage_struct.rasterizer;
    pipeline_info.pMultisampleState = &pipeline_stage_struct.multisampling;
    pipeline_info.pDepthStencilState = nullptr;
    pipeline_info.pColorBlendState = &pipeline_stage_struct.color_blending;

    if (pipeline_stage_struct.dynamic_states.empty())
      pipeline_info.pDynamicState = nullptr;
    else
      pipeline_info.pDynamicState = &pipeline_stage_struct.dynamic_state;

    pipeline_info.layout = pipeline_layout;
    pipeline_info.renderPass = render_pass->GetRenderPass();
    pipeline_info.subpass = 0;
    pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
    pipeline_info.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device->GetDevice(), VK_NULL_HANDLE, 1, &pipeline_info, nullptr, &pipeline) != VK_SUCCESS) 
    {
      throw std::runtime_error("Failed to create graphics pipeline!");
    }
  }

  void GraphicPipeline::FillGraphicPipelineStageStructs(GraphicPipelineStageStructs &pipeline_stage_struct)
  {
    pipeline_stage_struct = {};

    pipeline_stage_struct.vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    if (!attribute_descriptions.empty() && !binding_description.empty())
    {
      pipeline_stage_struct.vertex_input_info.vertexBindingDescriptionCount = (uint32_t) binding_description.size();
      pipeline_stage_struct.vertex_input_info.pVertexBindingDescriptions = binding_description.data();
      pipeline_stage_struct.vertex_input_info.vertexAttributeDescriptionCount = (uint32_t) attribute_descriptions.size();
      pipeline_stage_struct.vertex_input_info.pVertexAttributeDescriptions = attribute_descriptions.data(); 
    }
    else
    {
      pipeline_stage_struct.vertex_input_info.vertexBindingDescriptionCount = 0;
      pipeline_stage_struct.vertex_input_info.pVertexBindingDescriptions = nullptr;
      pipeline_stage_struct.vertex_input_info.vertexAttributeDescriptionCount = 0;
      pipeline_stage_struct.vertex_input_info.pVertexAttributeDescriptions = nullptr;  
    }

    pipeline_stage_struct.input_assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    pipeline_stage_struct.input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    pipeline_stage_struct.input_assembly.primitiveRestartEnable = VK_FALSE;

    pipeline_stage_struct.viewport.x = 0.0f;
    pipeline_stage_struct.viewport.y = 0.0f;
    pipeline_stage_struct.viewport.width = (float) this->swapchain->GetExtent().width;
    pipeline_stage_struct.viewport.height = (float) this->swapchain->GetExtent().height;
    pipeline_stage_struct.viewport.minDepth = 0.0f;
    pipeline_stage_struct.viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = this->swapchain->GetExtent();

    pipeline_stage_struct.scissors.push_back(scissor);

    pipeline_stage_struct.viewport_state.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    pipeline_stage_struct.viewport_state.viewportCount = 1;
    pipeline_stage_struct.viewport_state.pViewports = &pipeline_stage_struct.viewport;
    pipeline_stage_struct.viewport_state.scissorCount = (uint32_t) pipeline_stage_struct.scissors.size();
    pipeline_stage_struct.viewport_state.pScissors = pipeline_stage_struct.scissors.data();

    pipeline_stage_struct.rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    pipeline_stage_struct.rasterizer.depthClampEnable = VK_FALSE;
    pipeline_stage_struct.rasterizer.rasterizerDiscardEnable = VK_FALSE;
    pipeline_stage_struct.rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    pipeline_stage_struct.rasterizer.lineWidth = 1.0f;
    pipeline_stage_struct.rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    pipeline_stage_struct.rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    pipeline_stage_struct.rasterizer.depthBiasEnable = VK_FALSE;
    pipeline_stage_struct.rasterizer.depthBiasConstantFactor = 0.0f;
    pipeline_stage_struct.rasterizer.depthBiasClamp = 0.0f;
    pipeline_stage_struct.rasterizer.depthBiasSlopeFactor = 0.0f;

    pipeline_stage_struct.multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    pipeline_stage_struct.multisampling.sampleShadingEnable = VK_FALSE;
    pipeline_stage_struct.multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    pipeline_stage_struct.multisampling.minSampleShading = 1.0f;
    pipeline_stage_struct.multisampling.pSampleMask = nullptr;
    pipeline_stage_struct.multisampling.alphaToCoverageEnable = VK_FALSE;
    pipeline_stage_struct.multisampling.alphaToOneEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    color_blend_attachment.blendEnable = VK_FALSE;
    color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
    color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

    pipeline_stage_struct.color_blend_attachments.push_back(color_blend_attachment);

    pipeline_stage_struct.color_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    pipeline_stage_struct.color_blending.logicOpEnable = VK_FALSE;
    pipeline_stage_struct.color_blending.logicOp = VK_LOGIC_OP_COPY;
    pipeline_stage_struct.color_blending.attachmentCount = (uint32_t) pipeline_stage_struct.color_blend_attachments.size();
    pipeline_stage_struct.color_blending.pAttachments = pipeline_stage_struct.color_blend_attachments.data();
    pipeline_stage_struct.color_blending.blendConstants[0] = 0.0f;
    pipeline_stage_struct.color_blending.blendConstants[1] = 0.0f;
    pipeline_stage_struct.color_blending.blendConstants[2] = 0.0f;
    pipeline_stage_struct.color_blending.blendConstants[3] = 0.0f;

    pipeline_stage_struct.dynamic_states = 
    {
      //VK_DYNAMIC_STATE_VIEWPORT,
      //VK_DYNAMIC_STATE_LINE_WIDTH
    };

    pipeline_stage_struct.dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    pipeline_stage_struct.dynamic_state.dynamicStateCount = (uint32_t) pipeline_stage_struct.dynamic_states.size();
    pipeline_stage_struct.dynamic_state.pDynamicStates = pipeline_stage_struct.dynamic_states.data();
  }

  void GraphicPipeline::ReBuildPipeline()
  {
    if (!shader_infos.empty())
    {
      vkDeviceWaitIdle(device->GetDevice());
      Destroy();
      Create();
    }
    else
      std::cout << "There are no shaders to create." << std::endl;
  }

  VkPipeline GraphicPipeline::GetPipeline()
  {
    if (pipeline == VK_NULL_HANDLE)
    {
      Create();
    }

    return pipeline;
  }

  void GraphicPipeline::SetShaderInfos(std::vector<Vulkan::ShaderInfo> shader_infos)
  {
    this->shader_infos = shader_infos;
    if (pipeline != VK_NULL_HANDLE)
      ReBuildPipeline();
  }

  void GraphicPipeline::SetVertexInputBindingDescription(std::vector<VkVertexInputBindingDescription> binding_description, std::vector<VkVertexInputAttributeDescription> attribute_descriptions)
  {
    this->binding_description = binding_description;
    this->attribute_descriptions = attribute_descriptions;
    if (pipeline != VK_NULL_HANDLE)
      ReBuildPipeline();
  }
}