// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
// os::Exit must end the process at once while another thread is blocked writing stdout to a pipe
// that nobody reads. That thread holds stdout's lock, so flushing stdout or std::cout on exit would
// wait for it forever.
#include <eo/fmt.h>
#include <eo/os.h>

#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <unistd.h>

int main() {
  int fds[2];
  if (pipe(fds) != 0 || dup2(fds[1], STDOUT_FILENO) < 0) {
    std::perror("pipe");
    return 1;
  }
  // The read end stays open and is never read, so the pipe fills and the writer blocks.
  std::thread writer([] {
    auto line = std::string(1000, 'x');
    for (;;) {
      fmt::Println(line);
    }
  });
  writer.detach();
  // Wait until the writer is blocked: it then holds stdout's lock for good.
  for (auto held = 0, i = 0; held < 20 && i < 500; ++i) {
    if (ftrylockfile(stdout) == 0) {
      funlockfile(stdout);
      held = 0;
    } else {
      ++held;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }
  eo::os::Exit(0);
}
