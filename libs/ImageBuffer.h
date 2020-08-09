#ifndef __CPU_NW_LIBS_IMAGE_H
#define __CPU_NW_LIBS_IMAGE_H

#include <iostream>
#include <vector>
#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

class ImageBuffer
{
private:
  cv::Mat image;
  void SetChannels(const int channals);
public:
  ImageBuffer() = default;
  ImageBuffer(const ImageBuffer &img);
  explicit ImageBuffer(std::string file_path, const int channals = 4);
  explicit ImageBuffer(const size_t w, const size_t h, const int channals = 4);
  ImageBuffer& operator=(const ImageBuffer &img);
  uint8_t& operator[](const size_t index);

  void Load(std::string file_path, const int channals = 4);
  void Save(std::string file_path, const int channals = 4);

  std::vector<uint8_t> Canvas() const;

  int Width() const { return image.rows; }
  int Height() const { return image.cols; }
  int Channels() const { return image.channels(); }

  void Resize(int w, int h);
  ImageBuffer SubImage(int x, int y, int w, int h);

  ~ImageBuffer() = default;
};

#endif