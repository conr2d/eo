#include <eo/fmt.h>

#include <string>

using namespace eo;

func<> eo_main() {
  auto messages = make_chan<std::string>();
  auto signals = make_chan<bool>();

  switch (auto select = Select{*messages}; select.try_index()) {
  case 0: {
    auto msg = select.recv<0>();
    fmt::Println("received message", msg);
    break;
  }
  default: {
    fmt::Println("no message received");
    break;
  }
  }

  auto msg = std::string{"hi"};
  switch (auto select = Select{messages << msg}; select.try_index()) {
  case 0: {
    fmt::Println("sent message", msg);
    break;
  }
  default: {
    fmt::Println("no message sent");
    break;
  }
  }

  switch (auto select = Select{*messages, *signals}; select.try_index()) {
  case 0: {
    auto msg = select.recv<0>();
    fmt::Println("received message", msg);
    break;
  }
  case 1: {
    auto sig = select.recv<1>();
    fmt::Println("received signal", sig);
    break;
  }
  default: {
    fmt::Println("no activity");
    break;
  }
  }
  co_return;
}
