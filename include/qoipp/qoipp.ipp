#pragma once
#include "qoipp/utility.hpp"
#include <array>
#include <iterator>
#include <type_traits>
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

inline uint32_t toBE(uint32_t in) noexcept {
// clang-format off
  #if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN
  return in;
  #elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__ 
  return __builtin_bswap32(in);
  #endif
  // clang-format on
}

template <typename T> void writeto(const T &input, auto &&output) {
  if constexpr (std::is_pointer_v<decltype(output)>) {
    std::memcpy(output, &input, sizeof(T));
    std::advance(output, sizeof(T));
  } else {
    auto *ptr = reinterpret_cast<const unsigned char *>(&input);
    for (uint i = 0; i < sizeof(T); ++i, ++output, ++ptr) {
      *output = *ptr;
    }
  }
}

} // namespace detail
using namespace detail;
template <typename It>
image_file<It>::image_file(It in)
    : header{.magic = getfrom<decltype(header.magic)>(in),
             .width = toBE(getfrom<uint32_t>(in)),
             .height = toBE(getfrom<uint32_t>(in)),
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
  writeto(header.magic, in);
  writeto(toBE(header.width), in);
  writeto(toBE(header.height), in);
  writeto(header.channels, in);
  writeto(header.colorspace, in);
  std::copy_n(pixels, size(), pixel_writer(in, header.channels > 3));
}
template <typename It> pixel_reader<It> image_file<It>::read() {
  return pixel_reader(start, header.channels > 3);
}
template <typename It>
pixel_reader<It>::pixel_reader(It start, bool alpha)
    : prev{}, next(start), has_alpha(alpha) {
  read();
}
template <typename It> pixel pixel_reader<It>::operator*() const noexcept {
  return result;
}
template <typename It> pixel_reader<It> &pixel_reader<It>::operator++() {
  if (current_run != 0) {
    --current_run; // clang-format off
    #ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " run px\n";
    #endif
                   // clang-format on
    return *this;
  }
  prev = result;
  read();
  return *this;
}
template <typename It> void pixel_reader<It>::read() {
  // less than ideal but it seperates business logic from implementation
  // ideally the implementation would be after the business logic
  // and ideally not in the same function but whatever I guess
  const auto ucast = [](unsigned int i) { return static_cast<uchar>(i); };
  const auto cache_result = [&]() { cache[hash(result.rgba)] = result; };
  const auto get_rgba = [&]() {
    result.rgba = getfrom<rgba>(++next);
    cache_result(); // this allows me to collapse the entire function in vscode.
                    // clang-format off
    #ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " rgba px\n";
    #endif
    // clang-format on
    return result;
  };
  const auto get_rgb = [&]() {
    result.rgb = getfrom<rgb>(++next);
    result.rgba.a = 255;
    cache_result(); // clang-format off
    #ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " rgb px\n";
    #endif
    // clang-format on
    return result;
  };
  const auto get_diff = [&](auto first_byte) {
    auto diff = bit_cast<diff_d>(first_byte);
    ++next;
    result.rgba = prev.rgba + diff;
    cache_result(); // clang-format off
    #ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " diff px\n";
    #endif
    // clang-format on
    return result;
  };

  const auto get_luma = [&]() {
    auto luma = getfrom<luma_d>(next);
    result.rgba = prev.rgba + luma;
    cache_result(); // clang-format off
    #ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " luma px\n";
    #endif
    // clang-format on
    return result;
  };
  auto first_byte = ucast(*next);
  if (first_byte == rgba_tag) {
    get_rgba();
    return;
  }
  if (first_byte == rgb_tag) {
    get_rgb();
    return;
  }
  constexpr uchar tag_mask = 0b11000000;
  constexpr uchar val_mask = 0b00111111;
  if ((first_byte & tag_mask) == index_tag) {
    ++next;
    index_d data = bit_cast<index_d>(first_byte);
    auto index = data.index;
    result.rgba = cache[data.index].rgba; // clang-format off
    #ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " index px\n";
    #endif
    // clang-format on
    return;
  }
  if ((first_byte & tag_mask) == diff_tag) {
    get_diff(first_byte);
    return;
  }
  if ((first_byte & tag_mask) == luma_tag) {
    get_luma();
    return;
  }
  if ((first_byte & tag_mask) == run_tag) {
    auto data = getfrom<run_d>(next);
    current_run = data.length; // clang-format off
    #ifdef QOIPP_DEBUG
    std::cout << current_pos++ << " run start\n";
    #endif
    // clang-format on
    return;
  }
  throw std::runtime_error("unexpected pixel type encountered");
}
template <typename It>
pixel_writer<It>::pixel_writer(It start, bool alpha)
    : next(start), prev{rgba{0, 0, 0, 255}}, has_alpha(alpha) {}
template <typename It> pixel_writer<It> &pixel_writer<It>::operator++() {
  write();
  prev = current;
  next++;
  return *this;
}
template <typename It> void pixel_writer<It>::write() { // clang-format off
  #ifdef QOIPP_DEBUG
  current_pos++;
  #endif
  // clang-format on
  static uint pos = 0;
  pos++;
  if (pixel_compare(prev, current, has_alpha)) {
    // might be vectorizable if there are a lot of repeated
    // pixels.
    ++current_run;
    return;
  }
  if (current_run != 0 || current_run == 62) {
    writeto(run_d(current_run - 1), next);
    current_run = 0;
    return;
  }
  if (auto index = hash(current.rgba); cache[index] == current.rgba) {
    writeto(index_d(index), next);
  }
  auto dr = current.rgb.r - prev.rgb.r;
  auto dg = current.rgb.g - prev.rgb.g;
  auto db = current.rgb.b - prev.rgb.b;
  const auto diff_check = [](int num) -> bool { return num >= -2 && num <= 1; };

  if (diff_check(dr) && diff_check(dg) && diff_check(db)) {
    writeto(diff_d(dr, dg, db), next);
    return;
  }
  const auto gluma_check = [](int g) -> bool { return g >= -32 && g <= 31; };
  const auto rbluma_check = [](int rb, int dg) -> bool {
    return rb - dg >= -8 && rb - dg <= 7;
  };
  if (gluma_check(dg) && rbluma_check(dr, dg) && rbluma_check(db, dg)) {
    writeto(luma_d(dr, dg, db), next);
    return;
  }
  if (has_alpha) {
    writeto(rgba_d(current.rgba), next);
    return;
  } else {
    writeto(rgb_d(current.rgb), next);
    return;
  }
};
template <typename It> pixel_writer<It>::~pixel_writer() {
  static constexpr std::array<uint8_t, 8> endtag = {0x00, 0x00, 0x00, 0x00,
                                                    0x00, 0x00, 0x00, 0x01};
  writeto(endtag, next);
}
} // namespace qoipp