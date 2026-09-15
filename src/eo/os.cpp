#include <eo/os.h>

#include <cstdlib>
#include <string_view>
#include <utility>

namespace eo::os {
  namespace {

    auto is_shell_special_var(char c) -> bool {
      switch (c) {
      case '*':
      case '#':
      case '$':
      case '@':
      case '!':
      case '?':
      case '-':
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        return true;
      default:
        return false;
      }
    }

    auto is_alphanumeric(char c) -> bool {
      return c == '_' || ('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z');
    }

    auto shell_name(std::string_view s) -> std::pair<std::string_view, std::size_t> {
      if (s.front() == '{') {
        if (s.size() > 2 && is_shell_special_var(s[1]) && s[2] == '}') {
          return {s.substr(1, 1), 3};
        }
        for (std::size_t i = 1; i < s.size(); ++i) {
          if (s[i] == '}') {
            if (i == 1) {
              return {{}, 2};
            }
            return {s.substr(1, i - 1), i + 1};
          }
        }
        return {{}, 1};
      }
      if (is_shell_special_var(s.front())) {
        return {s.substr(0, 1), 1};
      }

      std::size_t i = 0;
      while (i < s.size() && is_alphanumeric(s[i])) {
        ++i;
      }
      return {s.substr(0, i), i};
    }

  } // namespace

  auto expand_env(const std::string& s) -> std::string {
    std::string result;
    std::size_t start = 0;
    bool expanded = false;

    for (std::size_t j = 0; j < s.size(); ++j) {
      if (s[j] != '$' || j + 1 >= s.size()) {
        continue;
      }

      expanded = true;
      result.append(s, start, j - start);

      auto [name, width] = shell_name(std::string_view{s}.substr(j + 1));
      if (name.empty() && width > 0) {
        // Invalid expansion syntax is consumed, matching Go's os.Expand.
      } else if (name.empty()) {
        result += '$';
      } else if (auto* value = std::getenv(std::string{name}.c_str())) {
        result += value;
      }

      j += width;
      start = j + 1;
    }

    if (!expanded) {
      return s;
    }
    result.append(s, start, std::string::npos);
    return result;
  }

} // namespace eo::os
