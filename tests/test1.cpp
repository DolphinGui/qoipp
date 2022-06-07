#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <streambuf>
#define QOI_IMPLEMENTATION
#include "qoipp/qoi_debugging.h"
#define QOIPP_FORCE_RGB
#include "qoipp.hpp"

using uchar = unsigned char;

namespace {
void print(unsigned int r, unsigned int g, unsigned int b) {
  std::cout << "pixel: r: " << r << " g: " << g << " b:" << b << '\n';
}
const static auto test_filename = "039.qoi";
// reading a binary file in c++ is actually suprisingly hard
// it'd require overriding filebuf, which isn't the point of
// this test. This code is just copied from the qoi.h reference
// codec.
auto creader() {

  FILE *f = fopen(test_filename, "rb");
  int size, bytes_read;

  if (!f) {
    throw std::runtime_error("failed to open file");
  }

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  if (size <= 0) {
    fclose(f);
    throw std::runtime_error("file size is weird");
  }
  fseek(f, 0, SEEK_SET);
  auto data = std::vector<uchar>(size);
  bytes_read = fread(data.data(), 1, size, f);
  fclose(f);
  return data;
}

auto cqoi() {
  qoi_desc desc;
  auto pixels =
      reinterpret_cast<qoipp::rgb *>(qoi_read(test_filename, &desc, 0));
  std::vector<qoipp::rgb> results;
  size_t length = desc.height * desc.width;
  results.reserve(length);
  std::copy_n(pixels, length, std::back_insert_iterator(results));
  free(pixels);
  return results;
}
auto coipp() {
  std::basic_ifstream<char> file(test_filename,
                                 std::ios::binary | std::ios::ate);
  auto data = creader();
  auto image = qoipp::image_file(data.begin());
  std::vector<qoipp::rgb> results;
  auto length = image.header.height * image.header.width;
  results.reserve(length);
  std::copy_n(image.read_pixels(), length, std::back_inserter(results));
  return results;
}
auto find_errors(auto &a, auto &b) {
  std::vector<size_t> loc;
  loc.reserve(a.size() / 2);
  for (uint i = 0; i < a.size(); ++i) {
    if (a[i] != b[i]) {
      loc.emplace_back(i);
    }
  }
  return loc;
}
} // namespace

int main() {
  auto reference = cqoi();
  auto results = coipp();
  std::cout << "length: " << reference.size() << '\n';
  auto errors = find_errors(reference, results);
  if (errors.size() == 0) {
    std::cout << "success!\n";
  } else {
    for (auto err : errors) {
      std::cout << "erorr at pixel " << err << '\n';
    }
  }
}