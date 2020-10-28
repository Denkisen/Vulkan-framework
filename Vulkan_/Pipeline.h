#ifndef __VULKAN_PIPELINE_H
#define __VULKAN_PIPELINE_H

#include <vulkan/vulkan.h>
#include <memory>
#include <vector>
#include <mutex>

#include "Device.h"
#include "Logger.h"

namespace Vulkan
{
  enum class ShaderType
  {
    Vertex = VK_SHADER_STAGE_VERTEX_BIT,
    Fragment = VK_SHADER_STAGE_FRAGMENT_BIT,
    Compute = VK_SHADER_STAGE_COMPUTE_BIT,
  };

  struct ShaderInfo
  {
    std::string entry = "main";
    std::string file_path = "";
    Vulkan::ShaderType type;
  };

  struct Shader
  {
    VkShaderModule shader = VK_NULL_HANDLE;
    ShaderInfo info;
  };

  class Pipeline
  {
  protected:
    VkPipeline pipeline = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
    std::shared_ptr<Vulkan::Device> device;
    std::vector<Shader> shaders;
    std::vector<VkDescriptorSetLayout> layouts;
    Pipeline(std::shared_ptr<Vulkan::Device> dev);
    VkShaderModule CreateShader(const std::string shader_path);
    VkPipelineLayout CreatePipelineLayout();
    virtual VkPipeline BuildPipeline() { return VK_NULL_HANDLE; }
  public:
    Pipeline() = delete;
    Pipeline(const Pipeline &obj) = delete;
    Pipeline(Pipeline &&obj) = delete;
    Pipeline &operator=(const Pipeline &obj) = delete;
    Pipeline &operator=(Pipeline &&obj) = delete;
    VkPipeline GetPipeline();
    VkPipelineLayout GetPipelineLayout();
    virtual VkResult AddShader(const ShaderInfo info);
    VkResult AddDescriptorSetLayouts(const std::vector<VkDescriptorSetLayout> layouts);
    virtual ~Pipeline(); 
  };

  class ComputePipeline : public Pipeline
  {
  private:
    VkPipeline BuildPipeline() override;
  public:
    ComputePipeline() = delete;
    ComputePipeline(const ComputePipeline &obj) = delete;
    ComputePipeline(ComputePipeline &&obj) = delete;
    ComputePipeline(std::shared_ptr<Vulkan::Device> dev) : Pipeline(dev) {};
    ComputePipeline &operator=(const ComputePipeline &obj) = delete;
    ComputePipeline &operator=(ComputePipeline &&obj) = delete;
    VkResult AddShader(const ShaderInfo info) override;
    ~ComputePipeline(); 
  };

  class GraphicPipeline : public Pipeline
  {
  private:
    VkPipeline BuildPipeline() override;
  public:
    GraphicPipeline() = delete;
    GraphicPipeline(const GraphicPipeline &obj) = delete;
    GraphicPipeline(GraphicPipeline &&obj) = delete;
    GraphicPipeline(std::shared_ptr<Vulkan::Device> dev) : Pipeline(dev) {};
    GraphicPipeline &operator=(const GraphicPipeline &obj) = delete;
    GraphicPipeline &operator=(GraphicPipeline &&obj) = delete;
    VkResult AddShader(const ShaderInfo info) override;
    ~GraphicPipeline()
    {
      Logger::EchoDebug("", __func__);
    }
  };
}

#endif