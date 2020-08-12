#include <iostream>
#include <memory>

#include "Vulkan/Instance.h"
#include "Vulkan/Device.h"
#include "Vulkan/Offload.h"
#include "Vulkan/Buffer.h"
#include "libs/ImageBuffer.h"

int main(int argc, char const *argv[])
{
  try
  {
    Vulkan::Instance instance;
    std::shared_ptr<Vulkan::Device> device = std::make_shared<Vulkan::Device>(Vulkan::Discrete);
    std::shared_ptr<Vulkan::Buffer<float>> input_src = std::make_shared<Vulkan::Buffer<float>>(device, Vulkan::StorageType::Storage, Vulkan::BufferUsage::Transfer_src);
    std::shared_ptr<Vulkan::Buffer<float>> input_dst = std::make_shared<Vulkan::Buffer<float>>(device, Vulkan::StorageType::Storage, Vulkan::BufferUsage::Transfer_dst);
    std::shared_ptr<Vulkan::Buffer<float>> output = std::make_shared<Vulkan::Buffer<float>>(device, Vulkan::StorageType::Storage, Vulkan::BufferUsage::Transfer_src);
    *input_src = std::vector<float>(64, 5.0);
    input_dst->AllocateBuffer(64);
    *output = std::vector<float>(64, 0.0);
    struct UniformData
    {
      unsigned mul;
      unsigned val[63];
    };
    UniformData global_data = {};
    global_data.mul = 5;
    std::shared_ptr<Vulkan::Buffer<UniformData>> global = std::make_shared<Vulkan::Buffer<UniformData>>(device, Vulkan::StorageType::Uniform, Vulkan::BufferUsage::Transfer_src);
    *global = global_data;
    std::vector<std::shared_ptr<Vulkan::IBuffer>> data;
    data.push_back(input_dst);
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
        auto t = ((Vulkan::Buffer<UniformData>*) data[element].get())->Extract(0);
        t.mul += iteration;
        *((Vulkan::Buffer<UniformData>*) data[element].get()) = t;
      }
    };
    opts.DispatchEndEvents.push_back(uni_opts);

    std::string path = Vulkan::Supply::GetExecDirectory(argv[0]);

    Vulkan::Offload<float> offload = Vulkan::Offload<float>(device, path + "test.comp.spv", "main");
    Vulkan::IBuffer::MoveData(device->GetDevice(), offload.GetCommandPool(), device->GetComputeQueue(), input_src, input_dst);
    offload.SetPipelineOptions(opts);
    offload = data;
    offload.Run(64, 1, 1);

    auto out = output->Extract();
    std::cout << "Output:" << out.size() << std::endl;
    for (size_t i = 0; i < out.size(); ++i)
    {
      std::cout << out[i] << " ";
    }
    std::cout << std::endl;

    offload.Run(64, 1, 1);

    out = output->Extract();
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
