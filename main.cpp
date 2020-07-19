#include <iostream>
#include <memory>

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
    std::shared_ptr<Vulkan::Device> device = std::make_shared<Vulkan::Device>(Vulkan::Discrete);
    Vulkan::Array<float> input(device, Vulkan::StorageType::Default);
    Vulkan::Array<float> output(device, Vulkan::StorageType::Default);
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
    Vulkan::OffloadPipelineOptions opts;
    opts.DispatchTimes = 3;
    Vulkan::UpdateBufferOpt uni_opts;
    uni_opts.index = data.size() - 1;
    uni_opts.OnDispatchEndEvent = [](const std::size_t iteration, const std::size_t index, const Vulkan::StorageType type, void *buff, const std::size_t length)
    {
      std::cout << "EndEvent" << std::endl;
      if (type == Vulkan::StorageType::Uniform)
      {
        UniformData *buffer = nullptr;
        buffer = (UniformData*) buff;
        if (buffer != nullptr)
          buffer->mul += iteration;
      }
    };
    opts.DispatchEndEvents.push_back(uni_opts);

    Vulkan::Offload<float> offload = Vulkan::Offload<float>(device, "bin/comp.spv", "main");
    offload.SetPipelineOptions(opts);
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
