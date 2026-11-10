// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
// os::Exit must end the process at once while another thread is blocked reading stdin. A blocked
// stdio read holds the stream's lock, so flushing every open FILE on exit would hang here.
#include <eo/os.h>

#include <chrono>
#include <cstdio>
#include <thread>
#include <unistd.h>

int main() {
  int fds[2];
  if (pipe(fds) != 0 || dup2(fds[0], STDIN_FILENO) < 0) {
    std::perror("pipe");
    return 1;
  }
  // The write end stays open and nothing is written, so the read below never returns.
  std::thread reader([] {
    char buf[64];
    std::fgets(buf, sizeof buf, stdin);
  });
  reader.detach();
  // Wait until the reader holds stdin's lock inside fgets. If it never shows up as held, for example
  // on a libc whose reads do not lock, the test still runs but proves less.
  for (auto i = 0; i < 500; ++i) {
    if (ftrylockfile(stdin) != 0) {
      break;
    }
    funlockfile(stdin);
    std::this_thread::sleep_for(std::chrono::milliseconds{10});
  }
  eo::os::Exit(0);
}
