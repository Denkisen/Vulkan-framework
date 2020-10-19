#include <iostream>
#include <gtest/gtest.h>

#include "Vulkan_/Instance.h"


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

int main(int argc, char *argv[])
{
  testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}