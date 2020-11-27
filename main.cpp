#include <iostream>
#include <vector>
#include <memory>
#include <optional>
#include <gtest/gtest.h>

#include "Vulkan/Instance.h"
#include "Vulkan/Device.h"
#include "Vulkan/Surface.h"
#include "Vulkan/StorageArray.h"
#include "Vulkan/Descriptors.h"
#include "Vulkan/CommandPool.h"
#include "Vulkan/Pipelines.h"
#include "Vulkan/Misc.h"
#include "Vulkan/RenderPass.h"
#include "Vulkan/ImageArray.h"
#include "Vulkan/Fence.h"

struct UniformData
{
  unsigned mul;
  unsigned val[63];
};

TEST (Vulkan, StorageArray)
{
  std::vector<float> test_data1(256, 5.0);
  std::vector<float> test_data2(256, 6.0);
  std::vector<float> test_data3(256, 7.0);
  std::vector<float> test_data4(256, 8.0);
  std::vector<float> tmp;
  tmp.reserve(test_data3.size() + test_data4.size());
  tmp.insert(tmp.end(), test_data3.begin(), test_data3.end());
  tmp.insert(tmp.end(), test_data4.begin(), test_data4.end());
  std::shared_ptr<Vulkan::Device> dev = std::make_shared<Vulkan::Device>(Vulkan::DeviceConfig()
                                          .SetDeviceType(Vulkan::PhysicalDeviceType::Discrete)
                                          .SetQueueType(Vulkan::QueueType::ComputeType));
  Vulkan::StorageArray array1(dev);
  EXPECT_EQ(array1.StartConfig(Vulkan::HostVisibleMemory::HostVisible), VK_SUCCESS);
  EXPECT_EQ(array1.AddBuffer(Vulkan::BufferConfig()
                  .SetType(Vulkan::StorageType::Storage)
                  .AddSubBufferRange(2, test_data1.size(), sizeof(float))), VK_SUCCESS);
  EXPECT_EQ(array1.AddBuffer(Vulkan::BufferConfig()
                  .SetType(Vulkan::StorageType::Storage)
                  .AddSubBuffer(test_data3.size(), sizeof(float))
                  .AddSubBuffer(test_data4.size(), sizeof(float))), VK_SUCCESS);
  EXPECT_EQ(array1.EndConfig(), VK_SUCCESS);

  EXPECT_EQ(array1.SetSubBufferData(0, 0, test_data1), VK_SUCCESS);
  EXPECT_EQ(array1.SetSubBufferData(0, 1, test_data2), VK_SUCCESS);
  EXPECT_EQ(array1.SetBufferData(1, tmp), VK_SUCCESS);

  Vulkan::StorageArray array2(array1);

  tmp.clear();
  EXPECT_EQ(array2.GetBufferData(0, tmp), VK_SUCCESS);
  EXPECT_EQ(tmp.size(), test_data1.size() + test_data2.size());
  EXPECT_EQ(array2.GetSubBufferData(1, 0, test_data1), VK_SUCCESS);
  EXPECT_EQ(array2.GetSubBufferData(1, 1, test_data2), VK_SUCCESS);
}

