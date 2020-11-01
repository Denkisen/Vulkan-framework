#ifndef __VULKAN_GRAPHIC_PIPELINE_H
#define __VULKAN_GRAPHIC_PIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <map>
#include <set>
#include <algorithm>

#include "../Device.h"
#include "../Logger.h"
#include "../SwapChain.h"
#include "Types.h"

namespace Vulkan
{
  struct GraphicPipelineStageStructs
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

  class GraphicPipelineConfig
  {
  public:
    struct InputBinding
    {
      VkVertexInputBindingDescription binding_desc;
      std::vector<VkVertexInputAttributeDescription> attribute_desc;
    };
  private:
    friend class Pipelines;
    friend class GraphicPipeline_impl;
    std::vector<VkDescriptorSetLayout> desc_layouts;
    std::vector<VkVertexInputBindingDescription> input_bindings;
    std::vector<VkVertexInputAttributeDescription> input_attributes;
    std::set<VkDynamicState> dynamic_states;
    VkPipeline base_pipeline = VK_NULL_HANDLE;
    std::map<ShaderType, ShaderInfo> shader_infos;
    VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
    VkPrimitiveTopology primitive_topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    VkFrontFace front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
    VkBool32 use_depth_testing = VK_FALSE;
    VkBool32 use_depth_bias = VK_FALSE;
    VkSampleCountFlagBits sample_count = VK_SAMPLE_COUNT_1_BIT;
    VkBool32 use_sample_shading = VK_FALSE;
    float min_sample_shading = 0.25f;
    
