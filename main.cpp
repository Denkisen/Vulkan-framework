#include <iostream>

#include "Vulkan/Array.h"
#include "Vulkan/UniformBuffer.h"
#include "Vulkan/Instance.h"
#include "Vulkan/Device.h"
#include "Vulkan/Offload.h"

int main(int argc, char const *argv[])
{
  try
  {
    Vulkan::Instance instance;
    Vulkan::Device device = Vulkan::Device(instance, Vulkan::Discrete);
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}
