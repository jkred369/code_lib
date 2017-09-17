//基于pthread_mutex 实现一个互斥锁
class Mutex
{
    public:
        Mutex()
        {
            m_count = 0;
            m_threadID = 0;
            //pthread_mutexattr_t attr;
            //pthread_mutexattr_init(&attr);
            //pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
            //pthread_mutex_init(&m_mutex, &attr);
            pthread_mutex_init(&m_mutex,0);
        }

        ~Mutex()
        {
            pthread_mutex_destroy(&m_mutex);
        }

        void lock()
        {
            if(m_count && m_threadID == pthread_self())
            {
                ++m_count;
                return;
            }
            pthread_mutex_lock(&m_mutex);
            ++m_count;
            m_threadID = pthread_self();
        }

        void unlock()
        {
            if(m_count > 1)
            {
                m_count--;
                return;
            }
            --m_count;
            m_threadID = 0;
            pthread_mutex_unlock(&m_mutex);
        }

    private:
        pthread_mutex_t m_mutex;
        pthread_t m_threadID;
        int m_count;
};

// 实现一个互斥锁对象，可以在退出作用域是被析构(unlock)
class Locker
{
    public:
        Locker(Mutex& mutex)
        : _mutex(mutex)
        {
            _mutex.lock();
        }
        ~Locker()
        {
            _mutex.unlock();
        }
    private:
        Mutex& _mutex;
};

class ReverseLocker
{
    public:
        ReverseLocker( Mutex& mutex  )
        : _mutex( mutex  )
        {
          _mutex.unlock();  
        }

        ~ReverseLocker()
        {
            _mutex.lock();
        }
    private:
        Mutex& _mutex;
};
