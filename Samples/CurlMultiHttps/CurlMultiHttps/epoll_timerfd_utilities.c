/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/timerfd.h>
#include <applibs/log.h>
#include "epoll_timerfd_utilities.h"

int CreateEpollFd(void)
{
    int epollFd = -1;

    epollFd = epoll_create1(0);
    if (epollFd == -1) {
        Log_Debug("ERROR: Could not create epoll instance: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    return epollFd;
}

int RegisterEventHandlerToEpoll(int epollFd, int eventFd, EventData *persistentEventData,
                                const uint32_t epollEventMask)
{
    persistentEventData->fd = eventFd;
    struct epoll_event eventToAddOrModify = {.data.ptr = persistentEventData,
                                             .events = epollEventMask};

    // Register the eventFd on the epoll instance referred by epollFd
    // and register the eventHandler handler for events in epollEventMask.
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, eventFd, &eventToAddOrModify) == -1) {
        // If the Add fails, retry with the Modify as the file descriptor has already been
        // added to the epoll set after it was removed by the kernel upon its closure.
        if (epoll_ctl(epollFd, EPOLL_CTL_MOD, eventFd, &eventToAddOrModify) == -1) {
            Log_Debug("ERROR: Could not register event to epoll instance: %s (%d).\n",
                      strerror(errno), errno);
            return -1;
        }
    }

    return 0;
}

int UnregisterEventHandlerFromEpoll(int epollFd, int eventFd)
{
    int res = 0;
    // Unregister the eventFd on the epoll instance referred by epollFd.
    if ((res = epoll_ctl(epollFd, EPOLL_CTL_DEL, eventFd, NULL)) == -1) {
        if (res == -1 && errno != EBADF) { // Ignore EBADF errors
            Log_Debug("ERROR: Could not remove event from epoll instance: %s (%d).\n",
                      strerror(errno), errno);
            return -1;
        }
    }

    return 0;
}

int SetTimerFdToPeriod(int timerFd, const struct timespec *period)
{
    struct itimerspec newValue = {.it_value = *period, .it_interval = *period};

    if (timerfd_settime(timerFd, 0, &newValue, NULL) < 0) {
        Log_Debug("ERROR: Could not set timerfd period: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    return 0;
}

int SetTimerFdToSingleExpiry(int timerFd, const struct timespec *expiry)
{
    struct itimerspec newValue = {.it_value = *expiry, .it_interval = {}};

    if (timerfd_settime(timerFd, 0, &newValue, NULL) < 0) {
        Log_Debug("ERROR: Could not set timerfd interval: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    return 0;
}

int ConsumeTimerFdEvent(int timerFd)
{
    uint64_t timerData = 0;

    if (read(timerFd, &timerData, sizeof(timerData)) == -1) {
        Log_Debug("ERROR: Could not read timerfd %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    return 0;
}

int CreateTimerFdAndAddToEpoll(int epollFd, const struct timespec *period,
                               EventData *persistentEventData, const uint32_t epollEventMask)
{
    // Create the timerfd and arm it by setting the interval to period
    int timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timerFd < 0) {
        Log_Debug("ERROR: Could not create timerfd: %s (%d).\n", strerror(errno), errno);
        return -1;
    }
    if (SetTimerFdToPeriod(timerFd, period) != 0) {
        int result = close(timerFd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close timerfd: %s (%d).\n", strerror(errno), errno);
        }
        return -1;
    }

    persistentEventData->fd = timerFd;
    if (RegisterEventHandlerToEpoll(epollFd, timerFd, persistentEventData, epollEventMask) != 0) {
        return -1;
    }

    return timerFd;
}

int WaitForEventAndCallHandler(int epollFd)
{
    struct epoll_event event;
    int numEventsOccurred = epoll_wait(epollFd, &event, 1, -1);

    if (numEventsOccurred == -1) {
        if (errno == EINTR) {
            // interrupted by signal, e.g. due to breakpoint being set; ignore
            return 0;
        }
        Log_Debug("ERROR: Failed waiting on events: %s (%d).\n", strerror(errno), errno);
        return -1;
    }

    if (numEventsOccurred == 1 && event.data.ptr != NULL) {
        EventData *eventData = event.data.ptr;
        eventData->eventHandler(eventData);
    }

    return 0;
}

void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}