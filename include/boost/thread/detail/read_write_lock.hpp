// Copyright (C)  2002-2003
// David Moore
//
// Original scoped_lock implementation
// Copyright (C) 2001
// William E. Kempf
//
// Permission to use, copy, modify, distribute and sell this software
// and its documentation for any purpose is hereby granted without fee,
// provided that the above copyright notice appear in all copies and
// that both that copyright notice and this permission notice appear
// in supporting documentation.  David Moore makes no representations
// about the suitability of this software for any purpose.
// It is provided "as is" without express or implied warranty.

#ifndef BOOST_READ_WRITE_LOCK_JDM031002_HPP
#define BOOST_READ_WRITE_LOCK_JDM031002_HPP

#include <boost/thread/detail/config.hpp>

#include <boost/utility.hpp>
#include <boost/thread/exceptions.hpp>
#include <boost/thread/detail/lock.hpp>

namespace boost {

struct xtime;

namespace read_write_lock_state {
    typedef enum
    {
        unlocked=0,
        read_locked=1,
        write_locked=2
    } read_write_lock_state;
} //namespace read_write_lock_state

namespace detail {
namespace thread {

template <typename Mutex>
class read_write_lock_ops : private noncopyable
{
private:
    read_write_lock_ops() { }
    ~read_write_lock_ops() { }

public:

    typedef Mutex mutex_type;

    static void write_lock(Mutex& m)
    {
        m.do_write_lock();
    }
    static void read_lock(Mutex& m)
    {
        m.do_read_lock();
    }
    static void write_unlock(Mutex& m)
    {
        m.do_write_unlock();
    }
    static void read_unlock(Mutex &m)
    {
        m.do_read_unlock();
    }
    static bool try_write_lock(Mutex &m)
    {
        return m.do_try_write_lock();
    }
    static bool try_read_lock(Mutex &m)
    {
        return m.do_try_read_lock();
    }

    static bool timed_write_lock(Mutex &m,const xtime &xt)
    {
        return m.do_timed_write_lock(xt);
    }
    static bool timed_read_lock(Mutex &m,const xtime &xt)
    {
        return m.do_timed_read_lock(xt);
    }

    static void demote(Mutex & m)
    {
        m.do_demote_to_read_lock();
    }
    static bool try_demote(Mutex & m)
    {
        return m.do_try_demote_to_read_lock();
    }
    static bool timed_demote(Mutex & m,const xtime &xt)
    {
        return m.do_timed_demote_to_read_lock(xt);
    }

    static bool try_promote(Mutex & m)
    {
        return m.do_try_promote_to_write_lock();
    }
    static bool timed_promote(Mutex & m,const xtime &xt)
    {
        return m.do_timed_promote_to_write_lock(xt);
    }
};

template <typename ReadWriteMutex>
class scoped_read_write_lock : private noncopyable
{
public:
    typedef ReadWriteMutex mutex_type;

    scoped_read_write_lock(
        ReadWriteMutex& mx,
        read_write_lock_state::read_write_lock_state initial_state)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (initial_state == read_write_lock_state::read_locked)
            read_lock();
        else if (initial_state == read_write_lock_state::write_locked)
            write_lock();
    }

    ~scoped_read_write_lock()
    {
        if (m_state != read_write_lock_state::unlocked)
            unlock();
    }

    void read_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        read_write_lock_ops<ReadWriteMutex>::read_lock(m_mutex);
        m_state = read_write_lock_state::read_locked;
    }

    void write_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        read_write_lock_ops<ReadWriteMutex>::write_lock(m_mutex);
        m_state = read_write_lock_state::write_locked;
    }

    void unlock()
    {
        if (m_state == read_write_lock_state::unlocked) throw lock_error();
        if (m_state == read_write_lock_state::read_locked)
            read_write_lock_ops<ReadWriteMutex>::read_unlock(m_mutex);
        else //(m_state == read_write_lock_state::write_locked)
            read_write_lock_ops<ReadWriteMutex>::write_unlock(m_mutex);

        m_state = read_write_lock_state::unlocked;
    }