  public:
    GraphicPipelineConfig() = default;
    ~GraphicPipelineConfig() = default;
    auto &AddDescriptorSetLayout(const VkDescriptorSetLayout layout) { if (layout != VK_NULL_HANDLE) desc_layouts.push_back(layout); return *this; }
    auto &AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> layouts) 
    { 
      std::copy_if(layouts.begin(), layouts.end(), std::back_inserter(desc_layouts), [] (const auto &obj) { return obj != VK_NULL_HANDLE; });
      return *this;
    }
    auto &SetBasePipeline(const VkPipeline pipeline) { base_pipeline = pipeline; return *this; }
    auto &AddShader(const ShaderType type, const std::filesystem::path file_path, const std::string entry = "main") 
    {
      if (std::filesystem::exists(file_path)) shader_infos[type] = {entry, file_path, type}; 
      return *this;
    }
    auto &SetPolygonMode(const VkPolygonMode mode) { polygon_mode = mode; return *this; }
    auto &SetPrimitiveTopology(const VkPrimitiveTopology topology) { primitive_topology = topology; return *this; }
    auto &SetFace(const VkFrontFace face) { front_face = face; return *this; }
    auto &SetCullMode(const VkCullModeFlags mode) { cull_mode = mode; return *this; }
    auto &UseDepthTesting(const VkBool32 val) { use_depth_testing = val; return *this; }
    auto &UseDepthBias(const VkBool32 val) { use_depth_bias = val; return *this; }
    auto &SetSamplesCount(const VkSampleCountFlagBits val) { sample_count = val; return *this; }
    auto &UseSampleShading(const VkBool32 val) { use_sample_shading = val; return *this; }
    auto &SetMinSampleShading(const float val) { min_sample_shading = val; return *this; }
    auto &AddInputBinding(const InputBinding conf) 
    { 
      input_bindings.push_back(conf.binding_desc); 
      std::copy(conf.attribute_desc.begin(), conf.attribute_desc.end(), std::back_inserter(input_attributes));
      return *this; 
    }
    auto &AddDynamicState(const VkDynamicState state) { dynamic_states.insert(state); return *this; }
  };

  class GraphicPipeline_impl
  {
  public:
    GraphicPipeline_impl() = delete;
    GraphicPipeline_impl(const GraphicPipeline_impl &obj) = delete;
    GraphicPipeline_impl(GraphicPipeline_impl &&obj) = delete;
    GraphicPipeline_impl &operator=(const GraphicPipeline_impl &obj) = delete;
    GraphicPipeline_impl &operator=(GraphicPipeline_impl &&obj) = delete;
    ~GraphicPipeline_impl();
  private:
    friend class GraphicPipeline;
    std::shared_ptr<Device> device;
    std::shared_ptr<SwapChain> swapchain;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    std::vector<Shader> shaders;
    GraphicPipelineStageStructs stages_config = {};
    std::mutex config_mutex;
    GraphicPipelineConfig init_config;
    bool build_shaders = true;
    bool build_layout = true;

    void SetupVertexInput();
    void SetupInputAssembly();
    void SetupViewports();
    void SetupScissors();
    void SetupViewportState();
    void SetupRasterizer();
    void SetupMultisampling();
    void SetupColorBlending();
    void SetupDynamicState();
    void SetupDepthStencil();
    void SetupTessellation();
    void BuildLayout();
    void BuildShaders();
    VkResult Create();


    GraphicPipeline_impl(const std::shared_ptr<Device> dev, const std::shared_ptr<SwapChain> swapchain, const GraphicPipelineConfig &params);
    VkResult ReCreate();
    VkPipeline GetPipeline() { return pipeline; }
    VkPipelineLayout GetLayout() { return pipeline_layout; }
    VkResult AddDescriptorSetLayout(const VkDescriptorSetLayout layout);
    VkResult AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> layouts);
    VkResult ClearDescriptorSetLayouts();
    VkResult SetBasePipeline(const VkPipeline pipeline);
    VkResult AddShader(const ShaderType type, const std::filesystem::path file_path, const std::string entry);
    VkResult SetPolygonMode(const VkPolygonMode mode);
    VkResult SetPrimitiveTopology(const VkPrimitiveTopology topology);
    VkResult SetFace(const VkFrontFace face);
    VkResult SetCullMode(const VkCullModeFlags mode);
    VkResult UseDepthTesting(const VkBool32 val);
    VkResult UseDepthBias(const VkBool32 val);
    VkResult SetSamplesCount(const VkSampleCountFlagBits val);
    VkResult UseSampleShading(const VkBool32 val);
    VkResult SetMinSampleShading(const float val);
    VkResult AddInputBinding(const GraphicPipelineConfig::InputBinding conf);
    VkResult ClearInputBindings();
    VkResult AddDynamicState(const VkDynamicState state);
  };

  class GraphicPipeline
  {
  private:
    std::unique_ptr<GraphicPipeline_impl> impl;
  public:
    GraphicPipeline() = delete;
    GraphicPipeline(const GraphicPipeline &obj) = delete;
    GraphicPipeline(GraphicPipeline &&obj) noexcept : impl(std::move(obj.impl)) {};
    GraphicPipeline(const std::shared_ptr<Device> dev, const std::shared_ptr<SwapChain> swapchain, const GraphicPipelineConfig &params) :
      impl(std::unique_ptr<GraphicPipeline_impl>(new GraphicPipeline_impl(dev, swapchain, params))) {};
    GraphicPipeline &operator=(const GraphicPipeline &obj) = delete;
    GraphicPipeline &operator=(GraphicPipeline &&obj) noexcept;
    void swap(GraphicPipeline &obj) noexcept;

    VkResult ReCreate() { return impl->ReCreate(); }
    VkPipeline GetPipeline() { return impl->GetPipeline(); }
    VkPipelineLayout GetLayout() { return impl->GetLayout(); }
    bool IsValid() { return impl != nullptr; }
    VkResult AddDescriptorSetLayout(const VkDescriptorSetLayout layout) { return impl->AddDescriptorSetLayout(layout); }
    VkResult AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> layouts) { return impl->AddDescriptorSetLayouts(layouts);}
    VkResult ClearDescriptorSetLayouts() { return impl->ClearDescriptorSetLayouts(); }
    VkResult SetBasePipeline(const VkPipeline pipeline) { return impl->SetBasePipeline(pipeline); }
    VkResult AddShader(const ShaderType type, const std::filesystem::path file_path, const std::string entry = "main") { return impl->AddShader(type, file_path, entry); }
    VkResult SetPolygonMode(const VkPolygonMode mode) { return impl->SetPolygonMode(mode); }
    VkResult SetPrimitiveTopology(const VkPrimitiveTopology topology) { return impl->SetPrimitiveTopology(topology); }
    VkResult SetFace(const VkFrontFace face) { return impl->SetFace(face); }
    VkResult SetCullMode(const VkCullModeFlags mode) { return impl->SetCullMode(mode); }
    VkResult UseDepthTesting(const VkBool32 val) { return impl->UseDepthTesting(val); }
    VkResult UseDepthBias(const VkBool32 val) { return impl->UseDepthBias(val); }
    VkResult SetSamplesCount(const VkSampleCountFlagBits val) { return impl->SetSamplesCount(val); }
    VkResult UseSampleShading(const VkBool32 val) { return impl->UseSampleShading(val); }
    VkResult SetMinSampleShading(const float val) { return impl->SetMinSampleShading(val); }
    VkResult AddInputBinding(const GraphicPipelineConfig::InputBinding conf) { return impl->AddInputBinding(conf); }
    VkResult ClearInputBindings() { return impl->ClearInputBindings(); }
    VkResult AddDynamicState(const VkDynamicState state) { return impl->AddDynamicState(state); }
    ~GraphicPipeline() = default;
  };

  void swap(GraphicPipeline &lhs, GraphicPipeline &rhs) noexcept;
}

#endif