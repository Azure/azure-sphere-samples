/* Copyright (c) Microsoft Corporation. All rights reserved.
   Licensed under the MIT License. */

#pragma once
#include <time.h>

#include <unistd.h>

#include <applibs/eventloop.h>

/// <summary>
/// Opaque handle. Obtain via <see cref="CreateEventLoopPeriodicTimer" />
/// or <see cref="CreateEventLoopDisarmedTimer" /> and dispose of via
/// <see cref="DisposeEventLoopTimer" />.
/// </summary>
typedef struct EventLoopTimer EventLoopTimer;

/// <summary>
/// Applications implement a function with this signature to be
/// notified when a timer expires.
/// </summary>
/// <param name="timer">The timer which has expired.</param>
/// <seealso cref="CreateEventLoopPeriodicTimer" />
/// <seealso cref="CreateEventLoopDisarmedTimer" />
typedef void (*EventLoopTimerHandler)(EventLoopTimer *timer);

/// <summary>
/// Create a periodic timer which is invoked on the event loop. The timer
/// will begin firing immediately.
/// </summary>
/// <param name="eventLoop">Event loop to which the timer will be added.</param>
/// <param name="handler">Callback to invoke when the timer expires.</param>
/// <param name="period">Timer period.</param>
/// <returns>On success, pointer to new EventLoopTimer, which should be disposed of
/// with <see cref="DisposeEventLoopTimer" />. On failure, returns NULL, with more
/// information available in errno.</returns>.
EventLoopTimer *CreateEventLoopPeriodicTimer(EventLoop *eventLoop, EventLoopTimerHandler handler,
                                             const struct timespec *period);

/// <summary>
/// Create a disarmed timer. After the timer has been allocated, call
/// <see cref="SetEventLoopTimerPeriod" /> or <see cref="SetEventLoopTimerOneShot" />
/// to arm the timer.
/// </summary>
/// <param name="eventLoop">Event loop to which the timer will be added.</param>
/// <param name="handler">Callback to invoke when the timer expires.</param>
/// <returns>On success, pointer to new EventLoopTimer, which should be disposed of
/// with <see cref="DisposeEventLoopTimer" />. On failure, returns NULL, with more
/// information available in errno.</returns>.
EventLoopTimer *CreateEventLoopDisarmedTimer(EventLoop *eventLoop, EventLoopTimerHandler handler);

/// <summary>
/// Dispose of a timer which was allocated with <see cref="CreateEventLoopPeriodicTimer" />
/// or <see cref="CreateEventLoopDisarmedTimer" />.
/// It is safe to call this function with a NULL pointer.
/// </summary>
/// <param name="timer">Successfully allocated event loop timer, or NULL.</param>
void DisposeEventLoopTimer(EventLoopTimer *timer);

/// <summary>
/// The timer callback should call this function to consume the timer event.
/// </summary>
/// <param name="timer">Successfully allocated timer.</param>
/// <returns>0 on success, -1 on failure, in which case errno contains more information.</returns>
int ConsumeEventLoopTimerEvent(EventLoopTimer *timer);

/// <summary>
/// Change the timer's period. This function should only be called to change an existing
/// timer's period. It does not have to be called to set the initial period - that is
/// handled by <see cref="CreateEventLoopPeriodicTimer" />.
/// </summary>
/// <param name="timer">Timer previously allocated with <see cref="CreateEventLoopPeriodicTimer" />
/// or <see cref="CreateEventLoopDisarmedTimer" />.</param>
/// <param name="period">New timer period.</param>
/// <returns>0 on success, -1 on failure, in which case errno contains more information.</returns>
/// <seealso cref="SetEventLoopTimerOneShot" />
/// <seealso cref="DisarmEventLoopTimer" />
int SetEventLoopTimerPeriod(EventLoopTimer *timer, const struct timespec *period);

/// <summary>
/// Set the timer to expire one after a specified period.
/// </summary>
/// <returns>0 on succcess, -1 on failure, in which case errno contains more information.</returns>
/// <param name="timer">Timer previously allocated with <see cref="CreateEventLoopPeriodicTimer" />
/// or <see cref="CreateEventLoopDisarmedTimer" />.</param>
/// <param name="delay">Period to wait before timer expires.</param>
/// <returns>0 on success, -1 on failure, in which case errno contains more
/// information.</returns>
/// <seealso cref="SetEventLoopTimerPeriod" />
/// <seealso cref="DisarmEventLoopTimer" />
int SetEventLoopTimerOneShot(EventLoopTimer *timer, const struct timespec *delay);

/// <summary>
/// Disarm an existing event loop timer.
/// </summary>
/// <param name="timer">Timer previously allocated with <see cref="CreateEventLoopPeriodicTimer" />
/// or <see cref="CreateEventLoopDisarmedTimer" />.</param>
/// <returns>0 on success; -1 on failure, in which case errno contains more
/// information.</returns>
/// <seealso cref="SetEventLoopTimerOneShot" />
/// <seealso cref="SetEventLoopTimerPeriod" />
int DisarmEventLoopTimer(EventLoopTimer *timer);
