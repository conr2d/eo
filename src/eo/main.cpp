// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/go.h>
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>

using namespace eo;

extern func<> eo_main();

int main(int argc, char** argv) {
  try {
    std::exception_ptr _eptr{};
    boost::asio::co_spawn(runtime::executor, eo_main(), [&](std::exception_ptr eptr) {
      _eptr = eptr;
    });
    runtime::executor.join();
    if (_eptr) {
      std::rethrow_exception(_eptr);
    }
  } catch (const boost::exception& e) {
    std::cerr << boost::diagnostic_information(e) << std::endl;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  } catch (...) {
  }
}
