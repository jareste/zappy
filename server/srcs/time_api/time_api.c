#include "time_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "../log/log.h"

time_api* m_time = NULL;

/* Helper function to get the current system time in milliseconds */
// static long get_current_time_ms()
// {
//     struct timeval tv;
//     gettimeofday(&tv, NULL);
//     return tv.tv_sec * 1000L + tv.tv_usec / 1000;
// }

static long get_current_time_ms()
{
    struct timespec ts;
    /* Supposed to be faster than gettimeofday() */
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000;
}

time_api *time_api_get_local()
{
    // time_api_update(NULL);
    return m_time;
}

time_api *time_api_init(int t)
{
    time_api *api = malloc(sizeof(time_api));

    api->t = t;
    api->start_time_ms = get_current_time_ms();
    api->current_time_units = 0;
    return api;
}

int time_api_init_local(int t)
{
    m_time = time_api_init(t);
    if (!m_time)
    {
        log_msg(LOG_LEVEL_ERROR, "Failed to initialize time API.\n");
        return ERROR;
    }
    return SUCCESS;
}

void time_api_free(time_api *api)
{
    if (api)
    {
        free(api);
        api = NULL;
        return;
    }

    free(m_time);
    m_time = NULL;
}

/* Compute and return the current game time (in time units) */
uint64_t time_get_current_time_units(time_api *_api)
{
    time_api *api;
    long now;

    api= _api ? _api : m_time;
    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "Time API not initialized.\n");
        return -1;
    }

    now = get_current_time_ms();
    /* Each time unit lasts 1000/t milliseconds */
    return (uint64_t)((now - api->start_time_ms) * api->t / 1000);
}

/* Update the current game time stored in the API */
int time_api_update(time_api *_api)
{
    time_api *api;

    api= _api ? _api : m_time;
    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "Time API not initialized.\n");
        return ERROR;
    }

    api->current_time_units = time_get_current_time_units(api);
    return SUCCESS;
}

int m_is_event_buffer_full(event_buffer *buffer)
{
    return (buffer->count == MAX_EVENTS);
}

/* Schedule a client event in the client's fixed-size event buffer.
 * The event's exec_time is set to current game time + delay.
 */
int time_api_schedule_client_event(time_api *_api, event_buffer *buffer, int delay, int (*callback)(void *, void *), void *data, void *arg)
{
    event new_event;
    time_api *api;

    api= _api ? _api : m_time;
    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "Time API not initialized.\n");
        return -1;
    }

    if (buffer->count >= MAX_EVENTS)
    {
        return -1;
    }

    if (buffer->count > 0)
        new_event.exec_time = buffer->events[buffer->tail].exec_time + delay;
    else
        new_event.exec_time = api->current_time_units + delay;

    new_event.callback = callback;
    new_event.data = data;
    new_event.arg = arg;
    
    buffer->events[buffer->tail] = new_event;
    buffer->tail = (buffer->tail + 1) % MAX_EVENTS;
    buffer->count++;

    return 0;
}

int time_api_schedule_client_event_front(time_api *_api, event_buffer *buffer, int delay,\
    int (*callback)(void *, void *), void *data, void *arg)
{
    time_api* api;
    event new_event;

    api = _api ? _api : m_time;
    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "Time API not initialized.\n");
        return -1;
    }

    if (m_is_event_buffer_full(buffer))
        return -1;

    new_event.exec_time = api->current_time_units + delay;
    new_event.callback  = callback;
    new_event.data      = data;
    new_event.arg       = arg;

    /* move head backwards, insert there */
    buffer->head = (buffer->head - 1 + MAX_EVENTS) % MAX_EVENTS;
    buffer->events[buffer->head] = new_event;
    buffer->count++;
    return 0;
}

/* Process and execute all client events in the buffer that are due.
 * The function checks the event at the head of the circular buffer.
 */
int time_api_process_client_events(time_api *_api, event_buffer *buffer)
{
    time_api *api;
    event *ev;
    int prev_head;

    api = _api ? _api : m_time;
    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "Time API not initialized.\n");
        return ERROR;
    }

    if (buffer->count > 0)
    {
        ev = &buffer->events[buffer->head];
        if (ev->exec_time <= api->current_time_units)
        {
            buffer->head = (buffer->head + 1) % MAX_EVENTS;
            buffer->count--;
            prev_head = buffer->head;
            if (ev->callback)
                ev->callback(ev->data, ev->arg);

            if (prev_head == buffer->head && ev->arg)
            {
                free(ev->arg);
                ev->arg = NULL;
            }
        }
    }
    return SUCCESS;
}
