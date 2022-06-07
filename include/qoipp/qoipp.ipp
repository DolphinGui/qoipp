#pragma once
#include <iterator>
#ifndef QOIPP_HEADER_INCLUDE
// this is to get the linter to work
#include "qoipp.hpp"
#endif

namespace qoipp {
namespace detail {
inline uint hash(qoipp::rgba val) {
  unsigned int hash = (val.r * 3 + val.g * 5 + val.b * 7 + val.a * 11) % 64;
  return (val.r * 3 + val.g * 5 + val.b * 7 + val.a * 11) % 64;
}

template <typename T> T getfrom(auto &&input) {
  static_assert(sizeof(*input) == 1, "iterator does not return char");
  std::array<uchar, sizeof(T)> result;
  if constexpr (std::is_pointer_v<decltype(input)>) {
    std::memcpy(&result, input, sizeof(T));
    std::advance(input, sizeof(T));
  } else {
    auto r = result.begin();
    for (uint i = 0; i < sizeof(T); ++i, ++r, ++input) {
      *r = *input;
    }
  }
  return std::bit_cast<T>(result);
}

template <typename T> void writeto(T &&input, auto &&output) {
  static_assert(sizeof(*input) == 1, "iterator does not return char");
  if constexpr (std::is_pointer_v<decltype(output)>) {
    std::memcpy(output, &input, sizeof(T));
    std::advance(output, sizeof(T));
  } else {
    for (uint i = 0; i < sizeof(T); ++i, ++output, ++input) {
      *output = *input;
    }
  }
}

} // namespace detail
using namespace detail;
template <typename It>
image_file<It>::image_file(It in)
    : header{.magic = getfrom<decltype(header.magic)>(in),
             .width = __builtin_bswap32(getfrom<uint32_t>(in)),
             .height = __builtin_bswap32(getfrom<uint32_t>(in)),
             .channels = getfrom<uint8_t>(in),
             .colorspace = getfrom<bool>(in)},
      start(in) {
  if (reinterpret_cast<const int &>(qoitag) !=
      reinterpret_cast<const int &>(header.magic))
    throw std::runtime_error("magic number does not match");
}
template <typename It>
template <typename P>
image_file<It>::image_file(It in, P pixels, header_t header)
    : header{header}, start(in) {
  if (reinterpret_cast<const int &>(qoitag) !=
      reinterpret_cast<const int &>(header.magic))
    throw std::runtime_error("magic number does not match");
}
template <typename It> pixel_read_iterator<It> image_file<It>::read_pixels() {
  return pixel_read_iterator(start, header.channels > 3);
}
template <typename It>
pixel_read_iterator<It>::pixel_read_iterator(It start, bool alpha)
    : prev{}, next(start), has_alpha(alpha) {
  read();
}
template <typename It> pixel pixel_read_iterator<It>::operator*() {
  return result;
}
template <typename It>
pixel_read_iterator<It> &pixel_read_iterator<It>::operator++() {
  if (current_run != 0) {
    --current_run;
#ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " run px\n";
#endif
    return *this;
  }
  prev = result;
  read();
  return *this;
}
template <typename It> pixel pixel_read_iterator<It>::read() {
  // less than ideal but it seperates business logic from implementation
  // ideally the implementation would be after the business logic
  // and ideally not in the same function but whatever I guess
  const auto ucast = [](unsigned int i) { return static_cast<uchar>(i); };
  const auto cache_result = [&]() { cache[hash(result.rgba)] = result; };
  const auto get_rgba = [&]() {
    result.rgba = getfrom<rgba>(++next);
    cache_result();
#ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " rgba px\n";
#endif
    return result;
  };
  const auto get_rgb = [&]() {
    result.rgb = getfrom<rgb>(++next);
    result.rgba.a = 255;
    cache_result();
#ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " rgb px\n";
#endif
    return result;
  };
  const auto get_diff = [&]() {
    auto diff = getfrom<diff_d>(next);
    auto r = ucast(prev.rgba.r + diff.dr - 2),
         g = ucast(prev.rgba.g + diff.dg - 2),
         b = ucast(prev.rgba.b + diff.db - 2);
    result.rgba =
        rgba{ucast(prev.rgba.r + diff.dr - 2), ucast(prev.rgba.g + diff.dg - 2),
             ucast(prev.rgba.b + diff.db - 2), prev.rgba.a};
    cache_result();
#ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " diff px\n";
#endif
    return result;
  };

  const auto get_luma = [&]() {
    auto luma = getfrom<luma_d>(next);
    auto r = ucast(prev.rgba.r + luma.dg - 32 + luma.r - 8);
    auto g = ucast(prev.rgba.g + luma.dg - 32);
    auto b = ucast(prev.rgba.b + luma.dg - 32 + luma.b - 8);
    result.rgba = rgba{r, g, b, prev.rgba.a};
    cache_result();
#ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " luma px\n";
#endif
    return result;
  };
  auto first_byte = ucast(*next);
  if (first_byte == rgba_tag) {
    return get_rgba();
  }
  if (first_byte == rgb_tag) {
    return get_rgb();
  }
  constexpr uchar tag_mask = 0b11000000;
  constexpr uchar val_mask = 0b00111111;
  if ((first_byte & tag_mask) == index_tag) {
    ++next;
    index_d data = bit_cast<index_d>(first_byte);
    auto index = data.index;
    result.rgba = cache[data.index].rgba;
#ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " index px\n";
#endif
    return result;
  }
  if ((first_byte & tag_mask) == diff_tag) {
    return get_diff();
  }
  if ((first_byte & tag_mask) == luma_tag) {
    return get_luma();
  }
  if ((first_byte & tag_mask) == run_tag) {
    auto data = getfrom<run_d>(next);
    current_run = data.length;
#ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " run start\n";
#endif
    return prev;
  }
  throw std::runtime_error("unexpected pixel type encountered");
}
} // namespace qoipp