    void demote(void)
    {
        if (m_state != read_write_lock_state::write_locked) throw lock_error();
        read_write_lock_ops<ReadWriteMutex>::demote(m_mutex);
        m_state = read_write_lock_state::read_locked;
    }

    void set_lock(read_write_lock_state::read_write_lock_state ls)
    {
        if (m_state != ls)
        {
            if (m_state == read_write_lock_state::unlocked)
            {
                if (ls == read_write_lock_state::read_locked)
                    read_lock();
                else //(ls == read_write_lock_state::write_locked)
                    write_lock();
            }
            else //(m_state == read_write_lock_state::read_locked || m_state == read_write_lock_state::write_locked)
            {
                if (ls == read_write_lock_state::read_locked)
                    demote();
                else if (ls == read_write_lock_state::write_locked)
                {
                    unlock();
                    write_lock();
                }
                else //(ls == read_write_lock_state::unlocked)
                    unlock();
            }
        }
    }
  
    bool locked() const
    {
        return m_state != read_write_lock_state::unlocked;
    }
  
    bool read_locked() const
    {
        return m_state == read_write_lock_state::read_locked;
    }
  
    bool write_locked() const
    {
        return m_state != read_write_lock_state::write_locked;
    }

    operator const void*() const
    {
        return (m_state != read_write_lock_state::unlocked) ? this : 0; 
    }

    read_write_lock_state::read_write_lock_state state() const
    {
        return m_state;
    }
    
private:
    ReadWriteMutex& m_mutex;
    read_write_lock_state::read_write_lock_state m_state;
};

template <typename ReadWriteMutex>
class scoped_read_lock : private noncopyable
{
public:
    typedef ReadWriteMutex mutex_type;

    scoped_read_lock(
        ReadWriteMutex& mx,
        lock_state::lock_state initial_state)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (initial_state)
            lock();
    }

    ~scoped_read_lock()
    {
        if (m_state != read_write_lock_state::unlocked)
            unlock();
    }

    void lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        read_write_lock_ops<ReadWriteMutex>::read_lock(m_mutex);
        m_state = read_write_lock_state::read_locked;
    }

    void unlock()
    {
        if (m_state != read_write_lock_state::read_locked) throw lock_error();
        read_write_lock_ops<ReadWriteMutex>::read_unlock(m_mutex);

        m_state = read_write_lock_state::unlocked;
    }
  
    bool locked() const
    {
        return m_state != read_write_lock_state::unlocked;
    }

    operator const void*() const
    {
        return (m_state != read_write_lock_state::unlocked) ? this : 0; 
    }
    
private:
    ReadWriteMutex& m_mutex;
    read_write_lock_state::read_write_lock_state m_state;
};

template <typename ReadWriteMutex>
class scoped_write_lock : private noncopyable
{
public:
    typedef ReadWriteMutex mutex_type;

    scoped_write_lock(
        ReadWriteMutex& mx,
        lock_state::lock_state initial_state)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (initial_state)
            lock();
    }

    ~scoped_write_lock()
    {
        if (m_state != read_write_lock_state::unlocked)
            unlock();
    }

    void lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        read_write_lock_ops<ReadWriteMutex>::write_lock(m_mutex);
        m_state = read_write_lock_state::write_locked;
    }

    void unlock()
    {
        if (m_state != read_write_lock_state::write_locked) throw lock_error();
        read_write_lock_ops<ReadWriteMutex>::write_unlock(m_mutex);

        m_state = read_write_lock_state::unlocked;
    }
  
    bool locked() const
    {
        return m_state != read_write_lock_state::unlocked;
    }

    operator const void*() const
    {
        return (m_state != read_write_lock_state::unlocked) ? this : 0; 
    }
    
private:
    ReadWriteMutex& m_mutex;
    read_write_lock_state::read_write_lock_state m_state;
};

template <typename TryReadWriteMutex>
class scoped_try_read_write_lock : private noncopyable
{
public:
    typedef TryReadWriteMutex mutex_type;
    
