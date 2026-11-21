#include "github.com/conr2d/eo/tests/parity/package_imports/package.h"

namespace github_com_conr2d_eo_tests_parity_package_imports {

namespace util = ::github_com_conr2d_eo_tests_parity_package_imports_u2_util;

void Holder::Scale(ptr<Holder> _eo_receiver, int64_t k) {
  return a::Point::Scale(ptr(_eo_receiver, &_eo_receiver->Point), k);
}

auto Holder::Sum(Holder _eo_receiver) -> int64_t {
  return a::Point::Sum(_eo_receiver.Point);
}

// Record names the package trace while Holder has a field trace.
void Holder::Record(Holder h) {
  _eo_package::trace::Add("record "s + h.trace);
}

auto Square::Area(Square s) -> int64_t {
  return s.Side * s.Side;
}

auto box(std::string label) -> util::Box {
  return util::Box{.Label = label};
}

auto other() -> std::string {
  return util::Name();
}

} // namespace github_com_conr2d_eo_tests_parity_package_imports
