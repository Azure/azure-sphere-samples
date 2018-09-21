#pragma once
#include <time.h>
#include <sys/epoll.h>
#include <unistd.h>

typedef void (*event_handler_t)(void);

/// <summary>
///    Creates an epoll instance.
/// </summary>
/// <returns>A valid epoll file descriptor on success, or -1 on failure</returns>
int CreateEpollFd(void);

/// <summary>
///     Registers an event handler to an epoll instance.
/// </summary>
/// <param name="epollFd">Epoll file descriptor</param>
/// <param name="eventFd">File descriptor generating events for the epoll</param>
/// <param name="eventHandler">Event handler</param>
/// <param name="epollEventMask">Bit mask for the epoll event type</param>
/// <returns>0 on success, or -1 on failure</returns>
int AddEventHandlerToEpoll(int epollFd, int eventFd, event_handler_t eventHandler,
                           const uint32_t epollEventMask);

/// <summary>
///     Changes the period of a timerfd.
/// </summary>
/// <param name="timerFd">Timer file descriptor</param>
/// <param name="period">The new period</param>
/// <returns>0 on success, or -1 on failure</returns>
int SetTimerFdInterval(int timerFd, const struct timespec *period);

/// <summary>
///     Consumes an event by reading from the timer file descriptor.
///     If the event is not consumed, then it will immediately recur.
/// </summary>
/// <param name="timerFd">Timer file descriptor</param>
/// <returns>0 on success, or -1 on failure</returns>
int ConsumeTimerFdEvent(int timerFd);

/// <summary>
///     Creates a timerfd and adds it to an epoll instance.
/// </summary>
/// <param name="epollFd">Epoll file descriptor</param>
/// <param name="period">The timer period</param>
/// <param name="eventHandler">Event handler</param>
/// <param name="epollEventMask">Bit mask for the epoll event type</param>
/// <returns>A valid timerfd file descriptor on success, or -1 on failure</returns>
int CreateTimerFdAndAddToEpoll(int epollFd, const struct timespec *period,
                               event_handler_t eventHandler, const uint32_t epollEventMask);

/// <summary>
///     Waits for an event on an epoll instance and triggers the handler.
/// </summary>
/// <param name="epollFd">Epoll file descriptor</param>
/// <returns>0 on success, or -1 on failure</returns>
int WaitForEventAndCallHandler(int epollFd);

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="name">File descriptor name to use in error message</param>
void CloseFdAndPrintError(int fd, const char *name);