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
  std::this_thread::sleep_for(std::chrono::milliseconds{200});
  eo::os::Exit(0);
}
