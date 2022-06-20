#include "qoipp.hpp"
#include "qoipp/utility.hpp"
#include <iterator>
#include <vector>

namespace {
const static auto test_filename = "039.qoi";
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
  auto data = std::vector<unsigned char>(size);
  bytes_read = fread(data.data(), 1, size, f);
  fclose(f);
  return data;
}
auto find_errors(auto &a, auto &b) {
  std::vector<size_t> loc;
  loc.reserve(a.size() / 2);
  for (uint i = 0; i < a.size() && i < b.size(); ++i) {
    if (a[i] != b[i]) {
      loc.emplace_back(i);
    }
  }
  return loc;
}
} // namespace
int main() {
  std::vector<qoipp::rgb> ref_data;
  qoipp::header_t header;
  auto ref_file = creader();
  {
    auto ref_image = qoipp::image_file(ref_file.begin());
    ref_data.reserve(ref_image.size());
    std::copy_n(ref_image.read(), ref_image.size(),
                std::back_inserter(ref_data));
    header = ref_image.header;
  }
  std::vector<qoipp::rgb> data;
  {
    std::vector<unsigned char> fake_file;
    fake_file.reserve(header.size());
    {
      auto image = qoipp::image_file(std::back_inserter(fake_file),
                                     ref_data.begin(), header);
    }
    auto image = qoipp::image_file(fake_file.begin());
    data.reserve(image.size());
    // image.size()
    std::copy_n(image.read(), 50, std::back_inserter(data));
  }

  auto errors = find_errors(ref_data, data);
  if (errors.empty()) {
    std::cout << "success!\n";
  } else {
    for (auto e : errors) {
      std::cout << e << '\n';
    }
  }
}