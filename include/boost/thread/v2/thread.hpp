// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
// (C) Copyright 2011 Vicente J. Botet Escriba

#ifndef BOOST_THREAD_V2_THREAD_HPP
#define BOOST_THREAD_V2_THREAD_HPP

#include <boost/thread/detail/config.hpp>
#ifdef BOOST_THREAD_USES_CHRONO
#include <boost/chrono/system_clocks.hpp>
#include <boost/chrono/ceil.hpp>
#endif
#include <boost/thread/condition_variable.hpp>
#include <boost/thread/lock_types.hpp>

namespace boost
{
  namespace this_thread
  {
    namespace no_interruption_point
    {
#ifdef BOOST_THREAD_USES_CHRONO

    template <class Duration>
    void sleep_until(const chrono::time_point<thread_detail::internal_clock_t, Duration>& t)
    {
      using namespace chrono;
      mutex mut;
      condition_variable cv;
      unique_lock<mutex> lk(mut);
      while (thread_detail::internal_clock_t::now() < t)
      {
        cv.wait_until(lk, t);
      }
    }
    template <class Clock, class Duration>
    void sleep_until(const chrono::time_point<Clock, Duration>& t)
    {
      using namespace chrono;
      Duration d = t - Clock::now();
      while (d > Duration::zero())
      {
        Duration d100 = (std::min)(d, Duration(milliseconds(100)));
        sleep_until(thread_detail::internal_clock_t::now() + ceil<nanoseconds>(d100));
        d = t - Clock::now();
      }
    }
#if defined BOOST_THREAD_SLEEP_FOR_IS_STEADY && !defined BOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC

    template <class Rep, class Period>
    void sleep_for(const chrono::duration<Rep, Period>& d)
    {
      using namespace chrono;
      if (d > duration<Rep, Period>::zero())
      {
          duration<long double> Max = nanoseconds::max BOOST_PREVENT_MACRO_SUBSTITUTION ();
          nanoseconds ns;
          if (d < Max)
          {
            ns = ceil<nanoseconds>(d);
          }
          else
          {
              ns = nanoseconds:: max BOOST_PREVENT_MACRO_SUBSTITUTION ();
          }
          sleep_for(ns);
      }
    }

    template <class Duration>
    inline BOOST_SYMBOL_VISIBLE
    void sleep_until(const chrono::time_point<chrono::steady_clock, Duration>& t)
    {
      using namespace chrono;
      sleep_for(t - steady_clock::now());
    }
#else
    template <class Rep, class Period>
    void sleep_for(const chrono::duration<Rep, Period>& d)
    {
      using namespace chrono;
      if (d > duration<Rep, Period>::zero())
      {
        sleep_until(steady_clock::now() + d);
      }
    }

#endif

#endif
    }
#ifdef BOOST_THREAD_USES_CHRONO

    template <class Duration>
    void sleep_until(const chrono::time_point<thread_detail::internal_clock_t, Duration>& t)
    {
      using namespace chrono;
      mutex mut;
      condition_variable cv;
      unique_lock<mutex> lk(mut);
      while (thread_detail::internal_clock_t::now() < t)
      {
        cv.wait_until(lk, t);
      }
    }
    template <class Clock, class Duration>
    void sleep_until(const chrono::time_point<Clock, Duration>& t)
    {
      using namespace chrono;
      Duration d = t - Clock::now();
      while (d > Duration::zero())
      {
        Duration d100 = (std::min)(d, Duration(milliseconds(100)));
        sleep_until(thread_detail::internal_clock_t::now() + ceil<nanoseconds>(d100));
        d = t - Clock::now();
      }
    }

#if defined BOOST_THREAD_SLEEP_FOR_IS_STEADY && !defined BOOST_THREAD_HAS_CONDATTR_SET_CLOCK_MONOTONIC

    template <class Rep, class Period>
    void sleep_for(const chrono::duration<Rep, Period>& d)
    {
      using namespace chrono;
      if (d > duration<Rep, Period>::zero())
      {
          const duration<long double> Max = (nanoseconds::max)();
          nanoseconds ns;
          if (d < Max)
          {
              ns = ceil<nanoseconds>(d);
          }
          else
          {
              // fixme: it is normal to sleep less than requested? Shouldn't we need to iterate until d has been elapsed?
              ns = (nanoseconds::max)();
          }
          sleep_for(ns);
      }
    }

    template <class Duration>
    inline BOOST_SYMBOL_VISIBLE
    void sleep_until(const chrono::time_point<chrono::steady_clock, Duration>& t)
    {
      using namespace chrono;
      sleep_for(t - steady_clock::now());
    }
#else
    template <class Rep, class Period>
    void sleep_for(const chrono::duration<Rep, Period>& d)
    {
      using namespace chrono;
      if (d > duration<Rep, Period>::zero())
      {
        sleep_until(steady_clock::now() + d);
      }
    }

#endif
#endif
  }
}


#endif
