// 一个容量无限的阻塞 queue
muduo:MutexLock mutex;
muduo::Condtition cond(mutex);
std::queue<int> queue;

int dequeue()
{
    MutexLockGuard lock(mutex);
    while(queue.empty())
    {
        cond.wait(); // 这一步会原子地 unlock mutex 并进入等待，不会与enqueue 死锁
        // wait() 执行完毕时会自动重新加锁
    }
    assert(!queue.empty());
    int top = queue.front();
    queue.pop_front();
    return top;
}

void enqueue(int x)
{
    MutexLockGuard lock(mutex);
    queue.push_back(x);
    cond.notify();// 可以移出临界区外
}