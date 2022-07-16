#include <eo/os.h>
#include <wordexp.h>

namespace eo::os {

auto expand_env(const std::string& s) -> std::string {
  wordexp_t p;
  eo_defer([&]() { wordfree(&p); });
  wordexp(s.data(), &p, 0);

  std::string v{};
  for (auto i = 0; i < p.we_wordc; ++i) {
    if (i)
      v += " ";
    v += p.we_wordv[i];
  }
  return v;
}

} // namespace eo::os
