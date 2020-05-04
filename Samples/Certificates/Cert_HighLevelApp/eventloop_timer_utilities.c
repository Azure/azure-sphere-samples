/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <stdbool.h>
#include <stdlib.h>

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/timerfd.h>

#include <applibs/log.h>
#include <applibs/eventloop.h>

#include "eventloop_timer_utilities.h"

static int SetTimerPeriod(int timerFd, const struct timespec *initial,
                          const struct timespec *repeat);

static int SetTimerPeriod(int timerFd, const struct timespec *initial,
                          const struct timespec *repeat)
{
    static const struct timespec nullTimeSpec = {.tv_sec = 0, .tv_nsec = 0};
    struct itimerspec newValue = {.it_value = initial ? *initial : nullTimeSpec,
                                  .it_interval = repeat ? *repeat : nullTimeSpec};

    if (timerfd_settime(timerFd, /* flags */ 0, &newValue, /* old_value */ NULL) == -1) {
        Log_Debug("ERROR: Could not set timer period: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    return 0;
}

struct EventLoopTimer {
    EventLoop *eventLoop;
    EventLoopTimerHandler handler;
    int fd;
    EventRegistration *registration;
};

// This satisfies the EventLoopIoCallback signature.
static void TimerCallback(EventLoop *el, int fd, EventLoop_IoEvents events, void *context)
{
    EventLoopTimer *timer = (EventLoopTimer *)context;

    timer->handler(timer);
}

EventLoopTimer *CreateEventLoopPeriodicTimer(EventLoop *eventLoop, EventLoopTimerHandler handler,
                                             const struct timespec *period)
{
    if (handler == NULL) {
        errno = EINVAL;
        return NULL;
    }

    EventLoopTimer *timer = malloc(sizeof(EventLoopTimer));
    if (timer == NULL) {
        return NULL;
    }

    timer->eventLoop = eventLoop;
    timer->handler = handler;

    // Initialize to unused values in case have to clean up partially initialized object.
    timer->fd = -1;
    timer->registration = NULL;

    timer->fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timer->fd == -1) {
        Log_Debug("ERROR: Unable to create timer: %s (%d).\n", strerror(errno), errno);
        goto failed;
    }

    if (SetTimerPeriod(timer->fd, /* initial */ period, /* repeat */ period) == -1) {
        goto failed;
    }

    timer->registration =
        EventLoop_RegisterIo(eventLoop, timer->fd, EventLoop_Input, TimerCallback, timer);
    if (timer->registration == NULL) {
        Log_Debug("ERROR: Unable to register timer event: %s (%d).\n", strerror(errno), errno);
        goto failed;
    }

    return timer;

failed:
    DisposeEventLoopTimer(timer);
    return NULL;
}

EventLoopTimer *CreateEventLoopDisarmedTimer(EventLoop *eventLoop, EventLoopTimerHandler handler)
{
    return CreateEventLoopPeriodicTimer(eventLoop, handler, NULL);
}

void DisposeEventLoopTimer(EventLoopTimer *timer)
{
    if (timer == NULL) {
        return;
    }

    EventLoop_UnregisterIo(timer->eventLoop, timer->registration);

    if (timer->fd != -1) {
        close(timer->fd);
    }

    free(timer);
}

int ConsumeEventLoopTimerEvent(EventLoopTimer *timer)
{
    uint64_t timerData = 0;

    if (read(timer->fd, &timerData, sizeof(timerData)) == -1) {
        Log_Debug("ERROR: Could not read timerfd %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    return 0;
}

int SetEventLoopTimerPeriod(EventLoopTimer *timer, const struct timespec *period)
{
    return SetTimerPeriod(timer->fd, /* initial */ period, /* period */ period);
}

int SetEventLoopTimerOneShot(EventLoopTimer *timer, const struct timespec *delay)
{
    return SetTimerPeriod(timer->fd, /* initial */ delay, /* repeat */ NULL);
}

int DisarmEventLoopTimer(EventLoopTimer *timer)
{
    return SetTimerPeriod(timer->fd, /* initial */ NULL, /* repeat */ NULL);
}
