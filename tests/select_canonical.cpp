// Copyright (c) 2022 Jeeyong "conr2d" Um
// SPDX-License-Identifier: MIT
//
#include <eo/chan.h>
#include <eo/select.h>

#include <boost/asio/io_context.hpp>

#include <stdexcept>
#include <string>

namespace asio = boost::asio;

void check(bool condition, const char* message) {
  if (!condition) {
    throw std::runtime_error(message);
  }
}

void test_try_index_commits_receive() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};
  check(ch.raw().try_send(boost::system::error_code{}, 7), "test setup send should succeed");

  auto select = eo::Select{*ch};
  auto index = select.try_index();

  check(index == 0, "try_index should select a ready receive");
  check(!ch.raw().ready(), "try_index should commit the selected receive");
  check(select.recv<0>() == 7, "recv should return the already committed receive value");
}

void test_try_index_returns_default_sentinel_when_idle() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};

  auto select = eo::Select{*ch};

  check(select.try_index() == -1, "try_index should return -1 when no communication is ready");
}

void test_recv_returns_zero_value_for_closed_channel() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};
  ch.close();

  auto select = eo::Select{*ch};

  check(select.try_index() == 0, "closed receive should be immediately selectable");
  check(select.recv<0>() == 0, "closed receive should expose the zero value synchronously");
}

void test_selected_closed_send_panics_before_case_body() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};
  ch.close();

  auto select = eo::Select{ch << 1};

  try {
    select.try_index();
  } catch (const std::runtime_error& error) {
    check(std::string{error.what()} == "panic: send on closed channel", "closed send returned wrong error");
    return;
  }

  throw std::runtime_error("selected closed send did not panic during selection");
}

void test_recv_rejects_unselected_case() {
  asio::io_context io;
  eo::chan<int> first{io.get_executor(), 1};
  eo::chan<int> second{io.get_executor(), 1};
  check(first.raw().try_send(boost::system::error_code{}, 7), "test setup send should succeed");

  auto select = eo::Select{*first, *second};
  check(select.try_index() == 0, "first receive should be selected");

  try {
    select.recv<1>();
  } catch (const std::runtime_error&) {
    return;
  }

  throw std::runtime_error("recv should reject an accessor for an unselected case");
}

void test_recv_rejects_double_consume() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};
  check(ch.raw().try_send(boost::system::error_code{}, 7), "test setup send should succeed");

  auto select = eo::Select{*ch};
  check(select.try_index() == 0, "receive should be selected");
  check(select.recv<0>() == 7, "first recv should return the selected value");

  try {
    select.recv<0>();
  } catch (const std::runtime_error&) {
    return;
  }

  throw std::runtime_error("recv should reject consuming the selected result twice");
}

void test_select_rejects_reexecution_after_selection() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};
  check(ch.raw().try_send(boost::system::error_code{}, 7), "test setup send should succeed");

  auto select = eo::Select{*ch};
  check(select.try_index() == 0, "receive should be selected");

  try {
    select.try_index();
  } catch (const std::runtime_error&) {
    return;
  }

  throw std::runtime_error("Select should reject repeated execution after selecting a case");
}

void test_select_rejects_reexecution_after_default() {
  asio::io_context io;
  eo::chan<int> ch{io.get_executor(), 1};

  auto select = eo::Select{*ch};
  check(select.try_index() == -1, "idle select should return the default sentinel");

  try {
    select.try_index();
  } catch (const std::runtime_error&) {
    return;
  }

  throw std::runtime_error("Select should reject repeated execution after returning default");
}

int main() {
  test_try_index_commits_receive();
  test_try_index_returns_default_sentinel_when_idle();
  test_recv_returns_zero_value_for_closed_channel();
  test_selected_closed_send_panics_before_case_body();
  test_recv_rejects_unselected_case();
  test_recv_rejects_double_consume();
  test_select_rejects_reexecution_after_selection();
  test_select_rejects_reexecution_after_default();
}
