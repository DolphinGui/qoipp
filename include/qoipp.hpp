#pragma once
#include "qoipp/utility.hpp"
#include <algorithm>
#include <array>
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
template <typename It> struct pixel_read_iterator {
  pixel_read_iterator(const pixel_read_iterator &) = default;
  pixel_read_iterator(pixel_read_iterator &&) = default;

  pixel operator*();
  pixel_read_iterator &operator++();

private:
  pixel read();
  pixel_read_iterator(It start, bool alpha);
  friend class image_file<It>;
  std::array<pixel, 64> cache{};
  uint current_run = 0;
#ifdef QOIPP_DEBUG
  uint current_pos = 0;
#endif
  It next;
  pixel prev;
  pixel result;
  bool has_alpha;
};
template <typename It> struct pixel_write_iterator {
  pixel_write_iterator(const pixel_write_iterator &) = default;
  pixel_write_iterator(pixel_write_iterator &&) = default;

  pixel_write_iterator &operator++();
  pixel_write_iterator &operator=(pixel p) {
    current = p;
    return *this;
  }

private:
  pixel write();
  pixel_write_iterator(It start, bool alpha);
  friend class image_file<It>;
  std::array<pixel, 64> cache{};
  uint current_run = 0;
#ifdef QOIPP_DEBUG
  uint current_pos = 0;
#endif
  It next;
  pixel prev;
  pixel current;
  bool has_alpha;
};
struct read {};
struct write {};
template <typename It> struct image_file {
public:
 struct header_t {
    std::array<char, 4> magic;
    uint32_t width, height;
    uint8_t channels;
    bool colorspace;

    static_assert(sizeof(int) == 4 * sizeof(char),
                  "int is not able to hold the magic number");
  } header;
  image_file(It);
  template<typename Pixels>
  image_file(It out, Pixels, header_t);
  pixel_read_iterator<It> read_pixels();
 

private:
  It start;
};
} // namespace qoipp

template <typename It>
struct std::iterator_traits<qoipp::pixel_read_iterator<It>> {
  typedef void difference_type;
  typedef qoipp::pixel value_type;
  typedef void pointer;
  typedef qoipp::pixel reference;
  typedef std::input_iterator_tag iterator_category;
};

// actual implementation
#include "qoipp/qoipp.ipp"