TEST (Vulkan, Descriptors)
{
  std::shared_ptr<Vulkan::Device> dev = std::make_shared<Vulkan::Device>(Vulkan::DeviceConfig()
                                          .SetDeviceType(Vulkan::PhysicalDeviceType::Discrete)
                                          .SetQueueType(Vulkan::QueueType::ComputeType));
  
  std::vector<float> test_data1(256, 5.0);
  std::vector<float> test_data2(256, 6.0);
  Vulkan::StorageArray array1(dev);

  EXPECT_EQ(array1.StartConfig(Vulkan::HostVisibleMemory::HostVisible), VK_SUCCESS);
  EXPECT_EQ(array1.AddBuffer(Vulkan::BufferConfig()
                  .SetType(Vulkan::StorageType::Storage)
                  .AddSubBufferRange(2, test_data1.size(), sizeof(float))), VK_SUCCESS);
  EXPECT_EQ(array1.EndConfig(), VK_SUCCESS);

  EXPECT_EQ(array1.SetSubBufferData(0, 0, test_data1), VK_SUCCESS);
  EXPECT_EQ(array1.SetSubBufferData(0, 1, test_data2), VK_SUCCESS);

  Vulkan::Descriptors desc(dev);

  Vulkan::LayoutConfig conf;
  Vulkan::DescriptorInfo info = {};
  info.type = info.MapStorageType(array1.GetInfo(0).type);
  info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  info.size = array1.GetInfo(0).sub_buffers[0].size;
  info.offset = array1.GetInfo(0).sub_buffers[0].offset;
  info.buffer_info.buffer = array1.GetInfo(0).buffer;
  info.buffer_info.buffer_view = array1.GetInfo(0).sub_buffers[0].view;
  conf.AddBufferOrImage(info);
  info.size = array1.GetInfo(0).sub_buffers[1].size;
  info.offset = array1.GetInfo(0).sub_buffers[1].offset;
  info.buffer_info.buffer_view = array1.GetInfo(0).sub_buffers[1].view;
  conf.AddBufferOrImage(info);

  EXPECT_EQ(desc.AddSetLayoutConfig(conf), VK_SUCCESS);
  EXPECT_EQ(desc.BuildAllSetLayoutConfigs(), VK_SUCCESS);
  EXPECT_EQ(desc.GetLayoutsCount(), 1);
  EXPECT_NE(desc.GetDescriptorSet(0), (VkDescriptorSet) VK_NULL_HANDLE);
  EXPECT_NE(desc.GetDescriptorSetLayout(0), (VkDescriptorSetLayout) VK_NULL_HANDLE);

  Vulkan::Descriptors desc1(desc);

  EXPECT_EQ(desc1.AddSetLayoutConfig(conf), VK_SUCCESS);
  EXPECT_EQ(desc1.BuildAllSetLayoutConfigs(), VK_SUCCESS);
  EXPECT_EQ(desc1.GetLayoutsCount(), 1);
  EXPECT_NE(desc1.GetDescriptorSet(0), (VkDescriptorSet) VK_NULL_HANDLE);
  EXPECT_NE(desc1.GetDescriptorSetLayout(0), (VkDescriptorSetLayout) VK_NULL_HANDLE);

  Vulkan::Descriptors desc2(dev);
  desc2 = desc;

  EXPECT_EQ(desc1.AddSetLayoutConfig(conf), VK_SUCCESS);
  EXPECT_EQ(desc1.BuildAllSetLayoutConfigs(), VK_SUCCESS);
  EXPECT_EQ(desc1.GetLayoutsCount(), 1);
  EXPECT_NE(desc1.GetDescriptorSet(0), (VkDescriptorSet) VK_NULL_HANDLE);
  EXPECT_NE(desc1.GetDescriptorSetLayout(0), (VkDescriptorSetLayout) VK_NULL_HANDLE);
}

