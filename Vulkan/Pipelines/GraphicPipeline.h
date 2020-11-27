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
#include "../RenderPass.h"
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
    uint32_t subpass = 0;
    
  public:
    GraphicPipelineConfig() = default;
    ~GraphicPipelineConfig() noexcept = default;
    auto &AddDescriptorSetLayout(const VkDescriptorSetLayout layout) { if (layout != VK_NULL_HANDLE) desc_layouts.push_back(layout); return *this; }
    auto &AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> layouts)
    { 
      std::copy_if(layouts.begin(), layouts.end(), std::back_inserter(desc_layouts), [] (const auto &obj) { return obj != VK_NULL_HANDLE; });
      return *this;
    }
    auto &SetBasePipeline(const VkPipeline pipeline) noexcept { base_pipeline = pipeline; return *this; }
    auto &AddShader(const ShaderType type, const std::filesystem::path file_path, const std::string entry = "main")
    {
      if (std::filesystem::exists(file_path)) shader_infos[type] = {entry, file_path, type};
      else Logger::EchoWarning("Shader path is not exists", __func__); 
      return *this;
    }
    auto &SetPolygonMode(const VkPolygonMode mode) noexcept { polygon_mode = mode; return *this; }
    auto &SetPrimitiveTopology(const VkPrimitiveTopology topology) noexcept { primitive_topology = topology; return *this; }
    auto &SetFace(const VkFrontFace face) noexcept { front_face = face; return *this; }
    auto &SetCullMode(const VkCullModeFlags mode) noexcept { cull_mode = mode; return *this; }
    auto &UseDepthTesting(const VkBool32 val) noexcept { use_depth_testing = val; return *this; }
    auto &UseDepthBias(const VkBool32 val) noexcept { use_depth_bias = val; return *this; }
    auto &SetSamplesCount(const VkSampleCountFlagBits val) noexcept { sample_count = val; return *this; }
    auto &UseSampleShading(const VkBool32 val) noexcept { use_sample_shading = val; return *this; }
    auto &SetMinSampleShading(const float val) noexcept { min_sample_shading = val; return *this; }
    auto &AddInputBinding(const InputBinding conf)
    { 
      input_bindings.push_back(conf.binding_desc);
      std::copy(conf.attribute_desc.begin(), conf.attribute_desc.end(), std::back_inserter(input_attributes));

      return *this; 
    }
    auto &AddDynamicState(const VkDynamicState state) { dynamic_states.insert(state); return *this; }
    auto &SetSubpass(const uint32_t index) noexcept { subpass = index; return *this; }
  };

  class GraphicPipeline_impl
  {
  public:
    GraphicPipeline_impl() = delete;
    GraphicPipeline_impl(const GraphicPipeline_impl &obj) = delete;
    GraphicPipeline_impl(GraphicPipeline_impl &&obj) = delete;
    GraphicPipeline_impl &operator=(const GraphicPipeline_impl &obj) = delete;
    GraphicPipeline_impl &operator=(GraphicPipeline_impl &&obj) = delete;
    ~GraphicPipeline_impl() noexcept;
  private:
    friend class GraphicPipeline;
    std::shared_ptr<Device> device;
    std::shared_ptr<SwapChain> swapchain;
    std::shared_ptr<RenderPass> render_pass;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    std::vector<Shader> shaders;
    GraphicPipelineStageStructs stages_config = {};
    GraphicPipelineConfig init_config;
    bool build_shaders = true;
    bool build_layout = true;

    void SetupVertexInput() noexcept;
    void SetupInputAssembly() noexcept;
    void SetupViewports();
    void SetupScissors();
    void SetupViewportState();
    void SetupRasterizer() noexcept;
    void SetupMultisampling() noexcept;
    void SetupColorBlending();
    void SetupDynamicState();
    void SetupDepthStencil() noexcept;
    void SetupTessellation() noexcept;
    void BuildLayout();
    void BuildShaders();
    VkResult Create();


    GraphicPipeline_impl(const std::shared_ptr<Device> dev, const std::shared_ptr<SwapChain> swapchain, 
                         const std::shared_ptr<RenderPass> render_pass, const GraphicPipelineConfig &params);
    VkResult ReCreate();
    VkPipeline GetPipeline() const noexcept { return pipeline; }
    VkPipelineLayout GetLayout() const noexcept { return pipeline_layout; }
    VkResult AddDescriptorSetLayout(const VkDescriptorSetLayout layout);
    VkResult AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> layouts);
    VkResult ClearDescriptorSetLayouts() noexcept;
    VkResult SetBasePipeline(const VkPipeline pipeline) noexcept;
    VkResult AddShader(const ShaderType type, const std::filesystem::path file_path, const std::string entry);
    VkResult SetPolygonMode(const VkPolygonMode mode) noexcept;
    VkResult SetPrimitiveTopology(const VkPrimitiveTopology topology) noexcept;
    VkResult SetFace(const VkFrontFace face) noexcept;
    VkResult SetCullMode(const VkCullModeFlags mode) noexcept;
    VkResult UseDepthTesting(const VkBool32 val) noexcept;
    VkResult UseDepthBias(const VkBool32 val) noexcept;
    VkResult SetSamplesCount(const VkSampleCountFlagBits val) noexcept;
    VkResult UseSampleShading(const VkBool32 val) noexcept;
    VkResult SetMinSampleShading(const float val) noexcept;
    VkResult AddInputBinding(const GraphicPipelineConfig::InputBinding conf);
    VkResult ClearInputBindings() noexcept;
    VkResult AddDynamicState(const VkDynamicState state);
    std::shared_ptr<Device> GetDevice() const noexcept { return device; }
  };

  class GraphicPipeline
  {
  private:
    std::unique_ptr<GraphicPipeline_impl> impl;
  public:
    GraphicPipeline() = delete;
    GraphicPipeline(const GraphicPipeline &obj) = delete;
    GraphicPipeline(GraphicPipeline &&obj) noexcept : impl(std::move(obj.impl)) {};
    GraphicPipeline(const std::shared_ptr<Device> dev, const std::shared_ptr<SwapChain> swapchain, 
                    const std::shared_ptr<RenderPass> render_pass, const GraphicPipelineConfig &params) :
      impl(std::unique_ptr<GraphicPipeline_impl>(new GraphicPipeline_impl(dev, swapchain, render_pass, params))) {};
    GraphicPipeline &operator=(const GraphicPipeline &obj) = delete;
    GraphicPipeline &operator=(GraphicPipeline &&obj) noexcept;
    void swap(GraphicPipeline &obj) noexcept;
    bool IsValid() const noexcept { return impl.get() && impl->pipeline != VK_NULL_HANDLE; }

    VkResult ReCreate() { if (impl.get()) return impl->ReCreate(); return VK_ERROR_UNKNOWN; }
    VkPipeline GetPipeline() const noexcept { if (impl.get()) return impl->GetPipeline(); return VK_NULL_HANDLE; }
    VkPipelineLayout GetLayout() const noexcept { if (impl.get()) return impl->GetLayout(); return VK_NULL_HANDLE; }
    VkResult AddDescriptorSetLayout(const VkDescriptorSetLayout layout) { if (impl.get()) return impl->AddDescriptorSetLayout(layout); return VK_ERROR_UNKNOWN; }
    VkResult AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> layouts) { if (impl.get()) return impl->AddDescriptorSetLayouts(layouts); return VK_ERROR_UNKNOWN; }
    VkResult ClearDescriptorSetLayouts() noexcept { if (impl.get()) return impl->ClearDescriptorSetLayouts(); return VK_ERROR_UNKNOWN; }
    VkResult SetBasePipeline(const VkPipeline pipeline) noexcept { if (impl.get()) return impl->SetBasePipeline(pipeline); return VK_ERROR_UNKNOWN; }
    VkResult AddShader(const ShaderType type, const std::filesystem::path file_path, const std::string entry = "main") { if (impl.get()) return impl->AddShader(type, file_path, entry); return VK_ERROR_UNKNOWN; }
    VkResult SetPolygonMode(const VkPolygonMode mode) noexcept { if (impl.get()) return impl->SetPolygonMode(mode); return VK_ERROR_UNKNOWN; }
    VkResult SetPrimitiveTopology(const VkPrimitiveTopology topology) noexcept { if (impl.get()) return impl->SetPrimitiveTopology(topology); return VK_ERROR_UNKNOWN; }
    VkResult SetFace(const VkFrontFace face) noexcept { if (impl.get()) return impl->SetFace(face); return VK_ERROR_UNKNOWN; }
    VkResult SetCullMode(const VkCullModeFlags mode) noexcept { if (impl.get()) return impl->SetCullMode(mode); return VK_ERROR_UNKNOWN; }
    VkResult UseDepthTesting(const VkBool32 val) noexcept { if (impl.get()) return impl->UseDepthTesting(val); return VK_ERROR_UNKNOWN; }
    VkResult UseDepthBias(const VkBool32 val) noexcept { if (impl.get()) return impl->UseDepthBias(val); return VK_ERROR_UNKNOWN; }
    VkResult SetSamplesCount(const VkSampleCountFlagBits val) noexcept { if (impl.get()) return impl->SetSamplesCount(val); return VK_ERROR_UNKNOWN; }
    VkResult UseSampleShading(const VkBool32 val) noexcept { if (impl.get()) return impl->UseSampleShading(val); return VK_ERROR_UNKNOWN; }
    VkResult SetMinSampleShading(const float val) noexcept { if (impl.get()) return impl->SetMinSampleShading(val); return VK_ERROR_UNKNOWN; }
    VkResult AddInputBinding(const GraphicPipelineConfig::InputBinding conf) { if (impl.get()) return impl->AddInputBinding(conf); return VK_ERROR_UNKNOWN; }
    VkResult ClearInputBindings() noexcept { if (impl.get()) return impl->ClearInputBindings(); return VK_ERROR_UNKNOWN; }
    VkResult AddDynamicState(const VkDynamicState state) { if (impl.get()) return impl->AddDynamicState(state); return VK_ERROR_UNKNOWN; }
    std::shared_ptr<Device> GetDevice() const noexcept { if (impl.get()) return impl->GetDevice(); return nullptr; }
    ~GraphicPipeline() noexcept = default;
  };

  void swap(GraphicPipeline &lhs, GraphicPipeline &rhs) noexcept;
}

#endif