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



// CountDownLatch 倒数计数器
class CountDownLatch:boost::noncopyable
{
    public:
    explicit CountDownLatch(int count); // 倒数几次
    void wait(); // 等待计数变为0
    void countDown(); //计数减一

    private:
    mutable MutexLock mutex_;
    Condition condition_;
    int count_;
};

void CountDownLatch::wait()
{
    MutexLockGuard lock(mutex_);
    while(count_ > 0)
        condition_.wait();
}

void CountDownLatch::countDown()
{
    MutexLockGuard lock(mutex_);
    --count_;
    if(count_ == 0)
        condition_.notifyAll(); // 这里必须使用notifyAll ,不能使用notify
}



// 线程安全的队列，queue
muduo::MutexLock mutex;
muduo::Condition cond(mutex);
std::deque<int> queue;

int deque()
{
    MutexLockGuard lock(mutex);
    while(queue.empty()) // 必须用循环，避免惊鸿效应，不能用if
    {
        cond.wait();
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
    cond.notify(); // 可以移出临界区，不一定要在mutex已上锁的情况下调用signal
}

//条件变量是非常底层的同步原语，很少直接使用，一般都是用它来实现高层的同步措施，如BlockingQueue<T>或CountDownLatch


//实现一个RingBuffer, 环形缓冲
#define BUFFER_SIZE 16
typedef struct _RingBuffer
{
    int buffer[BUFFER_SIZE];
    pthread_mutex_t mutex;
    int readpos,writepos;
    pthread_cond_t empty;
    pthread_cond_t full;
} RingBuffer;

void init(RingBuffer* b)
{
    pthread_mutex_init(&b->mutex,nullptr);
    pthread_cond_init(&b->empty,nullptr);
    pthread_cond_init(&b->full,nullptr);
    b->readpos = 0;
    b->writepos = 0;
}

void put(RingBuffer* b,int data)
{
    pthread_mutex_lock(&b->mutex);
    while((b->writepos+1)%BUFFER_SIZE == b->readpos)
    {
        pthread_cond_wait(&b->full,&b->mutex);
    }
    b->buffer[b->writepos] = data;
    b->writepos++;
    if(b->writepos >= BUFFER_SIZE)
    {
        b->writepos = 0;
    }
    pthread_cond_signal(&b->empty);
    pthread_mutex_unlock(&b->mutex);
}

int get(RingBuffer* b)
{
    int data = 0;
    pthread_mutex_lock(&b->mutex);
    while(b->writepos == b->readpos)
    {
        pthread_cond_wait(&b->empty,&b->mute);
    }
    data = b->buffer[b->readpos];
    b->readpos++;
    if(b->readpos >= BUFFER_SIZE)
    {
        b->readpos = 0;
    }
    pthread_cond_signal(&b->full);
    pthread_mutex_unlock(&b->mutex);
    return data;
}

#define OVER (-1)
RingBuffer g_buffer;

void* producer(void* data)
{
    for(int n = 0;n<10000;++n)
    {
        put(&g_buffer,n);
    }
    put(&g_buffer,OVER);
    return nullptr;
}

void* consumer(void* data)
{
    int d = 0;
    while(1)
    {
        d = get(&g_buffer);
        if(d == OVER)
            break;
        printf("%d\n",d);
    }
    return nullptr;
}

int main(int argc,char* argv[])
{
    pthread_t th_1,th_2;
    void* retVal;
    init(&g_buffer);
    pthread_create(&th_1,nullptr,producer,nullptr);
    pthread_create(&th_2,nullptr,consumer,nullptr);

    pthread_join(th_1,&retVal);
    pthread_join(th_2,&retVal);

    return 0;
}



//借用shared_ptr 实现 copy-on-write,写时复制，cow
// 参考陈硕《Linux多线程服务端编程》P70
// shared_ptr是引用计数型智能指针，如果当前只有一个观察者，那么引用计数的值为1
// 对于write端，如果发现引用计数为1，这时可以安全的修改共享对象，不必担心有人正在读它
// 对于read端，在读之前把引用计数加1，读完之后减1，这样保证在读的期间，其引用计数大于1，可以阻止并发写
// 比较难的是，对于write端，如果发现引用计数大于1，该如何处理？ sleep()一小段时间肯定是错的

typedef std::vector<Foo> FooList;
typedef boost::shared_ptr<FooList> FooListPtr;
MutexLock mutex;
FooListPtr g_foos;

//读
void traverse()
{
    FooListPtr foos;
    {
        MutexLockGuard lock(mutex);
        foos = g_foos;
        assert(!g_foos.unique());
    }
    for(const auot& e:*foos)
    {
        e.doit();
    }
}

// 写
void post(const Foo& f)
{
    printf("post\n");
    MutexLockGuard lock(mutex);
    if(!g_foos.unique())
    {
        g_foos.reset(new FooList(*g_foos));
        printf("copy the whole list\n");
    }
    assert(g_foos.unique());
    g_foos->push_back(f);
}









// 以下使用atomic 方法是线程安全的
long g_data;
std::atomic<bool> g_readyFlag(false);

void provider()
{
	std::cout << "<return>" << std::endl;
	std::cin.get();

	g_data = 42;
	g_readyFlag.store(true);
    //store 会对影响所及的内存区执行一个所谓release操作，确保此前所有内存操作不论是否为atomic,在store发挥效用之前都变为（可被其他线程看见）
}

void consumer()
{
    //load 会对影响所及的内存区执行一个所谓acquire操作，确保随后所有内存操作不论是否为atomic，在load之后都变为“可被其他线程看见”
	while (!g_readyFlag.load())
	{
		std::cout.put('.').flush();
		std::this_thread::sleep_for(std::chrono::milliseconds(500));
	}
	std::cout << "\nvalue: " << g_data << std::endl;
}
//atomic 操作默认使用一个特别的内存次序 memory_order 名为 memory_order_seq_cst, 它代表 sequential consistent memory order(顺序一致的内存次序)。低层的atomic操作能够放宽这一次序保证

int main()
{
	auto p = std::async(std::launch::async, provider);
	auto c = std::async(std::launch::async, consumer);

	return 0;
}