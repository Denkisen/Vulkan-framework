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
    std::shared_ptr<Vulkan::Array<float>> input = std::make_shared<Vulkan::Array<float>>(device, Vulkan::StorageType::Storage);
    std::shared_ptr<Vulkan::Array<float>> output = std::make_shared<Vulkan::Array<float>>(device, Vulkan::StorageType::Storage);
    *input = std::vector<float>(64, 5.0);
    *output = std::vector<float>(64, 0.0);
    struct UniformData
    {
      unsigned mul;
      unsigned val[63];
    };
    UniformData global_data = {};
    global_data.mul = 5;
    std::shared_ptr<Vulkan::UniformBuffer<UniformData>> global = std::make_shared<Vulkan::UniformBuffer<UniformData>>(device);
    *global = global_data;
    std::vector<std::shared_ptr<Vulkan::IStorage>> data;
    data.push_back(input);
    data.push_back(output);
    data.push_back(global);
    Vulkan::OffloadPipelineOptions opts;
    opts.DispatchTimes = 3;
    Vulkan::UpdateBufferOpt uni_opts;
    uni_opts.index = data.size() - 1;
    uni_opts.OnDispatchEndEvent = [&data](const std::size_t iteration, const std::size_t index, const std::size_t element)
    {
      std::cout << "EndEvent" << std::endl;
      if (data[element]->Type() == Vulkan::StorageType::Uniform)
      {
        auto t = ((Vulkan::UniformBuffer<UniformData>*) data[element].get())->Extract();
        t.mul += iteration;
        *((Vulkan::UniformBuffer<UniformData>*) data[element].get()) = t;
      }
    };
    opts.DispatchEndEvents.push_back(uni_opts);

    Vulkan::Offload<float> offload = Vulkan::Offload<float>(device, "bin/comp.spv", "main");
    offload.SetPipelineOptions(opts);
    offload = data;
    offload.Run(64, 1, 1);

    auto out = output->Extract();
    std::cout << "Output:" << std::endl;
    for (size_t i = 0; i < out.size(); ++i)
    {
      std::cout << out[i] << " ";
    }
    std::cout << std::endl;
  }
  catch(const std::exception& e)
  {
    std::cerr << "Error: " << e.what() << '\n';
  }
  return 0;
}