TEST (Vulkan, ComputePipeline)
{
  std::shared_ptr<Vulkan::Device> dev = std::make_shared<Vulkan::Device>(Vulkan::DeviceConfig()
                                          .SetDeviceType(Vulkan::PhysicalDeviceType::Discrete)
                                          .SetQueueType(Vulkan::QueueType::ComputeType));
  std::vector<float> input(256, 5.0);
  UniformData udata = {};
  udata.mul = 3;
  
  Vulkan::StorageArray array1(dev);
  EXPECT_EQ(array1.StartConfig(), VK_SUCCESS);
  EXPECT_EQ(array1.AddBuffer(Vulkan::BufferConfig().AddSubBufferRange(2, input)), VK_SUCCESS);
  EXPECT_EQ(array1.AddBuffer(Vulkan::BufferConfig().AddSubBuffer(1, sizeof(UniformData)).SetType(Vulkan::StorageType::Uniform)), VK_SUCCESS);
  EXPECT_EQ(array1.EndConfig(), VK_SUCCESS);
  EXPECT_EQ(array1.SetSubBufferData(0, 0, input), VK_SUCCESS);
  EXPECT_EQ(array1.SetBufferData<UniformData>(1, { udata }), VK_SUCCESS);

  Vulkan::Descriptors desc(dev);
  Vulkan::DescriptorInfo d_info = {};
  d_info.buffer_info.buffer = array1.GetInfo(0).buffer;
  d_info.stage = VK_SHADER_STAGE_COMPUTE_BIT;
  d_info.type = d_info.MapStorageType(array1.GetInfo(0).type);

  d_info.size = array1.GetInfo(0).sub_buffers[0].size;
  d_info.offset = array1.GetInfo(0).sub_buffers[0].offset;
  EXPECT_EQ(desc.AddSetLayoutConfig(Vulkan::LayoutConfig().AddBufferOrImage(d_info)), VK_SUCCESS);

  d_info.size = array1.GetInfo(0).sub_buffers[1].size;
  d_info.offset = array1.GetInfo(0).sub_buffers[1].offset;
  EXPECT_EQ(desc.AddSetLayoutConfig(Vulkan::LayoutConfig().AddBufferOrImage(d_info)), VK_SUCCESS);

  d_info.size = array1.GetInfo(1).size;
  d_info.offset = 0;
  d_info.type = d_info.MapStorageType(array1.GetInfo(1).type);
  d_info.buffer_info.buffer = array1.GetInfo(1).buffer;
  EXPECT_EQ(desc.AddSetLayoutConfig(Vulkan::LayoutConfig().AddBufferOrImage(d_info)), VK_SUCCESS);

  EXPECT_EQ(desc.BuildAllSetLayoutConfigs(), VK_SUCCESS);

  Vulkan::Pipelines pipelines;
  EXPECT_EQ(pipelines.AddPipeline(dev, Vulkan::ComputePipelineConfig()
                              .SetShader("test.comp.spv", "main")
                              .AddDescriptorSetLayouts(desc.GetDescriptorSetLayouts())), VK_SUCCESS);

  Vulkan::ComputePipeline c_pipe(dev, Vulkan::ComputePipelineConfig()
                              .SetShader("test.comp.spv", "main")
                              .SetBasePipeline(pipelines.GetPipeline(0))
                              .AddDescriptorSetLayouts(desc.GetDescriptorSetLayouts()));

  EXPECT_NE(c_pipe.GetPipeline(), (VkPipeline) VK_NULL_HANDLE);
  EXPECT_NE(pipelines.GetPipeline(0), (VkPipeline) VK_NULL_HANDLE);
  EXPECT_EQ(pipelines.AddPipeline(std::move(c_pipe)), VK_SUCCESS);
  EXPECT_NE(pipelines.GetPipeline(1), (VkPipeline) VK_NULL_HANDLE);

  Vulkan::Pipelines pipelines2(std::move(pipelines));

  EXPECT_EQ(c_pipe.IsValid(), false);

  Vulkan::CommandPool pool(dev, dev->GetComputeFamilyQueueIndex().value());
  pool.GetCommandBuffer(0, VK_COMMAND_BUFFER_LEVEL_PRIMARY)
      .BeginCommandBuffer()
      .BindPipeline(pipelines2.GetPipeline(1), VK_PIPELINE_BIND_POINT_COMPUTE)
      .BindDescriptorSets(pipelines2.GetLayout(1), VK_PIPELINE_BIND_POINT_COMPUTE, desc.GetDescriptorSets(), 0, {})
      .Dispatch(256, 1, 1)
      .EndCommandBuffer();

  EXPECT_EQ(pool.IsReady(0), true);

  if (Vulkan::Fence f(dev); f.IsValid())
  {
    EXPECT_EQ(pool.ExecuteBuffer(0, f.GetFence()), VK_SUCCESS);
    EXPECT_EQ(f.Wait(), VK_SUCCESS);
  }

  {
    Vulkan::FenceArray f(dev);
    EXPECT_EQ(f.Add(), VK_SUCCESS);
    if (auto ptr = f.GetFence(0); ptr != nullptr)
    {
      EXPECT_EQ(pool.ExecuteBuffer(0, ptr->GetFence()), VK_SUCCESS);
      EXPECT_EQ(f.WaitFor(), VK_SUCCESS);
    }
  }

  std::vector<float> output(256, 0.0);
  EXPECT_EQ(array1.GetSubBufferData(0, 1, output), VK_SUCCESS);

  std::cout << "Output:" << std::endl;
  for (size_t i = 0; i < output.size(); ++i)
  {
    std::cout << output[i] << " ";
  }
  std::cout << std::endl;
}