    scoped_try_read_write_lock(
        TryReadWriteMutex& mx,
        read_write_lock_state::read_write_lock_state initial_state, 
        blocking_mode::blocking_mode blocking)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (blocking)
        {
            if (initial_state == read_write_lock_state::read_locked)
                read_lock();
            else if (initial_state == read_write_lock_state::write_locked)
                write_lock();
        }
        else
        {
            if (initial_state == read_write_lock_state::read_locked)
                try_read_lock();
            else if (initial_state == read_write_lock_state::write_locked)
                try_write_lock();
        }
    }

    ~scoped_try_read_write_lock()
    {
        if (m_state != read_write_lock_state::unlocked)
            unlock();
    }

    void read_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        read_write_lock_ops<TryReadWriteMutex>::read_lock(m_mutex);
        m_state = read_write_lock_state::read_locked;
    }

    bool try_read_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        if(read_write_lock_ops<TryReadWriteMutex>::try_read_lock(m_mutex))
        {
            m_state = read_write_lock_state::read_locked;
            return true;
        }
        return false;
    }
   
    void write_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        read_write_lock_ops<TryReadWriteMutex>::write_lock(m_mutex);
        m_state = read_write_lock_state::write_locked;
    }

    bool try_write_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        if(read_write_lock_ops<TryReadWriteMutex>::try_write_lock(m_mutex))
        {
            m_state = read_write_lock_state::write_locked;
            return true;
        }
        return false;
    }

    void unlock()
    {
        if (m_state == read_write_lock_state::unlocked) throw lock_error();
        if (m_state == read_write_lock_state::read_locked)
            read_write_lock_ops<TryReadWriteMutex>::read_unlock(m_mutex);
        else //(m_state == read_write_lock_state::write_locked)
            read_write_lock_ops<TryReadWriteMutex>::write_unlock(m_mutex);

        m_state = read_write_lock_state::unlocked;
    }

    void demote(void)
    {
        if (m_state != read_write_lock_state::write_locked) throw lock_error();
        read_write_lock_ops<TryReadWriteMutex>::demote(m_mutex);
        m_state = read_write_lock_state::read_locked;
    }

    bool try_demote(void)
    {
        if (m_state != read_write_lock_state::write_locked) throw lock_error();
        return read_write_lock_ops<TryReadWriteMutex>::try_demote(m_mutex) ? (m_state = read_write_lock_state::read_locked, true) : false;
    }

    bool try_promote(void)
    {
        if (m_state != read_write_lock_state::read_locked) throw lock_error();
        return read_write_lock_ops<TryReadWriteMutex>::try_promote(m_mutex) ? (m_state = read_write_lock_state::write_locked, true) : false;
    }

    void set_lock(read_write_lock_state::read_write_lock_state ls)
    {
        if (m_state != ls)
        {
            if (m_state == read_write_lock_state::unlocked)
            {
                if (ls == read_write_lock_state::read_locked)
                    read_lock();
                else //(ls == read_write_lock_state::write_locked)
                    write_lock();
            }
            else //(m_state == read_write_lock_state::read_locked || m_state == read_write_lock_state::write_locked)
            {
                if (ls == read_write_lock_state::read_locked)
                    demote();
                else if (ls == read_write_lock_state::write_locked)
                {
                    if (!try_promote())
                    {
                        unlock();
                        write_lock();
                    }
                }
                else //(ls == read_write_lock_state::unlocked)
                    unlock();
            }
        }
    }

    bool try_set_lock(read_write_lock_state::read_write_lock_state ls)
    {
        if (m_state != ls)
        {
            if (m_state == read_write_lock_state::unlocked)
            {
                if (ls == read_write_lock_state::read_locked)
                    return try_read_lock();
                else // (ls == read_write_lock_state::write_locked)
                    return try_write_lock();
            }
            else //(m_state == read_write_lock_state::read_locked || m_state == read_write_lock_state::write_locked)
            {
                if (ls == read_write_lock_state::read_locked)
                    return try_demote();
                else if (ls == read_write_lock_state::write_locked)
                    return try_promote();
                else //(ls == read_write_lock_state::unlocked)
                    return unlock(), true;
            }
        }
        else //(m_state == ls) 
            return true;
    }
  
    bool locked() const
    {
        return m_state != read_write_lock_state::unlocked;
    }
  
    bool read_locked() const
    {
        return m_state == read_write_lock_state::read_locked;
    }
  
    bool write_locked() const
    {
        return m_state != read_write_lock_state::write_locked;
    }

    operator const void*() const
    {
        return (m_state != read_write_lock_state::unlocked) ? this : 0; 
    }

    read_write_lock_state::read_write_lock_state state() const
    {
        return m_state;
    }

