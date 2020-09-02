#include <iostream>
#include <memory>
#include <optional>

#include "Vulkan/Instance.h"
#include "Vulkan/Device.h"
#include "Vulkan/Compute.h"
#include "Vulkan/Buffer.h"
#include "Vulkan/CommandPool.h"
#include "Vulkan/BufferArray.h"

struct UniformData
{
  unsigned mul;
  unsigned val[63];
};

int main(int argc, char const *argv[])
{
  try
  {
    Vulkan::Instance instance;
    std::shared_ptr<Vulkan::Device> device = std::make_shared<Vulkan::Device>(Vulkan::PhysicalDeviceType::Discrete);

    std::shared_ptr<Vulkan::BufferArray> test = std::make_shared<Vulkan::BufferArray>(device);
    UniformData global_data = {};
    global_data.mul = 5;

    test->DeclareBuffer(64 * 64 * sizeof(float), Vulkan::HostVisibleMemory::HostVisible, Vulkan::StorageType::Storage);
    test->DeclareVirtualBuffer(0, 0, 64 * sizeof(float));
    test->DeclareVirtualBuffer(0, 64 * sizeof(float), 64 * sizeof(float));
    test->DeclareBuffer(sizeof(UniformData), Vulkan::HostVisibleMemory::HostVisible, Vulkan::StorageType::Uniform);
    test->DeclareVirtualBuffer(1, 0, sizeof(UniformData));

    test->TrySetValue(0, 0, std::vector<float>(64, 5.0));
    test->TrySetValue(0, 1, std::vector<float>(64, 0.0));
    test->TrySetValue(1, 0, std::vector<UniformData>{global_data});

    Vulkan::ComputePipelineOptions opts;
    opts.DispatchTimes = 3;
    Vulkan::UpdateBufferOpt uni_opts;
    uni_opts.index = test->Count() - 1;
    uni_opts.OnDispatchEndEvent = [&test](const std::size_t iteration, const std::size_t index, const std::size_t element)
    {
      std::cout << "EndEvent" << std::endl;
      if (test->BufferType(element) == Vulkan::StorageType::Uniform)
      {
        std::vector<UniformData> t;
        test->TryGetValue(element, 0, t);
        t[0].mul += iteration;
        test->TrySetValue(element, 0, t);
      }
    };
    opts.DispatchEndEvents.push_back(uni_opts);

    std::string path = Vulkan::Supply::GetExecDirectory(argv[0]);

    Vulkan::Compute offload = Vulkan::Compute(device, path + "test.comp.spv", "main");
    offload.SetPipelineOptions(opts);
    offload = test;
    offload.Run(64, 1, 1);

    std::vector<float> out;
    test->TryGetValue(0, 1, out);

    std::cout << "Output:" << out.size() << std::endl;
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