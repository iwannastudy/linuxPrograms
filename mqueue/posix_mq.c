#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <mqueue.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

mqd_t mymq;

void *mq_r_run(void *arg)
{
    int msgid;
    struct mq_attr mqat;
    ssize_t ret = 0;

    mq_getattr(mymq, &mqat);
    printf("size = %d\n", mqat.mq_msgsize);
    printf("maxmsg = %d\n", mqat.mq_maxmsg);
    sleep(4);

    while (1)
    {
        ret = mq_receive(mymq, (char *)&msgid, sizeof(msgid), NULL);
        if(ret == -1)
        {
            perror("mq_receive error");
        }
        else
        {
            printf("receive %d bytes msg: %d\n", ret, msgid);
        }
    }
    
}

void *mq_s_run(void *arg)
{
    int ret = 0;
    int msgid = 0;

    while (1)
    {
        ret = mq_send(mymq, (char *)&msgid, sizeof(msgid), 0);
        if(ret == -1)
        {
            perror("mq_send error");
        }
        msgid++;
        sleep(2);
    }

}

int main()
{
    pthread_t sid, rid;
    struct mq_attr mqat;

    mqat.mq_maxmsg = 8;
    mqat.mq_msgsize = 4;

    mq_unlink("/mymq");

    mymq = mq_open("/mymq", O_CREAT | O_EXCL | O_RDWR, 0777, &mqat);
    if(mymq == -1)
    {
        perror("mq_open");
        return -1;
    }

    printf("mq_open ok %d\n", mymq);

    pthread_create(&sid, NULL, mq_s_run, NULL);

    pthread_create(&rid, NULL, mq_r_run, NULL);

    while (1)
    {
        /* code */
    }
    

    return 0;
}