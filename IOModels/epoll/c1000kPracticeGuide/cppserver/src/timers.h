#ifndef TIMER_H
#define TIMER_H

#include <map>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

typedef long long TimeValue;   // ms

// struct timer_event {
    // TimeValue times;
    // void *arg;
    // int eventtype;
// };

// Return X ms from UTC.
TimeValue getNowTimeStamp();

class TIMER     //TODO: signelton mode
{
    static const int64_t mintimer = 1000 * 1;

   public:
    TIMER() {}

    virtual ~TIMER() {}

    // add new timer
    int add_timer(void *fditem, TimeValue timeStamp);

    //
    void *get_timer(TimeValue timeStamp);

    // remove timer
    void remove_timer(void *fditem);

    TimeValue get_mintimer() const;

    int get_arg_time_size() const { return timer_arg_time.size(); }
    int get_time_arg_size() const { return timer_time_arg.size(); }

    void show() const;

   private:
    std::multimap<void *, TimeValue> timer_arg_time;

    std::multimap<TimeValue, void *> timer_time_arg;
};

#endif