private:
    TryReadWriteMutex& m_mutex;
    read_write_lock_state::read_write_lock_state m_state;
};

template <typename TryReadWriteMutex>
class scoped_try_read_lock : private noncopyable
{
public:
    typedef TryReadWriteMutex mutex_type;
    
    scoped_try_read_lock(
        TryReadWriteMutex& mx,
        lock_state::lock_state initial_state,
        blocking_mode::blocking_mode blocking)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (blocking)
        {
            if (initial_state)
                lock();
        }
        else
        {
            if (initial_state)
                try_lock();
        }
    }
    
    scoped_try_read_lock(
        TryReadWriteMutex& mx,
        blocking_mode::blocking_mode blocking)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (blocking)
            lock();
        else
            try_lock();
    }

    ~scoped_try_read_lock()
    {
        if (m_state != read_write_lock_state::unlocked)
            unlock();
    }

    void lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        read_write_lock_ops<TryReadWriteMutex>::read_lock(m_mutex);
        m_state = read_write_lock_state::read_locked;
    }

    bool try_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        if(read_write_lock_ops<TryReadWriteMutex>::try_read_lock(m_mutex))
        {
            m_state = read_write_lock_state::read_locked;
            return true;
        }
        return false;
    }

    void unlock()
    {
        if (m_state != read_write_lock_state::read_locked) throw lock_error();
        read_write_lock_ops<TryReadWriteMutex>::read_unlock(m_mutex);

        m_state = read_write_lock_state::unlocked;
    }
  
    bool locked() const
    {
        return m_state != read_write_lock_state::unlocked;
    }

    operator const void*() const
    {
        return (m_state != read_write_lock_state::unlocked) ? this : 0; 
    }

private:
    TryReadWriteMutex& m_mutex;
    read_write_lock_state::read_write_lock_state m_state;
};

template <typename TryReadWriteMutex>
class scoped_try_write_lock : private noncopyable
{
public:
    typedef TryReadWriteMutex mutex_type;
    
    scoped_try_write_lock(
        TryReadWriteMutex& mx,
        lock_state::lock_state initial_state,
        blocking_mode::blocking_mode blocking)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (blocking)
        {
            if (initial_state)
                lock();
        }
        else
        {
            if (initial_state)
                try_lock();
        }
    }
    
    scoped_try_write_lock(
        TryReadWriteMutex& mx,
        blocking_mode::blocking_mode blocking)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (blocking)
            lock();
        else
            try_lock();
    }

    ~scoped_try_write_lock()
    {
        if (m_state != read_write_lock_state::unlocked)
            unlock();
    }

    void lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        read_write_lock_ops<TryReadWriteMutex>::write_lock(m_mutex);
        m_state = read_write_lock_state::write_locked;
    }

    bool try_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        if(read_write_lock_ops<TryReadWriteMutex>::try_write_lock(m_mutex))
        {
            m_state = read_write_lock_state::write_locked;
            return true;
        }
        return false;
    }

    void unlock()
    {
        if (m_state != read_write_lock_state::write_locked) throw lock_error();
        read_write_lock_ops<TryReadWriteMutex>::write_unlock(m_mutex);

        m_state = read_write_lock_state::unlocked;
    }
  
    bool locked() const
    {
        return m_state != read_write_lock_state::unlocked;
    }

    operator const void*() const
    {
        return (m_state != read_write_lock_state::unlocked) ? this : 0; 
    }

private:
    TryReadWriteMutex& m_mutex;
    read_write_lock_state::read_write_lock_state m_state;
};

template <typename TimedReadWriteMutex>
class scoped_timed_read_write_lock : private noncopyable
{
public:
    typedef TimedReadWriteMutex mutex_type;

