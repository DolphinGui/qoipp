#pragma once

#include <iostream>
#include <iterator>
#include <utility>
#include <vector>
#ifdef __has_include
// #pragma clang diagnostic push
// #pragma clang diagnostic ignored "-Wextra-tokens"
#if __has_include(<bit>)
#include <bit>
#endif
#endif
namespace qoipp {
#ifndef __cpp_lib_byte
enum struct byte : unsigned char {};
#else
using byte = std::byte;
#endif

#ifndef __cpp_lib_bit_cast
template <typename To> decltype(auto) bit_cast(auto &&from) {
  return *reinterpret_cast<To *>(from);
}
#else
template <typename To> decltype(auto) bit_cast(auto &&from) {
  return std::bit_cast<To>(std::forward<decltype(from)>(from));
}
#endif
struct rgb {
  unsigned char r, g, b;
};
inline bool operator==(rgb LHS, rgb RHS) noexcept {
  return LHS.r == RHS.r && LHS.g == RHS.g && LHS.b == RHS.b;
}
struct rgba {
  unsigned char r, g, b, a;
  inline operator rgb() { return {r, g, b}; }
};
inline bool operator==(rgba LHS, rgba RHS) noexcept {
  return LHS.r == RHS.r && LHS.g == RHS.g && LHS.b == RHS.b && LHS.a == RHS.a;
}
union pixel {
  rgb rgb;
  rgba rgba;
  operator struct rgb() { return rgb; }
  operator struct rgba() { return rgba; }
};
inline std::ostream &operator<<(std::ostream &out, rgba in) {
  out << "r: " << uint(in.r) << " g: " << uint(in.g) << " b: " << uint(in.b);
  return out;
}
namespace detail {
using uchar = unsigned char;
constexpr inline uchar rgb_tag = 0b11111110;
constexpr inline uchar rgba_tag = 0b11111111;
constexpr inline uchar index_tag = 0b00000000;
constexpr inline uchar diff_tag = 0b01000000;
constexpr inline uchar luma_tag = 0b10000000;
constexpr inline uchar run_tag = 0b11000000;
constexpr inline char qoitag[4] = {'q', 'o', 'i', 'f'};
/*note that the odd ordering of the members is very much
  intentional due to the binary layout of it. */
struct rgb_d {
  const char tag = rgb_tag;
  uchar r, g, b;
};
struct rgba_d {
  const char tag = rgba_tag;
  uchar r, g, b, a;
};
struct index_d {
  uchar index : 6;
  char tag : 2;
  index_d(uchar index) : tag(0b00), index(index) {}
  index_d(index_d &&) = default;
  index_d(const index_d &) = default;
};
struct diff_d {
  uchar db : 2, dg : 2, dr : 2;
  char tag : 2;
  diff_d(char dr, char dg, char db) : tag(0b01), dr(dr), dg(dg), db(db) {}
  diff_d() = default;
  diff_d(diff_d &&) = default;
  diff_d(const diff_d &) = default;
};
struct luma_d {
  uchar dg : 6;
  const char tag : 2;
  uchar b : 4, r : 4;
};
struct run_d {
  uchar length : 6;
  const char tag : 2;
  run_d(uchar index) : tag(0b00), length(index) {}
  run_d(run_d &&) = default;
  run_d(const run_d &) = default;
};
} // namespace detail
} // namespace qoipp