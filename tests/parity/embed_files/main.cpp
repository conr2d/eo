#include <eo/embed.h>
#include <eo/errors.h>
#include <eo/fmt.h>
#include <eo/io/fs.h>

#include <cstdint>
#include <span>
#include <string>

using namespace eo;
using namespace std::string_literals;

void show(embed::FS f, std::string name);

auto _eo_embed_greeting() -> std::span<const eo::embed::file>;
auto _eo_embed_raw() -> std::span<const eo::embed::file>;
auto _eo_embed_files() -> std::span<const eo::embed::file>;

std::string greeting = embed::string_of(_eo_embed_greeting());

slice<uint8_t> raw = embed::bytes_of(_eo_embed_raw());

embed::FS files = embed::FS(_eo_embed_files());

embed::FS none{};

void show(embed::FS f, std::string name) {
  auto [b, err] = embed::FS::ReadFile(f, name);
  if (err != nullptr) {
    auto pe = New<ptr<fs::PathError>>();
    auto ok = errors::As(err, pe);
    fmt::Printf("%q: error %v (path error %v, not exist %v)\n"s, name, err, ok, errors::Is(err, fs::ErrNotExist));
    return;
  }
  fmt::Printf("%q: %d bytes %q (nil %v)\n"s, name, eo::len(b), b, b == nullptr);
}

func<> eo_main() {
  fmt::Printf("greeting %q\n"s, greeting);
  fmt::Printf("raw %d %q\n"s, eo::len(raw), raw);

  auto names = slice<std::string>::of({"hello.txt"s,
    "data/a.txt"s,
    "data/empty.txt"s,
    "data/sub/b.bin"s,
    "data"s,
    "data/sub"s,
    "."s,
    "data/"s,
    "missing.txt"s,
    "data/missing"s,
    "/hello.txt"s,
    "./hello.txt"s,
    "data//a.txt"s,
    "data/../hello.txt"s,
    ""s,
    "hello.txt/"s,
    "Hello.txt"s,
    "data/sub/b.bin/x"s,
    "\xff"s});
  for (auto [_eo_range_key, name] : eo::range(names)) {
    show(files, name);
  }
  show(none, "hello.txt"s);
  show(none, "."s);

  // A []byte variable and every ReadFile result are copies: changing one changes nothing else.
  eo::at(raw, 0) = 'B';
  auto [b, _] = embed::FS::ReadFile(files, "data/sub/b.bin"s);
  fmt::Printf("raw %q, file %q\n"s, eo::subslice(raw, 0, 5), eo::subslice(b, 0, 5));
  eo::at(b, 0) = 'X';
  auto c = std::get<0>(embed::FS::ReadFile(files, "data/sub/b.bin"s));
  fmt::Printf("again %q\n"s, eo::subslice(c, 0, 5));

  // An FS is a comparable struct: a copy is equal to it, and the zero FS is not.
  auto copied = files;
  fmt::Println(copied == files, none == files, none == embed::FS{});

  for (auto [_eo_range_key, p] :
    eo::range(slice<std::string>::of({"a"s, "a/b"s, "a/./b"s, "a/../b"s, "a//b"s, "/a"s, "a/"s, ".."s, "\xff"s}))) {
    fmt::Printf("ValidPath(%q) = %v\n"s, p, fs::ValidPath(p));
  }
  auto pe = New<fs::PathError>(fs::PathError{.Op = "read"s, .Path = "x"s, .Err = fs::ErrInvalid});
  auto _eo_call_0 = fs::PathError::Error(pe);
  auto _eo_call_1 = fs::PathError::Unwrap(pe);
  auto _eo_call_2 = fs::PathError::Timeout(pe);
  fmt::Println(_eo_call_0, _eo_call_1 == fs::ErrInvalid, _eo_call_2);
  fmt::Println(fs::ErrInvalid, fs::ErrPermission, fs::ErrExist, fs::ErrNotExist, fs::ErrClosed);
  co_return;
}