    scoped_timed_read_write_lock(
        TimedReadWriteMutex& mx,
        read_write_lock_state::read_write_lock_state initial_state, 
        blocking_mode::blocking_mode blocking)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (blocking)
        {
            if (initial_state == read_write_lock_state::read_locked)
                read_lock();
            else if (initial_state == read_write_lock_state::write_locked)
                write_lock();
        }
        else
        {
            if (initial_state == read_write_lock_state::read_locked)
                try_read_lock();
            else if (initial_state == read_write_lock_state::write_locked)
                try_write_lock();
        }
    }

    scoped_timed_read_write_lock(
        TimedReadWriteMutex& mx,
        read_write_lock_state::read_write_lock_state initial_state, 
        const xtime &xt)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (initial_state == read_write_lock_state::read_locked)
            timed_read_lock(xt);
        else if (initial_state == read_write_lock_state::write_locked)
            timed_write_lock(xt);
    }

    ~scoped_timed_read_write_lock()
    {
        if (m_state != read_write_lock_state::unlocked)
            unlock();
    }

    void read_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        read_write_lock_ops<TimedReadWriteMutex>::read_lock(m_mutex);
        m_state = read_write_lock_state::read_locked;
    }

    bool try_read_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        if(read_write_lock_ops<TimedReadWriteMutex>::try_read_lock(m_mutex))
        {
            m_state = read_write_lock_state::read_locked;
            return true;
        }
        return false;
    }

    bool timed_read_lock(const xtime &xt)
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        if(read_write_lock_ops<TimedReadWriteMutex>::timed_read_lock(m_mutex,xt))
        {
            m_state = read_write_lock_state::read_locked;
            return true;
        }
        return false;
    }

    void write_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        read_write_lock_ops<TimedReadWriteMutex>::write_lock(m_mutex);
        m_state = read_write_lock_state::write_locked;
    }

    bool try_write_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        if(read_write_lock_ops<TimedReadWriteMutex>::try_write_lock(m_mutex))
        {
            m_state = read_write_lock_state::write_locked;
            return true;
        }
        return false;
    }

    bool timed_write_lock(const xtime &xt)
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        if(read_write_lock_ops<TimedReadWriteMutex>::timed_write_lock(m_mutex,xt))
        {
            m_state = read_write_lock_state::write_locked;
            return true;
        }
        return false;
    }

    void unlock()
    {
        if (m_state == read_write_lock_state::unlocked) throw lock_error();
        if (m_state == read_write_lock_state::read_locked)
            read_write_lock_ops<TimedReadWriteMutex>::read_unlock(m_mutex);
        else //(m_state == read_write_lock_state::write_locked)
            read_write_lock_ops<TimedReadWriteMutex>::write_unlock(m_mutex);

        m_state = read_write_lock_state::unlocked;
    }

    void demote(void)
    {
        if (m_state != read_write_lock_state::write_locked) throw lock_error();
        read_write_lock_ops<TimedReadWriteMutex>::demote(m_mutex);
        m_state = read_write_lock_state::read_locked;
    }

    bool try_demote(void)
    {
        if (m_state != read_write_lock_state::write_locked) throw lock_error();
        return read_write_lock_ops<TimedReadWriteMutex>::try_demote(m_mutex) ? (m_state = read_write_lock_state::read_locked, true) : false;
    }

    bool timed_demote(const xtime &xt)
    {
        if (m_state != read_write_lock_state::write_locked) throw lock_error();
        return read_write_lock_ops<TimedReadWriteMutex>::timed_demote(m_mutex, xt) ? (m_state = read_write_lock_state::read_locked, true) : false;
    }

    bool try_promote(void)
    {
        if (m_state != read_write_lock_state::read_locked) throw lock_error();
        return read_write_lock_ops<TimedReadWriteMutex>::try_promote(m_mutex) ? (m_state = read_write_lock_state::write_locked, true) : false;
    }

    bool timed_promote(const xtime &xt)
    {
        if (m_state != read_write_lock_state::read_locked) throw lock_error();
        return read_write_lock_ops<TimedReadWriteMutex>::timed_promote(m_mutex, xt) ? (m_state = read_write_lock_state::write_locked, true) : false;
    }

    void set_lock(read_write_lock_state::read_write_lock_state ls)
    {
        if (m_state != ls)
        {
            if (m_state == read_write_lock_state::unlocked)
            {
                if (ls == read_write_lock_state::read_locked)
                    read_lock();
                else //(ls == read_write_lock_state::write_locked)
                    write_lock();
            }
            else //(m_state == read_write_lock_state::read_locked || m_state == read_write_lock_state::write_locked)
            {
                if (ls == read_write_lock_state::read_locked)
                    demote();
                else if (ls == read_write_lock_state::write_locked)
                {
                    if (!try_promote())
                    {
                        unlock();
                        write_lock();
                    }
                }
                else //(ls == read_write_lock_state::unlocked)
                    unlock();
            }
        }
    }

    bool try_set_lock(read_write_lock_state::read_write_lock_state ls)
    {
        if (m_state != ls)
        {
            if (m_state == read_write_lock_state::unlocked)
            {
                if (ls == read_write_lock_state::read_locked)
                    return try_read_lock();
                else // (ls == read_write_lock_state::write_locked)
                    return try_write_lock();
            }
            else //(m_state == read_write_lock_state::read_locked || m_state == read_write_lock_state::write_locked)
            {
                if (ls == read_write_lock_state::read_locked)
                    return try_demote();
                else if (ls == read_write_lock_state::write_locked)
                    return try_promote();
                else //(ls == read_write_lock_state::unlocked)
                    return unlock(), true;
            }
        }
        else //(m_state == ls) 
            return true;
    }

    bool timed_set_lock(read_write_lock_state::read_write_lock_state ls, const xtime &xt)
    {
        if (m_state != ls)
        {
            if (m_state == read_write_lock_state::unlocked)
            {
                if (ls == read_write_lock_state::read_locked)
                    return timed_read_lock(xt);
                else // (ls == read_write_lock_state::write_locked)
                    return timed_write_lock(xt);
            }
            else //(m_state == read_write_lock_state::read_locked || m_state == read_write_lock_state::write_locked)
            {
                if (ls == read_write_lock_state::read_locked)
                    return timed_demote(xt);
                else if (ls == read_write_lock_state::write_locked)
                    return timed_promote(xt);
                else //(ls == read_write_lock_state::unlocked)
                    return unlock(), true;
            }
        }
        else //(m_state == ls)
            return true;
    }
  
    bool locked() const
    {
        return m_state != read_write_lock_state::unlocked;
    }
  
    bool read_locked() const
    {
        return m_state == read_write_lock_state::read_locked;
    }
  
    bool write_locked() const
    {
        return m_state != read_write_lock_state::write_locked;
    }

    operator const void*() const
    {
        return (m_state != read_write_lock_state::unlocked) ? this : 0; 
    }

    read_write_lock_state::read_write_lock_state state() const
    {
        return m_state;
    }

