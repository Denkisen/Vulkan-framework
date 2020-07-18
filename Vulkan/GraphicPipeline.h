#ifndef __CPU_NW_LIBS_VULKAN_GRAPHICPIPELINE_H
#define __CPU_NW_LIBS_VULKAN_GRAPHICPIPELINE_H

#include <vulkan/vulkan.h>
#include <iostream>
#include <vector>
#include <memory>

#include "Supply.h"
#include "Device.h"
#include "SwapChain.h"
#include "RenderPass.h"

namespace Vulkan
{
  struct GraphicPipelineStageStructs
  {
    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    VkPipelineInputAssemblyStateCreateInfo input_assembly = {};
    VkViewport viewport;
    std::vector<VkRect2D> scissors;
    VkPipelineViewportStateCreateInfo viewport_state = {};
    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    VkPipelineMultisampleStateCreateInfo multisampling = {};
    std::vector<VkPipelineColorBlendAttachmentState> color_blend_attachments;
    VkPipelineColorBlendStateCreateInfo color_blending = {};
    std::vector<VkDynamicState> dynamic_states;
    VkPipelineDynamicStateCreateInfo dynamic_state = {};
  };

  class GraphicPipeline
  {
  private:
    std::shared_ptr<Vulkan::Device> device;
    std::shared_ptr<Vulkan::SwapChain> swapchain;
    std::shared_ptr<Vulkan::RenderPass> render_pass;
    VkShaderModule vertex_shader = VK_NULL_HANDLE;
    VkShaderModule fragment_shader = VK_NULL_HANDLE;
    VkShaderModule compute_shader = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    std::vector<VkPipelineShaderStageCreateInfo> stage_infos;
    std::vector<Vulkan::ShaderInfo> shader_infos;
    std::vector<VkVertexInputBindingDescription> binding_description;
    std::vector<VkVertexInputAttributeDescription> attribute_descriptions;
    void CreateShaderStageInfos();
    void FillGraphicPipelineStageStructs(GraphicPipelineStageStructs &pipeline_stage_struct);
    void BuildGraphicPipeline();
    std::vector<VkFramebuffer> CreateFrameBuffers();
    void Create();
    void Destroy();
  public:
    GraphicPipeline() = delete;
    GraphicPipeline(const GraphicPipeline &obj) = delete;
    GraphicPipeline& operator= (const GraphicPipeline &obj) = delete;
    GraphicPipeline(std::shared_ptr<Vulkan::Device> dev, std::shared_ptr<Vulkan::SwapChain> swapchain, std::shared_ptr<Vulkan::RenderPass> render_pass);
    VkPipeline GetPipeline();
    void ReBuildPipeline();
    void SetShaderInfos(std::vector<Vulkan::ShaderInfo> shader_infos);
    void SetVertexInputBindingDescription(std::vector<VkVertexInputBindingDescription> binding_description, std::vector<VkVertexInputAttributeDescription> attribute_descriptions);
    ~GraphicPipeline();
  };
}

#endif