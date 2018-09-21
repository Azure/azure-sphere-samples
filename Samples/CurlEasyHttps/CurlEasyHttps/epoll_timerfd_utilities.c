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
        Log_Debug("ERROR: Could not create epoll instance: %d (%s)\n", errno, strerror(errno));
        return -1;
    }

    return epollFd;
}

int AddEventHandlerToEpoll(int epollFd, int eventFd, event_handler_t eventHandler,
                           const uint32_t epollEventMask)
{
    struct epoll_event eventToAdd;
    eventToAdd.data.ptr = eventHandler;
    eventToAdd.events = epollEventMask;

    // Register the eventFd on the epoll instance referred by epollFd
    // and register the eventHandler handler for events in epollEventMask
    if (epoll_ctl(epollFd, EPOLL_CTL_ADD, eventFd, &eventToAdd) == -1) {
        Log_Debug("ERROR: Could not add event to epoll instance %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    return 0;
}

int SetTimerFdInterval(int timerFd, const struct timespec *period)
{
    struct itimerspec newValue = {};
    newValue.it_value = *period;
    newValue.it_interval = *period;

    if (timerfd_settime(timerFd, 0, &newValue, NULL) < 0) {
        Log_Debug("ERROR: Could not set timerfd interval %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    return 0;
}

int ConsumeTimerFdEvent(int timerFd)
{
    uint64_t timerData = 0;

    if (read(timerFd, &timerData, sizeof(timerData)) == -1) {
        Log_Debug("ERROR: Could not read timerfd %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    return 0;
}

int CreateTimerFdAndAddToEpoll(int epollFd, const struct timespec *period,
                               event_handler_t eventHandler, const uint32_t epollEventMask)
{
    // Create the timerfd and arm it by setting the interval to period
    int timerFd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
    if (timerFd < 0) {
        Log_Debug("ERROR: Could not create timerfd %s (%d)\n", strerror(errno), errno);
        return -1;
    }
    if (SetTimerFdInterval(timerFd, period) != 0) {
        int result = close(timerFd);
        if (result != 0) {
            Log_Debug("ERROR: Could not close timerfd %s (%d)\n", strerror(errno), errno);
        }
        return -1;
    }

    if (AddEventHandlerToEpoll(epollFd, timerFd, eventHandler, epollEventMask) != 0) {
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
        Log_Debug("ERROR: Failed waiting on events: %s (%d)\n", strerror(errno), errno);
        return -1;
    }

    if (numEventsOccurred == 1 && event.data.ptr != NULL) {
        event_handler_t eventHandler = (event_handler_t)(event.data.ptr);
        eventHandler();
    }

    return 0;
}

void CloseFdAndPrintError(int fd, const char *fdName)
{
    if (fd >= 0) {
        int result = close(fd);
        if (result != 0) {
            Log_Debug("WARNING: Could not close fd %s: %s (%d).\n", fdName, strerror(errno), errno);
        }
    }
}