private:
    TimedReadWriteMutex& m_mutex;
    read_write_lock_state::read_write_lock_state m_state;
};

template <typename TimedReadWriteMutex>
class scoped_timed_read_lock : private noncopyable
{
public:
    typedef TimedReadWriteMutex mutex_type;

    scoped_timed_read_lock(
        TimedReadWriteMutex& mx,
        lock_state::lock_state initial_state, 
        blocking_mode::blocking_mode blocking)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (blocking)
        {
            if (initial_state)
                lock();
        }
        else
        {
            if (initial_state)
                try_lock();
        }
    }

    scoped_timed_read_lock(
        TimedReadWriteMutex& mx,
        blocking_mode::blocking_mode blocking)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (blocking)
            lock();
        else
            try_lock();
    }

    scoped_timed_read_lock(
        TimedReadWriteMutex& mx,
        const xtime &xt)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        timed_lock(xt);
    }

    ~scoped_timed_read_lock()
    {
        if (m_state != read_write_lock_state::unlocked)
            unlock();
    }

    void lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        read_write_lock_ops<TimedReadWriteMutex>::read_lock(m_mutex);
        m_state = read_write_lock_state::read_locked;
    }

    bool try_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        if(read_write_lock_ops<TimedReadWriteMutex>::try_read_lock(m_mutex))
        {
            m_state = read_write_lock_state::read_locked;
            return true;
        }
        return false;
    }

    bool timed_lock(const xtime &xt)
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        if(read_write_lock_ops<TimedReadWriteMutex>::timed_read_lock(m_mutex,xt))
        {
            m_state = read_write_lock_state::read_locked;
            return true;
        }
        return false;
    }

    void unlock()
    {
        if (m_state != read_write_lock_state::read_locked) throw lock_error();
        read_write_lock_ops<TimedReadWriteMutex>::read_unlock(m_mutex);

        m_state = read_write_lock_state::unlocked;
    }
  
    bool locked() const
    {
        return m_state != read_write_lock_state::unlocked;
    }

    operator const void*() const
    {
        return (m_state != read_write_lock_state::unlocked) ? this : 0; 
    }

    read_write_lock_state::read_write_lock_state state() const
    {
        return m_state;
    }

