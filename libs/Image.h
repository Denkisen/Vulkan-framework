 
#ifndef __CPU_NW_LIBS_IMAGE_H
#define __CPU_NW_LIBS_IMAGE_H

#include <iostream>
#include <optional>
#include <vector>
#include <tuple>


class Image
{
private:
  int bpp = 0;
  int height = 0;
  int width = 0;
  std::vector<uint8_t> canva;
public:
  Image() = default;
  Image(const Image &img);
  Image(const int w, const int h, const int bpp);
  Image(std::string file_path);
  Image& operator=(const Image &img);
  uint8_t& operator[](const size_t index) { return canva[index]; }
  std::vector<uint8_t>& Canvas() { return canva; }
  void GetCopy(std::vector<uint8_t> &data) { data = canva; }
  std::tuple<int, int, int> Dimensions() { return std::make_tuple(height, width, bpp); }
  int Length() { return canva.size(); }
  int Width() { return width; }
  int Height() { return height; }
  int BitsPerPixel() { return bpp; }
  void Open(std::string file_path);
  void Close();
  void Save(std::string file_path);
  void Resize(int w, int h);
  Image CutRectangle(int x, int y, int w, int h);

  ~Image() = default;
};

#endif