TEST (Vulkan, RenderPass)
{
  std::shared_ptr<Vulkan::Surface> surf = std::make_shared<Vulkan::Surface>(Vulkan::SurfaceConfig()
                                          .SetHeight(1042).SetWidght(1024));
  VkPhysicalDeviceFeatures f = {};
  f.geometryShader = VK_TRUE;
  std::shared_ptr<Vulkan::Device> dev = std::make_shared<Vulkan::Device>(Vulkan::DeviceConfig()
                                          .SetDeviceType(Vulkan::PhysicalDeviceType::Discrete)
                                          .SetQueueType(Vulkan::QueueType::DrawingType)
                                          .SetSurface(surf)
                                          .SetRequiredDeviceFeatures(f));

  std::shared_ptr<Vulkan::SwapChain> swapchain = std::make_shared<Vulkan::SwapChain>(dev, Vulkan::SwapChainConfig());

  Vulkan::ImageArray buffers(dev);
  std::shared_ptr<Vulkan::RenderPass> render_pass = Vulkan::Helpers::CreateOneSubpassRenderPassMultisamplingDepth(dev, swapchain, buffers, VK_SAMPLE_COUNT_4_BIT);

  EXPECT_NE(render_pass.get(), nullptr);
}

TEST (Vulkan, DISABLED_GraphicPipeline)
{
  std::shared_ptr<Vulkan::Surface> surf = std::make_shared<Vulkan::Surface>(Vulkan::SurfaceConfig()
                                          .SetHeight(1042).SetWidght(1024));
  VkPhysicalDeviceFeatures f = {};
  f.geometryShader = VK_TRUE;
  std::shared_ptr<Vulkan::Device> dev = std::make_shared<Vulkan::Device>(Vulkan::DeviceConfig()
                                          .SetDeviceType(Vulkan::PhysicalDeviceType::Discrete)
                                          .SetQueueType(Vulkan::QueueType::DrawingType)
                                          .SetSurface(surf)
                                          .SetRequiredDeviceFeatures(f));

  std::shared_ptr<Vulkan::SwapChain> swapchain = std::make_shared<Vulkan::SwapChain>(dev, Vulkan::SwapChainConfig());

  Vulkan::ImageArray buffers(dev);
  std::shared_ptr<Vulkan::RenderPass> render_pass = Vulkan::Helpers::CreateOneSubpassRenderPassMultisamplingDepth(dev, swapchain, buffers, VK_SAMPLE_COUNT_4_BIT);

  Vulkan::GraphicPipeline g_pipe(dev, swapchain, render_pass, Vulkan::GraphicPipelineConfig()
                                .SetSamplesCount(VK_SAMPLE_COUNT_4_BIT)
                                .AddShader(Vulkan::ShaderType::Vertex, "tri.vert.spv", "main")
                                .AddShader(Vulkan::ShaderType::Fragment, "tri.frag.spv", "main")
                                .UseDepthTesting(VK_TRUE)
                                .UseSampleShading(VK_TRUE)
                                .AddDynamicState(VK_DYNAMIC_STATE_VIEWPORT));
}

int main(int argc, char *argv[])
{
  testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}