private:
    TimedReadWriteMutex& m_mutex;
    read_write_lock_state::read_write_lock_state m_state;
};

template <typename TimedReadWriteMutex>
class scoped_timed_write_lock : private noncopyable
{
public:
    typedef TimedReadWriteMutex mutex_type;

    scoped_timed_write_lock(
        TimedReadWriteMutex& mx,
        lock_state::lock_state initial_state, 
        blocking_mode::blocking_mode blocking)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (blocking)
        {
            if (initial_state)
                lock();
        }
        else
        {
            if (initial_state)
                try_lock();
        }
    }

    scoped_timed_write_lock(
        TimedReadWriteMutex& mx,
        blocking_mode::blocking_mode blocking)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        if (blocking)
            lock();
        else
            try_lock();
    }

    scoped_timed_write_lock(
        TimedReadWriteMutex& mx,
        const xtime &xt)
        : m_mutex(mx), m_state(read_write_lock_state::unlocked)
    {
        timed_lock(xt);
    }

    ~scoped_timed_write_lock()
    {
        if (m_state != read_write_lock_state::unlocked)
            unlock();
    }

    void lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        read_write_lock_ops<TimedReadWriteMutex>::write_lock(m_mutex);
        m_state = read_write_lock_state::write_locked;
    }

    bool try_lock()
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        if(read_write_lock_ops<TimedReadWriteMutex>::try_write_lock(m_mutex))
        {
            m_state = read_write_lock_state::write_locked;
            return true;
        }
        return false;
    }

    bool timed_lock(const xtime &xt)
    {
        if (m_state != read_write_lock_state::unlocked) throw lock_error();
        if(read_write_lock_ops<TimedReadWriteMutex>::timed_write_lock(m_mutex,xt))
        {
            m_state = read_write_lock_state::write_locked;
            return true;
        }
        return false;
    }

    void unlock()
    {
        if (m_state != read_write_lock_state::write_locked) throw lock_error();
        read_write_lock_ops<TimedReadWriteMutex>::write_unlock(m_mutex);

        m_state = read_write_lock_state::unlocked;
    }
  
    bool locked() const
    {
        return m_state != read_write_lock_state::unlocked;
    }

    operator const void*() const
    {
        return (m_state != read_write_lock_state::unlocked) ? this : 0; 
    }

    read_write_lock_state::read_write_lock_state state() const
    {
        return m_state;
    }

private:
    TimedReadWriteMutex& m_mutex;
    read_write_lock_state::read_write_lock_state m_state;
};

} // namespace thread
} // namespace detail
} // namespace boost

#endif

// Change Log:
//  10 Mar 02 
//      Original version.
//   4 May 04 GlassfordM 
//      Implement lock promotion and demotion (add member functions demote(),
//         try_demote(), timed_demote(), try_promote(), timed_promote(); note 
//         that there is intentionally no promote() member function).
//      Add set_lock() member function.
//      Change try lock & timed lock constructor parameters for consistency.
//      Rename to improve consistency and eliminate abbreviations:
//          Use "read" and "write" instead of "shared" and "exclusive".
//          Change "rd" to "read", "wr" to "write", "rw" to "read_write".
//      Add mutex_type typdef.