#ifndef __CPU_NW_VULKAN_GPIPELINE_H
#define __CPU_NW_VULKAN_GPIPELINE_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <memory>
#include <mutex>
#include <optional>
#include <map>

#include "Pipeline.h"

namespace Vulkan
{
  struct GraphicPipelineStageStructs_t
  {
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    VkPipelineViewportStateCreateInfo viewport_state = {};
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    VkPipelineColorBlendStateCreateInfo color_blending = {};    
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
    VkPipelineDepthStencilStateCreateInfo depth_stencil = {};
    VkPipelineTessellationStateCreateInfo tessellation = {};
    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;
    std::vector<VkViewport> viewports;
    std::vector<VkRect2D> scissors;    
    std::vector<VkDynamicState> dynamic_states;
    std::vector<VkPipelineShaderStageCreateInfo> stage_infos;
  };

  struct PipelineLock
  {
  private:
    std::optional<uint32_t> index;
    std::shared_ptr<std::mutex> lock;
    friend class GPipeline;
  };

  enum class PipelineInheritance
  {
    None,
    Base,
    Derivative
  };

  struct PipelineConfig
  {
    GraphicPipelineStageStructs_t stage_structs = {};
    std::vector<VkVertexInputBindingDescription> bindings;
    std::vector<VkVertexInputAttributeDescription> attributes;
    std::vector<VkDescriptorSetLayout> descriptor_set_layouts;
    std::vector<Vulkan::ShaderInfo> shader_infos;
    std::map<Vulkan::ShaderType, VkShaderModule> shaders;
    PipelineInheritance inheritance = PipelineInheritance::None;
    PipelineLock index_of_base;
    bool ready_to_build = false;
    bool build_shaders = true;
    bool build_layout = true;

    VkBool32 use_depth_testing = VK_FALSE;

    VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
    VkPrimitiveTopology primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkFrontFace face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
    VkBool32 depth_bias_enable = VK_FALSE;

    VkSampleCountFlagBits multisampling = VK_SAMPLE_COUNT_1_BIT;
    std::pair<VkBool32, float> supersampling = {VK_FALSE, 0.25f};
  };

  class GPipeline : public IPipeline
  {
  private:
    std::mutex block_mutex;
    std::map<uint32_t, std::shared_ptr<std::mutex>> blocks;

    std::vector<PipelineConfig> configs;
    std::vector<VkGraphicsPipelineCreateInfo> pipeline_create_infos;

    void SetupPipeline(const size_t index);
    void DestroyPipeline(const size_t index);

    void SetupVertexInput(const size_t index);
    void SetupInputAssembly(const size_t index);
    void SetupViewports(const size_t index);
    void SetupScissors(const size_t index);
    void SetupViewportState(const size_t index);
    void SetupRasterizer(const size_t index);
    void SetupMultisampling(const size_t index);
    void SetupColorBlending(const size_t index);
    void SetupDynamicState(const size_t index);
    void SetupDepthStencil(const size_t index);
    void SetupTessellation(const size_t index);

    void SetDynamicState(const size_t index, const VkDynamicState state, const VkBool32 enable);

    void SetupShaderInfos(const size_t index);
    void EndPipeline(const size_t index, const Vulkan::PipelineInheritance inheritance, const PipelineLock deriv_lock);
  public:
    GPipeline() = delete;
    GPipeline(const GPipeline &obj) = delete;
    GPipeline& operator= (const GPipeline &obj) = delete;
    GPipeline(std::shared_ptr<Vulkan::Device> dev, std::shared_ptr<Vulkan::SwapChain> swapchain, std::shared_ptr<Vulkan::RenderPass> render_pass);

    const VkPipeline GetPipeline(const PipelineLock pipeline_lock) const;
    const VkPipelineLayout GetPipelineLayout(const PipelineLock pipeline_lock) const;

    PipelineLock OrderPipelineLock();
    void ReleasePipelineLock(PipelineLock &pipeline_lock);

    void BeginPipeline(const PipelineLock pipeline_lock);
    void EndPipeline(const PipelineLock pipeline_lock, const Vulkan::PipelineInheritance inheritance, const PipelineLock deriv_lock);
    void ReBuildPipeline(const PipelineLock pipeline_lock);

    void SetShaders(const PipelineLock pipeline_lock, const std::vector<Vulkan::ShaderInfo> &shader_infos);
    void SetVertexInputBindingDescription(const PipelineLock pipeline_lock, const std::vector<VkVertexInputBindingDescription> &binding_description, const std::vector<VkVertexInputAttributeDescription> &attribute_descriptions);
    void SetDescriptorsSetLayouts(const PipelineLock pipeline_lock, const std::vector<VkDescriptorSetLayout> &layouts);

    void UseDepthTesting(const PipelineLock pipeline_lock, const VkBool32 enable);
    void SetMultisampling(const PipelineLock pipeline_lock, const VkSampleCountFlagBits count);
    void SetPolygonMode(const PipelineLock pipeline_lock, const VkPolygonMode val);
    void SetPrimitiveTopology(const PipelineLock pipeline_lock, const VkPrimitiveTopology val);
    void SetFrontFace(const PipelineLock pipeline_lock, const VkFrontFace val);
    void SetCullMode(const PipelineLock pipeline_lock, const VkCullModeFlags val);
    void SetSupersampling(const PipelineLock pipeline_lock, const VkBool32 enable, const float sample_shading);

    void EnableDynamicStateDepthBias(const PipelineLock pipeline_lock, const VkBool32 enable);
    void EnableDynamicStateViewport(const PipelineLock pipeline_lock, const VkBool32 enable);
    ~GPipeline();
  };
}

#endif