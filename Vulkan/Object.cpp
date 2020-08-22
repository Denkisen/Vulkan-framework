#include "Object.h"

#define TINYOBJLOADER_IMPLEMENTATION
#include "../libs/tiny_obj_loader.h"

#include <algorithm>
#include <unordered_map>

namespace Vulkan
{
  Object::Object(std::shared_ptr<Vulkan::Device> dev, std::shared_ptr<Vulkan::CommandPool> pool)
  {
    device = dev;
    command_pool = pool;
  }

  std::string Object::GetFileExtention(const std::string file)
  {
    size_t pos = file.find_last_of(".");
    if (pos == file.size())
      return "";

    std::string ext = file.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c){ return std::tolower(c); });

    return ext;
  }

  void Object::LoadFromFiles(const std::string model_file_path, const std::string material_file_path, const std::string texture_file_path)
  {
    auto buffer_lock = command_pool->OrderBufferLock();
    command_pool->BeginCommandBuffer(buffer_lock);
    PrepareTexture(texture_file_path, buffer_lock);
    PrepareModel(model_file_path, material_file_path, buffer_lock);
    command_pool->EndCommandBuffer(buffer_lock);
    command_pool->ExecuteBuffer(buffer_lock);
    command_pool->ReleaseBufferLock(buffer_lock);
    vertex_src_buffer.reset();
    index_src_buffer.reset();
    texture_src_buffer.reset();
  }

  void Object::PrepareModel(const std::string model_file_path, const std::string material_file_path, const Vulkan::BufferLock buffer_lock)
  {
    auto model_ext = GetFileExtention(model_file_path);
    if (model_ext != ".obj")
      throw std::runtime_error("Unsupported format");

    vertex_buffer = std::make_shared<Vulkan::Buffer<Vulkan::Vertex>>(device, Vulkan::StorageType::Vertex, 
                                                                    Vulkan::HostVisibleMemory::HostInvisible);
    index_buffer = std::make_shared<Vulkan::Buffer<uint32_t>>(device, Vulkan::StorageType::Index,
                                                              Vulkan::HostVisibleMemory::HostInvisible);
    vertex_src_buffer = std::make_shared<Vulkan::Buffer<Vulkan::Vertex>>(device, Vulkan::StorageType::Vertex,
                                                                              Vulkan::HostVisibleMemory::HostVisible);
    index_src_buffer = std::make_shared<Vulkan::Buffer<uint32_t>>(device, Vulkan::StorageType::Index,
                                                                      Vulkan::HostVisibleMemory::HostVisible);

    if (model_ext == ".obj")
    {
      tinyobj::attrib_t attrib;
      std::vector<tinyobj::shape_t> shapes;
      std::vector<tinyobj::material_t> materials;
      std::string warn, err;
      std::vector<uint32_t> indices;
      std::unordered_map<Vulkan::Vertex, uint32_t> vertices;
      std::vector<Vulkan::Vertex> vertices_buff;

      if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, model_file_path.c_str(), material_file_path.c_str())) 
        throw std::runtime_error(warn + err);

      for (const auto& shape : shapes) 
      {
        for (const auto& index : shape.mesh.indices) 
        {
          Vulkan::Vertex vertex = {};
          vertex.pos = 
          {
            attrib.vertices[3 * index.vertex_index + 0],
            attrib.vertices[3 * index.vertex_index + 1],
            attrib.vertices[3 * index.vertex_index + 2]
          };

          vertex.texCoord = 
          {
            attrib.texcoords[2 * index.texcoord_index + 0],
            1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
          };

          vertex.color = {1.0f, 1.0f, 1.0f};

          if (vertices.count(vertex) == 0)
          {
            vertices[vertex] = uint32_t (vertices_buff.size());
            vertices_buff.push_back(vertex);
          }

          indices.push_back(vertices[vertex]);
        }
      }

      *index_buffer = indices;
      *index_src_buffer = indices;
      *vertex_buffer = vertices_buff;
      *vertex_src_buffer = vertices_buff;
    }

    VkBufferCopy copy_region = {};
    copy_region.srcOffset = 0;
    copy_region.dstOffset = 0;
    copy_region.size = std::min(vertex_src_buffer->BufferLength(), vertex_buffer->BufferLength());
    command_pool->CopyBuffer(buffer_lock, vertex_src_buffer, vertex_buffer, {copy_region});
    copy_region.size = std::min(index_src_buffer->BufferLength(), index_buffer->BufferLength());
    command_pool->CopyBuffer(buffer_lock, index_src_buffer, index_buffer, {copy_region});
  }

  void Object::PrepareTexture(const std::string texture_file_path, const Vulkan::BufferLock buffer_lock)
  {
    ImageBuffer texture_loader;
    texture_loader.Load(texture_file_path);
    texture_image = std::make_shared<Vulkan::Image>(device, texture_loader.Width(), 
                                                    texture_loader.Height(),
                                                    true, 
                                                    Vulkan::ImageTiling::Optimal, 
                                                    Vulkan::HostVisibleMemory::HostInvisible,
                                                    Vulkan::ImageType::Sampled);
    texture_src_buffer = std::make_shared<Vulkan::Buffer<uint8_t>>(device, Vulkan::StorageType::Storage, 
                                                                        Vulkan::HostVisibleMemory::HostVisible);
    *texture_src_buffer = texture_loader.Canvas();
    sampler = std::make_shared<Vulkan::Sampler>(device, texture_image->GetMipLevels());

    VkBufferImageCopy image_region = {};
    image_region.bufferOffset = 0;
    image_region.bufferRowLength = 0;
    image_region.bufferImageHeight = 0;

    image_region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    image_region.imageSubresource.mipLevel = 0;
    image_region.imageSubresource.baseArrayLayer = 0;
    image_region.imageSubresource.layerCount = 1;

    image_region.imageOffset = {0, 0, 0};
    image_region.imageExtent = 
    {
      (uint32_t) texture_image->Width(),
      (uint32_t) texture_image->Height(),
      1
    };

    command_pool->TransitionImageLayout(buffer_lock, texture_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    texture_image->SetLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    command_pool->CopyBufferToImage(buffer_lock, texture_src_buffer, texture_image, {image_region});
    if (texture_image->GetMipLevels() > 1)
      command_pool->GenerateMipLevels(buffer_lock, texture_image, sampler);
    command_pool->TransitionImageLayout(buffer_lock, texture_image, texture_image->GetLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    texture_image->SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
  }
}