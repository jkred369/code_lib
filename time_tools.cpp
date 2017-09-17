//sleep usleep nanosleep select linux 下面比较
//usleep 有很大问题
//1.在一些平台下不是线程安全
//2.usleep 会影响信号
//3.大部分平台的帮助文档已经明确说明，
//该函数是已经被舍弃的函数
//还好，POSIX规范中有一个很好用的函数，nanosleep()，
//该函数没有usleep()的这些缺点，它的精度是纳秒级。
//在Solaris的多线程环境下编译器会自动把usleep()连接成nanosleep()
//Linux 下短延时推荐使用select 函数，因为准确

//详细讲解如下
//http://blog.csdn.net/hbuxiaofei/article/details/46416605

//使用nanosleep()函数对其他行为没有影响，不堵塞任何信号。
//nanosleep可以很好的保留中断时剩余时间
void process_sleep(double s)
{
    timespec time,remainder;
    double intpart;
    time.tv_nsec = (long)(modf(s,&intpart)*1e9);
    time.tv_sec = (int)intpart;
    while(nanosleep(&time,&remainder) == -1)
        time = remainder;
}

//select的精度是微妙，精确
struct timeval delay;
delay.tv_sec = 0;
delay.tv_usec = 20 * 1000; // 20 ms
select(0, NULL, NULL, NULL, &delay);
