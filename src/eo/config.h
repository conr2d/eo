#pragma once

#if defined(__clang__)
#  if __has_include(<coroutine>)
#    if !defined(BOOST_ASIO_HAS_STD_COROUTINE)
#      define BOOST_ASIO_HAS_STD_COROUTINE 1
#    endif
#    if !defined(BOOST_ASIO_HAS_CO_AWAIT)
#      define BOOST_ASIO_HAS_CO_AWAIT 1
#    endif
#  endif
#endif
