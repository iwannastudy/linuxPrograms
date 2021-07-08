#include "network.h"
#include "socket_util.h"
#include "timers.h"

pthread_t t_all[MAX_CPU] = {0};
cpu_set_t mask[MAX_CPU];

// 命令管道
int sockfdpair[MAX_CPU][2] = {};
int cpu_cmd_map[MAX_CPU][2];
int cpu_num = 1;
bool close_fun(int fd) { return true; }

volatile int jobs = 0;

struct epoll_event *evlist;

pthread_mutex_t dispatch_mutex;
pthread_cond_t dispatch_cond;

PFDITEM gFDITEM[MAX_FD];
extern char senddata[2048];

int main() {

    pthread_mutex_init(&dispatch_mutex, NULL);
    pthread_cond_init(&dispatch_cond, NULL);

    for (unsigned int i = 0; i < sizeof(senddata); ++i) 
    {
        senddata[i] = i % 26 + 'a';
    }
    // pipe
    signal(SIGPIPE, SIG_IGN);  // sigpipe 信号屏蔽

    // bind
    evlist = (struct epoll_event *)malloc(MAXEPOLLEVENT * sizeof(struct epoll_event));
    int ret = 0;
    int main_epollfd = epoll_create(MAXEPOLLEVENT);
    struct sockaddr_in servaddr;
    int listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP/*==0*/);
    if (listenfd <= 0) 
    {
        perror("socket");
        return 0;
    }

    ret = set_reused(listenfd);
    if (ret < 0) 
    {
        perror("setsockopt");
        exit(1);
    }

    for (int i = 0; i < MAX_FD; ++i) 
    {
        PFDITEM p = new FDITEM;
        assert(p);
        p->fd = i;
        gFDITEM[i] = p;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(listenport);

    ret = bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    if (ret < 0) 
    {
        perror("setsockopt");
        exit(1);
    }

    ret = listen(listenfd, 50000);
    if (ret < 0) 
    {
        perror("listen");
        exit(1);
    }
    //
    struct epoll_event ev = {0};

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = listenfd;
    set_noblock(listenfd);

    gFDITEM[listenfd]->m_readfun = accept_readfun;
    if (epoll_ctl(main_epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1) 
    {
        perror("epoll_ctl: listen_sock");
        exit(-1);
    }

    // 到Cpu 核心数目
    cpu_num = sysconf(_SC_NPROCESSORS_CONF) * 1;
    
    // 生成管理通道
    for (int i = 1; i < cpu_num; ++i) 
    {
        // Create pair of connected sockets
        socketpair(AF_LOCAL/*==AF_UNIX*/, SOCK_STREAM, 0, sockfdpair[i]);

        ev.events = EPOLLIN | EPOLLET;
        ev.data.fd = sockfdpair[i][0];
        set_noblock(sockfdpair[i][0]);

        gFDITEM[sockfdpair[i][0]]->m_writefun = accept_readfun;
        if (epoll_ctl(main_epollfd, EPOLL_CTL_ADD, sockfdpair[i][0], &ev) == -1) 
        {
            perror("epoll_ctl: listen_sock");
            exit(-1);
        }
    }

    for (int i = 1; i < cpu_num; ++i) 
    {
        Log << " cpu " << i << "  " << sockfdpair[i][0] << "  " << sockfdpair[i][1]
            << std::endl;
    }

    // start [cpu-1] workers binding to cpu and sockfdpair.
    for (int i = 1; i < cpu_num; ++i) 
    {
        CPU_ZERO(&mask[i]);
        CPU_SET(i, &mask[i]);
        struct WorkerThreadArgs *args = new WorkerThreadArgs;

        args->cpuid = i % (sysconf(_SC_NPROCESSORS_CONF) - 1) + 1;
        args->orderfd = sockfdpair[i][1];

        // 启动线程
        pthread_create(&t_all[i], NULL, worker_thread, (void *)args);
        //绑定亲元性
        pthread_setaffinity_np(t_all[i], sizeof(cpu_set_t), (const cpu_set_t *)&(mask[i]));
    }

    pthread_t dispatch_t;

    pthread_create(&dispatch_t, NULL, dispatch_conn, NULL);

    // 启动主线程
    pthread_t t_main;
    cpu_set_t mask_main;
    // pthread_attr_t attr_main;

    CPU_ZERO(&mask_main);
    CPU_SET(0, &mask_main);

    // 启动线程
    pthread_create(&t_main, NULL, main_thread, (void *)&main_epollfd);
    //绑定亲元性
    pthread_setaffinity_np(t_main, sizeof(cpu_set_t), (const cpu_set_t *)&mask_main);

    pthread_join(t_main, NULL);
    return 0;
}

void *main_thread(void *arg) 
{
    int main_epollfd = *(int *)arg;

    TIMER global_timer;
    int timeout = 1000;     // ms
    while (true) 
    {
        process_event(main_epollfd, evlist, timeout, &global_timer);
        if (global_timer.get_arg_time_size() > 0) 
        {
            timeout = global_timer.get_mintimer();
        } 
        else 
        {
            timeout = 1000;
        }
        if (jobs < 0) 
        {
            free(evlist);
            return 0;
        }
    }
}
