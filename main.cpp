#include <iostream>
#include <vector>
#include <memory>
#include <gtest/gtest.h>

#include "Vulkan_/Instance.h"
#include "Vulkan_/Device.h"
#include "Vulkan_/Surface.h"
#include "Vulkan_/Array.h"
#include "Vulkan_/Descriptors.h"

struct UniformData
{
  unsigned mul;
  unsigned val[63];
};

TEST (Vulkan, Instance) 
{
  EXPECT_NE (VK_NULL_HANDLE, (uint64_t) Vulkan::Instance::GetInstance());
  EXPECT_NE (VK_NULL_HANDLE, (uint64_t) Vulkan::Instance::GetInstance());
}

TEST (Vulkan, Device)
{
  Vulkan::Device test1(Vulkan::DeviceConfig()
                      .SetDeviceType(Vulkan::PhysicalDeviceType::Discrete)
                      .SetQueueType(Vulkan::QueueType::ComputeType));
  Vulkan::Device test2(Vulkan::DeviceConfig()
                      .SetQueueType(Vulkan::QueueType::ComputeType)
                      .SetDeviceType(Vulkan::PhysicalDeviceType::Integrated));
  
  EXPECT_NE(VK_NULL_HANDLE, (uint64_t) test1.GetPhysicalDevice());
  EXPECT_NE(VK_NULL_HANDLE, (uint64_t) test1.GetDevice());
  EXPECT_EQ((VkPhysicalDeviceType) Vulkan::PhysicalDeviceType::Discrete, test1.GetPhysicalDeviceProperties().deviceType);

  EXPECT_NE(VK_NULL_HANDLE, (uint64_t) test2.GetPhysicalDevice());
  EXPECT_NE(VK_NULL_HANDLE, (uint64_t) test2.GetDevice());
  EXPECT_EQ((VkPhysicalDeviceType) Vulkan::PhysicalDeviceType::Integrated, test2.GetPhysicalDeviceProperties().deviceType);

  Vulkan::Device test3(test2);

  EXPECT_NE(VK_NULL_HANDLE, (uint64_t) test3.GetDevice());
  EXPECT_EQ((VkPhysicalDeviceType) Vulkan::PhysicalDeviceType::Integrated, test3.GetPhysicalDeviceProperties().deviceType);
}

TEST (Vulkan, Surface)
{
  std::shared_ptr<Vulkan::Surface> surf1 = std::make_shared<Vulkan::Surface>(Vulkan::SurfaceConfig().SetWidght(1200).SetHeight(800).SetAppTitle("Test"));

  EXPECT_NE(VK_NULL_HANDLE, (uint64_t) surf1->GetSurface());
  EXPECT_NE(nullptr, surf1->GetWindow());

  VkPhysicalDeviceFeatures f = {};
  f.geometryShader = VK_TRUE;
  Vulkan::Device dev1(Vulkan::DeviceConfig()
                      .SetDeviceType(Vulkan::PhysicalDeviceType::Discrete)
                      .SetQueueType(Vulkan::QueueType::DrawingType)
                      .SetSurface(surf1)
                      .SetRequiredDeviceFeatures(f));
  
  EXPECT_NE(VK_NULL_HANDLE, (uint64_t) dev1.GetDevice());
  EXPECT_EQ((VkPhysicalDeviceType) Vulkan::PhysicalDeviceType::Discrete, dev1.GetPhysicalDeviceProperties().deviceType);
  EXPECT_EQ(surf1->GetSurface(), dev1.GetSurface());
}

TEST (Vulkan, Array)
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
  Vulkan::Array array1(dev);
  EXPECT_EQ(array1.StartConfig(Vulkan::HostVisibleMemory::HostVisible), VK_SUCCESS);
  EXPECT_EQ(array1.AddBuffer(Vulkan::BufferConfig()
                  .SetType(Vulkan::StorageType::Storage)
                  .AddSubBufferRange(2, test_data1.size(), sizeof(float))), VK_SUCCESS);
  EXPECT_EQ(array1.AddBuffer(Vulkan::BufferConfig()
                  .SetType(Vulkan::StorageType::Storage)
                  .AddSubBuffer(test_data3.size(), sizeof(float))
                  .AddSubBuffer(test_data4.size(), sizeof(float))), VK_SUCCESS);
  EXPECT_EQ(array1.EndConfig(), VK_SUCCESS);
#ifdef DEBUG
  array1.UseChunkedMapping(false);
#endif
  EXPECT_EQ(array1.SetSubBufferData(0, 0, test_data1), VK_SUCCESS);
  EXPECT_EQ(array1.SetSubBufferData(0, 1, test_data2), VK_SUCCESS);
  EXPECT_EQ(array1.SetBufferData(1, tmp), VK_SUCCESS);

  Vulkan::Array array2(array1);

  tmp.clear();
  EXPECT_EQ(array2.GetBufferData(0, tmp), VK_SUCCESS);
  EXPECT_EQ(tmp.size(), test_data1.size() + test_data2.size());
  EXPECT_EQ(array2.GetSubBufferData(1, 0, test_data1), VK_SUCCESS);
  EXPECT_EQ(array2.GetSubBufferData(1, 1, test_data2), VK_SUCCESS);

  std::cout << "Output:" << std::endl;
  for (size_t i = 0; i < test_data4.size(); ++i)
  {
    std::cout << tmp[i] << " ";
  }
  std::cout << std::endl << "Next:" << std::endl;
  for (size_t i = 0; i < test_data4.size(); ++i)
  {
    std::cout << tmp[test_data4.size() + i] << " ";
  }
  std::cout << std::endl << "Next:" << std::endl;
  for (size_t i = 0; i < test_data1.size(); ++i)
  {
    std::cout << test_data1[i] << " ";
  }
  std::cout << std::endl << "Next:" << std::endl;
  for (size_t i = 0; i < test_data2.size(); ++i)
  {
    std::cout << test_data2[i] << " ";
  }
  std::cout << std::endl;
}

TEST (Vulkan, Descriptors)
{
  std::shared_ptr<Vulkan::Device> dev = std::make_shared<Vulkan::Device>(Vulkan::DeviceConfig()
                                          .SetDeviceType(Vulkan::PhysicalDeviceType::Discrete)
                                          .SetQueueType(Vulkan::QueueType::ComputeType));
  
  std::vector<float> test_data1(256, 5.0);
  std::vector<float> test_data2(256, 6.0);
  Vulkan::Array array1(dev);

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
  conf.AddBuffer(info);
  info.size = array1.GetInfo(0).sub_buffers[1].size;
  info.offset = array1.GetInfo(0).sub_buffers[1].offset;
  info.buffer_info.buffer_view = array1.GetInfo(0).sub_buffers[1].view;
  conf.AddBuffer(info);

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
}


int main(int argc, char *argv[])
{
  testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}