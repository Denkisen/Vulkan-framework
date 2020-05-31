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
    Vulkan::Device device(instance, Vulkan::Discrete);
    Vulkan::Array<float> input(device);
    Vulkan::Array<float> output(device);
    input = std::vector<float>(64, 5.0);
    output = std::vector<float>(64, 0.0);
    struct UniformData
    {
      unsigned mul;
      unsigned val[63];
    };
    UniformData global_data = {};
    global_data.mul = 4;
    Vulkan::UniformBuffer global(device, &global_data, sizeof(UniformData));
    std::vector<Vulkan::IStorage*> data;
    data.push_back(&input);
    data.push_back(&output);
    data.push_back(&global);
    Vulkan::Offload<float> offload = Vulkan::Offload<float>(device, "bin/comp.spv", "main");
    offload = data;
    offload.Run(64, 1, 1);

    auto out = output.Extract();
    std::cout << "Output:" << std::endl;
    for (size_t i = 0; i < out.size(); ++i)
    {
      std::cout << out[i] << " ";
    }
    std::cout << std::endl;
  }
  catch(const std::exception& e)
  {
    std::cerr << e.what() << '\n';
  }
  return 0;
}
