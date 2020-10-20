#include <iostream>
#include <gtest/gtest.h>

#include "Vulkan_/Instance.h"
#include "Vulkan_/Device.h"
#include "Vulkan_/Surface.h"


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

  swap(test1, test2);

  EXPECT_EQ((VkPhysicalDeviceType) Vulkan::PhysicalDeviceType::Discrete, test2.GetPhysicalDeviceProperties().deviceType);
  EXPECT_EQ((VkPhysicalDeviceType) Vulkan::PhysicalDeviceType::Integrated, test1.GetPhysicalDeviceProperties().deviceType);
  EXPECT_NE(VK_NULL_HANDLE, (uint64_t) test2.GetDevice());
  EXPECT_NE(VK_NULL_HANDLE, (uint64_t) test1.GetDevice());

  Vulkan::Device test3(test2);

  EXPECT_NE(VK_NULL_HANDLE, (uint64_t) test3.GetDevice());
  EXPECT_EQ((VkPhysicalDeviceType) Vulkan::PhysicalDeviceType::Discrete, test3.GetPhysicalDeviceProperties().deviceType);

  Vulkan::Device test4(std::move(test3));

  EXPECT_NE(VK_NULL_HANDLE, (uint64_t) test4.GetDevice());
  EXPECT_EQ((VkPhysicalDeviceType) Vulkan::PhysicalDeviceType::Discrete, test4.GetPhysicalDeviceProperties().deviceType);

  test3 = std::move(test4);

  EXPECT_NE(VK_NULL_HANDLE, (uint64_t) test3.GetDevice());
  EXPECT_EQ((VkPhysicalDeviceType) Vulkan::PhysicalDeviceType::Discrete, test3.GetPhysicalDeviceProperties().deviceType);
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


int main(int argc, char *argv[])
{
  testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}