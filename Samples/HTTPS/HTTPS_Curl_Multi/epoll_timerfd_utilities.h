/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <time.h>
#include <sys/epoll.h>
#include <unistd.h>

/// Forward declaration of the data type passed to the handlers.
struct EventData;

/// <summary>
///     Function signature for event handlers.
/// </summary>
/// <param name="eventData">The provided event data</param>
typedef void (*EventHandler)(struct EventData *eventData);

/// <summary>
/// <para>Contains context data for epoll events.</para>
/// <para>When an event is registered with RegisterEventHandlerToEpoll, supply
/// a pointer to an instance of this struct.  The pointer must remain valid
/// for as long as the event is active.</para>
/// </summary>
/// <seealso cref="RegisterEventHandlerToEpoll" />
typedef struct EventData {
    /// <summary>
    /// Function which is called when the event occurs.
    /// </summary>
    EventHandler eventHandler;
    /// <summary>
    /// The file descriptor that generated the event.
    /// </summary>
    int fd;
} EventData;

/// <summary>
///    Creates an epoll instance.
/// </summary>
/// <returns>A valid epoll file descriptor on success, or -1 on failure</returns>
int CreateEpollFd(void);

/// <summary>
///     Registers an event with the epoll instance. If the event was previously added, that
///     registration will be modified to match the new mask.
/// </summary>
/// <param name="epollFd">Epoll file descriptor</param>
/// <param name="eventFd">File descriptor generating events for the epoll</param>
/// <param name="persistentEventData">Persistent event data structure. This must stay in memory
/// until the handler is removed from the epoll.</param>
/// <param name="epollEventMask">Bit mask for the epoll event type</param>
/// <returns>0 on success, or -1 on failure</returns>
int RegisterEventHandlerToEpoll(int epollFd, int eventFd, EventData *persistentEventData,
                                const uint32_t epollEventMask);

/// <summary>
///     Unregisters an event with the epoll instance.
/// </summary>
/// <param name="epollFd">Epoll file descriptor</param>
/// <param name="eventFd">File descriptor generating events for the epoll</param>
/// <returns>0 on success, or -1 on failure</returns>
int UnregisterEventHandlerFromEpoll(int epollFd, int eventFd);

/// <summary>
///     Sets the period of a timer.
/// </summary>
/// <param name="timerFd">Timer file descriptor</param>
/// <param name="period">The new period</param>
/// <returns>0 on success, or -1 on failure</returns>
int SetTimerFdToPeriod(int timerFd, const struct timespec *period);

/// <summary>
///     Sets a timer to fire once only, after a duration specified in milliseconds.
/// </summary>
/// <param name="timerFd">Timer file descriptor</param>
/// <param name="expiry">The time elapsed before it expires once</param>
/// <returns>0 on success, or -1 on failure</returns>
int SetTimerFdToSingleExpiry(int timerFd, const struct timespec *expiry);

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
/// <param name="persistentEventData">Persistent event data structure. This must stay in memory
/// until the handler is removed from the epoll.</param>
/// <param name="epollEventMask">Bit mask for the epoll event type</param>
/// <returns>A valid timerfd file descriptor on success, or -1 on failure</returns>
int CreateTimerFdAndAddToEpoll(int epollFd, const struct timespec *period,
                               EventData *persistentEventData, const uint32_t epollEventMask);

/// <summary>
///     Waits for an event on an epoll instance and triggers the handler.
/// </summary>
/// <param name="epollFd">
///     Epoll file descriptor which was created with <see cref="CreateEpollFd" />.
/// </param>
/// <returns>0 on success, or -1 on failure</returns>
int WaitForEventAndCallHandler(int epollFd);

/// <summary>
///     Closes a file descriptor and prints an error on failure.
/// </summary>
/// <param name="fd">File descriptor to close</param>
/// <param name="name">File descriptor name to use in error message</param>
void CloseFdAndPrintError(int fd, const char *name);