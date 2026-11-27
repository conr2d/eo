// Copyright (c) 2026 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#pragma once

#include <eo/core.h>
#include <eo/errors.h>
#include <eo/io/fs.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

// Go's embed: the files that a //go:embed directive names, which the build compiles into the program
// (see eo_embed() in cmake/EoEmbed.cmake), and the FS type of Go 1.26's embed.go with its ReadFile
// method. FS.Open, FS.ReadDir, the fs.File and fs.DirEntry values they return, and the use of an FS
// as an fs.FS are not mapped, nor is the hash that Go records for each file.

namespace eo::embed {

// One embedded file as the build compiles it: its name, a slash-separated path relative to the
// directory of the package, and its contents.
struct file {
  std::string_view name;
  std::string_view data;
};

namespace detail {

  // Go's embed.split: the directory and the last element of a name, and whether it ends in a slash,
  // which marks a directory.
  inline auto split(std::string_view name) -> std::tuple<std::string_view, std::string_view, bool> {
    auto isDir = name.ends_with('/');
    if (isDir) {
      name.remove_suffix(1);
    }
    auto i = name.rfind('/');
    if (i == std::string_view::npos) {
      return {".", name, isDir};
    }
    return {name.substr(0, i), name.substr(i + 1), isDir};
  }

  // An entry of Go's sorted file list: a file, or a directory that holds one, named with a trailing
  // slash, as the Go toolchain lists them.
  struct entry {
    std::string name;
    std::string_view data;
  };

  // The file list that the Go toolchain builds for an FS: the files and every directory above them,
  // sorted by directory and then by element, so that the files of a directory are adjacent.
  inline auto list(std::span<const file> files) -> std::vector<entry> {
    auto entries = std::vector<entry>{};
    for (const auto& f : files) {
      entries.push_back({std::string{f.name}, f.data});
      for (auto i = f.name.rfind('/'); i != std::string_view::npos && i > 0; i = f.name.rfind('/', i - 1)) {
        entries.push_back({std::string{f.name.substr(0, i + 1)}, {}});
      }
    }
    std::ranges::sort(entries, [](const entry& a, const entry& b) {
      auto [adir, aelem, aisDir] = split(a.name);
      auto [bdir, belem, bisDir] = split(b.name);
      return adir < bdir || (adir == bdir && aelem < belem);
    });
    auto [last, end] = std::ranges::unique(entries, {}, &entry::name);
    entries.erase(last, end);
    return entries;
  }

} // namespace detail

// Go's embed.FS: a read-only collection of embedded files. The zero FS holds no files.
struct FS {
  static constexpr std::string_view _eo_name = "embed.FS";

  std::shared_ptr<const std::vector<detail::entry>> files{};

  FS() = default;

  // The FS of a //go:embed variable, from the files that the build compiled.
  explicit FS(std::span<const file> embedded)
    : files(std::make_shared<const std::vector<detail::entry>>(detail::list(embedded))) {}

  auto operator==(const FS&) const -> bool = default;

  // ReadFile reads and returns the content of the named file.
  static auto ReadFile(FS f, std::string name) -> std::tuple<slice<uint8_t>, error> {
    // Go's FS.Open and FS.lookup, for the name ReadFile opens.
    const detail::entry* found = nullptr;
    auto isDir = false;
    if (fs::ValidPath(name)) {
      if (name == ".") {
        isDir = true;
      } else if (f.files != nullptr) {
        auto [dir, elem, _] = detail::split(name);
        const auto& files = *f.files;
        auto it = std::ranges::partition_point(files, [&](const detail::entry& e) {
          auto [idir, ielem, iisDir] = detail::split(e.name);
          return !(idir > dir || (idir == dir && ielem >= elem));
        });
        if (it != files.end()) {
          auto trimmed = std::string_view{it->name};
          if (trimmed.ends_with('/')) {
            trimmed.remove_suffix(1);
          }
          if (trimmed == name) {
            found = &*it;
            isDir = it->name.ends_with('/');
          }
        }
      }
    }
    if (found == nullptr && !isDir) {
      return {nullptr, eo::New<fs::PathError>(fs::PathError{"open", name, fs::ErrNotExist})};
    }
    if (isDir) {
      return {nullptr, eo::New<fs::PathError>(fs::PathError{"read", name, errors::New("is a directory")})};
    }
    return {eo::bytes_from_string(std::string{found->data}), nullptr};
  }

  using _eo_methods = eo::methods<eo::method<&FS::ReadFile, "ReadFile">>;
};

// The value of a string variable with a //go:embed directive, which names one file.
inline auto string_of(std::span<const file> embedded) -> std::string {
  return std::string{embedded.front().data};
}

// The value of a []byte variable with a //go:embed directive, which names one file.
inline auto bytes_of(std::span<const file> embedded) -> slice<uint8_t> {
  return eo::bytes_from_string(std::string{embedded.front().data});
}

} // namespace eo::embed
