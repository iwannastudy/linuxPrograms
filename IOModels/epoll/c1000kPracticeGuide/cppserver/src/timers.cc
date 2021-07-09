#include <map>
#include <time.h>
#include <stdlib.h>
#include "timers.h"
#include <assert.h>
#include <stdio.h>

TimeValue getNowTimeStamp()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

int TIMER::add_timer(void *fditem, TimeValue timeStamp)
{
    timer_time_arg.insert(std::make_pair(timeStamp, fditem));
    timer_arg_time.insert(std::make_pair(fditem, timeStamp));
    return 0;
}

void *TIMER::get_timer(TimeValue timeStamp) 
{
    std::multimap<TimeValue, void *>::iterator iter = timer_time_arg.begin();
    void *fditem = iter->second;

    if (iter == timer_time_arg.end()) 
    {
        return NULL;
    }

    if (iter->first > timeStamp) 
    {
        show();
        return NULL;
    }
    timer_time_arg.erase(iter);

    std::multimap<void *, TimeValue>::iterator pos_arg_time;

    for (pos_arg_time = timer_arg_time.lower_bound(fditem);
         pos_arg_time != timer_arg_time.upper_bound(fditem); ++pos_arg_time) 
    {
        if (pos_arg_time->second == iter->first) 
        {
            timer_arg_time.erase(pos_arg_time);
            show();
            return iter->second;
        }
    }
    assert(0);
    show();
    return iter->second;
}

void TIMER::show() const
{
    return;
    printf("arg_time %zu   time_arg %zu \n", timer_arg_time.size(),
           timer_time_arg.size());
}
void TIMER::remove_timer(void *fditem)
{

    std::multimap<void *, TimeValue>::iterator pos_arg_time;
    for (pos_arg_time = timer_arg_time.lower_bound(fditem);
         pos_arg_time != timer_arg_time.upper_bound(fditem);)
    {
        std::multimap<TimeValue, void *>::iterator iter;
        TimeValue t = pos_arg_time->second;
        for (iter = timer_time_arg.lower_bound(t);
             iter != timer_time_arg.upper_bound(t);)
        {
            if (iter->second == fditem)
            {
                timer_time_arg.erase(iter++);
            }
            else
            {
                iter++;
            }
        }
        timer_arg_time.erase(pos_arg_time++);
    }
    show();
}

TimeValue TIMER::get_mintimer() const
{
    if (timer_time_arg.size() == 0)
    {
        return TIMER::mintimer;
    }
    TimeValue timeStamp = getNowTimeStamp();
    std::multimap<TimeValue, void *>::const_iterator iter =
        timer_time_arg.begin();
    TimeValue t = timeStamp - iter->first;
    if (t >= 0)
    {
        return 0;
    }
    return -t;
}
