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

  void Object::LoadModel(const std::string model_file_path, const std::string material_file_path)
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

    auto buffer_lock = command_pool->OrderBufferLock();
    command_pool->BeginCommandBuffer(buffer_lock);
    copy_region.size = std::min(vertex_src_buffer->BufferLength(), vertex_buffer->BufferLength());
    command_pool->CopyBuffer(buffer_lock, vertex_src_buffer, vertex_buffer, {copy_region});
    copy_region.size = std::min(index_src_buffer->BufferLength(), index_buffer->BufferLength());
    command_pool->CopyBuffer(buffer_lock, index_src_buffer, index_buffer, {copy_region});
    command_pool->EndCommandBuffer(buffer_lock);
    command_pool->ExecuteBuffer(buffer_lock);
    command_pool->ReleaseBufferLock(buffer_lock);

    vertex_src_buffer.reset();
    index_src_buffer.reset();
  }

  void Object::LoadTexture(const std::string texture_file_path, const bool enable_mip_levels)
  {
    ImageBuffer texture_loader;
    texture_loader.Load(texture_file_path);
    size_t tex_w = enable_mip_levels ? ((size_t) texture_loader.Width() * 2) / 3 : (size_t) texture_loader.Width();
    size_t tex_h = (size_t) texture_loader.Height();
    texture_image = std::make_shared<Vulkan::Image>(device, tex_w, tex_h, enable_mip_levels, 
                                                    Vulkan::ImageTiling::Optimal, 
                                                    Vulkan::HostVisibleMemory::HostInvisible,
                                                    Vulkan::ImageType::Sampled);
    texture_src_buffer = std::make_shared<Vulkan::Buffer<uint8_t>>(device, Vulkan::StorageType::Storage, 
                                                                  Vulkan::HostVisibleMemory::HostVisible);

    if (enable_mip_levels)
      *texture_src_buffer = texture_loader.GetMipLevelsBuffer();
    else
      *texture_src_buffer = texture_loader.Canvas();
    sampler = std::make_shared<Vulkan::Sampler>(device, texture_image->GetMipLevels());

    std::vector<VkBufferImageCopy> image_regions(texture_image->GetMipLevels());

    uint32_t buffer_offset = 0;
    for (size_t i = 0; i < image_regions.size(); ++i)
    {
      image_regions[i].bufferOffset = buffer_offset;
      image_regions[i].bufferRowLength = (uint32_t) tex_w;
      image_regions[i].bufferImageHeight = (uint32_t) tex_h;

      image_regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
      image_regions[i].imageSubresource.mipLevel = i;
      image_regions[i].imageSubresource.baseArrayLayer = 0;
      image_regions[i].imageSubresource.layerCount = 1;

      image_regions[i].imageOffset = {0, 0, 0};
      image_regions[i].imageExtent = 
      {
        (uint32_t) tex_w,
        (uint32_t) tex_h,
        1
      };

      buffer_offset += tex_w * tex_h * texture_image->Channels();      
      if (tex_w > 1) tex_w /= 2;
      if (tex_h > 1) tex_h /= 2;
    }

    auto buffer_lock = command_pool->OrderBufferLock();
    command_pool->BeginCommandBuffer(buffer_lock);
    command_pool->TransitionImageLayout(buffer_lock, texture_image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    texture_image->SetLayout(VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    command_pool->CopyBufferToImage(buffer_lock, texture_src_buffer, texture_image, image_regions);
    command_pool->TransitionImageLayout(buffer_lock, texture_image, texture_image->GetLayout(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    texture_image->SetLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    command_pool->EndCommandBuffer(buffer_lock);
    command_pool->ExecuteBuffer(buffer_lock);
    command_pool->ReleaseBufferLock(buffer_lock);

    texture_src_buffer.reset();
  }
}