#include <eo/go.h>
#include <boost/exception/diagnostic_information.hpp>
#include <iostream>

using namespace eo;

extern func<> eo_main();

int main(int argc, char** argv) {
  try {
    boost::asio::co_spawn(runtime::executor, eo_main(), boost::asio::detached);
    runtime::executor.join();
  } catch (boost::exception& e) {
    std::cerr << boost::diagnostic_information(e) << std::endl;
  } catch (std::exception& e) {
    std::cerr << e.what() << std::endl;
  } catch (...) {
  }
}
