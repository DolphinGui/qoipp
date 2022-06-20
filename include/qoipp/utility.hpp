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
using uint = unsigned int;

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
  inline pixel &operator=(struct rgb p) noexcept {
    rgb = p;
    return *this;
  }
  inline pixel &operator=(struct rgba p) noexcept {
    rgba = p;
    return *this;
  }
};
// compares a and b as rgba if has_alpha is true, or as rgb otherwise.
inline bool pixel_compare(pixel a, pixel b, bool has_alpha) {
  // tested it, and this is a little premature. GCC and clang
  // compiles a naive version that branches which performs about
  // the same as this.
  a.rgba.a *= has_alpha;
  b.rgba.b *= has_alpha;
  return a.rgba == b.rgba;
}
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
  rgb px;
  rgb_d(rgb px) : px(px) {}
  rgb_d(rgb_d &&px) = default;
  rgb_d(const rgb_d &px) = default;
};
struct rgba_d {
  const char tag = rgba_tag;
  rgba px;
  rgba_d(rgba px) : px(px) {}
  rgba_d(rgba_d &&px) = default;
  rgba_d(const rgba_d &px) = default;
};
struct index_d {
  uchar index : 6;
  char tag : 2;
  index_d(uchar index) : tag(0b00), index(index) {}
  index_d(index_d &&) = default;
  index_d(const index_d &) = default;
};
class diff_d {
  uchar db : 2, dg : 2, dr : 2;
  char tag : 2;

public:
  // I normally dislike accessor methods but since all of the structs
  // represent binary representation to actually access them properly
  // requires this
  inline uchar getdr() const noexcept { return dr - 2; }
  inline uchar getdg() const noexcept { return dg - 2; }
  inline uchar getdb() const noexcept { return db - 2; }
  diff_d(char dr, char dg, char db)
      : tag(0b01), dr(dr + 2), dg(dg + 2), db(db + 2) {}
  diff_d() = default;
  diff_d(diff_d &&) = default;
  diff_d(const diff_d &) = default;
};
template <typename Diff> rgba operator+(rgba p, Diff d) noexcept {
  return rgba{static_cast<unsigned char>(p.r + d.getdr()),
              static_cast<unsigned char>(p.g + d.getdg()),
              static_cast<unsigned char>(p.b + d.getdb()), p.a};
}
template <typename Diff> rgba operator+(Diff d, rgba p) noexcept {
  return p + d;
}
template <typename Diff> rgb operator+(rgb p, Diff d) noexcept {
  return rgb{static_cast<unsigned char>(p.r + d.getdr()),
             static_cast<unsigned char>(p.g + d.getdg()),
             static_cast<unsigned char>(p.b + d.getdb())};
}
template <typename Diff> rgb operator+(Diff d, rgb p) noexcept { return p + d; }
class luma_d {
  uchar dg : 6;
  const unsigned char tag : 2;
  uchar db : 4, dr : 4;

public:
  inline uchar getdr() const noexcept { return dg - 32 + dr - 8; }
  inline uchar getdg() const noexcept { return dg - 32; }
  inline uchar getdb() const noexcept { return dg - 32 + db - 8; }
  luma_d(uchar dr, uchar dg, uchar db)
      : dg(dg + 32), dr(dr - dg + 8), db(db - dg + 8), tag(0b10) {}
  luma_d(luma_d &&) = default;
  luma_d(const luma_d &) = default;
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