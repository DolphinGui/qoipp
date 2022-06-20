#pragma once
#include "qoipp/utility.hpp"
#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <iterator>
#include <stdexcept>
#include <type_traits>

#define QOIPP_HEADER_INCLUDE

namespace qoipp {

template <typename T> struct image_file;
template <typename It> struct pixel_reader {
  pixel_reader(const pixel_reader &) = default;
  pixel_reader(pixel_reader &&) = default;

  pixel operator*() const noexcept;
  pixel_reader &operator++();

private:
  void read();
  pixel_reader(It start, bool alpha);
  friend class image_file<It>;
  std::array<pixel, 64> cache{};
  uint current_run = 0; // clang-format off
  #ifdef QOIPP_DEBUG
  uint current_pos = 0;
  #endif
  // clang-format on
  It next;
  pixel prev;
  pixel result;
  bool has_alpha;
};
template <typename It> struct pixel_writer {
public:
  pixel_writer(const pixel_writer &) = default;
  pixel_writer(pixel_writer &&) = default;
  pixel_writer(It start, bool alpha);
  ~pixel_writer();

  pixel_writer &operator++();
  pixel &operator*() { return current; }

private:
  void write();
  friend class image_file<It>;
  std::array<pixel, 64> cache{};
  uint current_run = 0; // clang-format off
  #ifdef QOIPP_DEBUG
  uint current_pos = 0;
  #endif
  // clang-format on
  It next;
  pixel prev;
  pixel current;
  bool has_alpha;
};
struct header_t {
  std::array<char, 4> magic;
  uint32_t width, height;
  uint8_t channels;
  bool colorspace;
  std::size_t size() const noexcept { return width * height; }

  static_assert(sizeof(int) == 4 * sizeof(char),
                "int is not able to hold the magic number");
};
template <typename It>
inline constexpr bool is_input =
    std::is_same_v<typename std::iterator_traits<It>::iterator_category,
                   std::input_iterator_tag>;
template <typename It> struct image_file {
public:
  header_t header;
  image_file(It);
  template <typename PixelIterator> image_file(It out, PixelIterator, header_t);
  pixel_reader<It> read();
  std::size_t size() const noexcept { return header.size(); }

private:
  It start;
};
} // namespace qoipp

template <typename It> struct std::iterator_traits<qoipp::pixel_reader<It>> {
  typedef void difference_type;
  typedef qoipp::pixel value_type;
  typedef void pointer;
  typedef qoipp::pixel reference;
  typedef std::input_iterator_tag iterator_category;
};

template <typename It> struct std::iterator_traits<qoipp::pixel_writer<It>> {
  typedef void difference_type;
  typedef qoipp::pixel value_type;
  typedef void pointer;
  typedef qoipp::pixel &reference;
  typedef std::output_iterator_tag iterator_category;
};

// actual implementation
#include "qoipp/qoipp.ipp"