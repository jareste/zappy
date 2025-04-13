#include "time_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

time_api* m_time = NULL;

/* Helper function to get the current system time in milliseconds */
static long get_current_time_ms()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000L + tv.tv_usec / 1000;
}

time_api *time_api_get_local()
{
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

void time_api_init_local(int t)
{
    m_time = time_api_init(t);
}

/* Compute and return the current game time (in time units) */
int time_get_current_time_units(time_api *_api)
{
    time_api *api;
    long now;

    api= _api ? _api : m_time;
    if (!api)
    {
        fprintf(stderr, "Time API not initialized.\n");
        return -1;
    }

    now = get_current_time_ms();
    /* Each time unit lasts 1000/t milliseconds */
    return (int)((now - api->start_time_ms) * api->t / 1000);
}

/* Update the current game time stored in the API */
int time_api_update(time_api *_api)
{
    time_api *api;

    api= _api ? _api : m_time;
    if (!api)
    {
        fprintf(stderr, "Time API not initialized.\n");
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
int time_api_schedule_client_event(time_api *_api, event_buffer *buffer, int delay, void (*callback)(void *), void *data)
{
    event new_event;
    time_api *api;

    api= _api ? _api : m_time;
    if (!api)
    {
        fprintf(stderr, "Time API not initialized.\n");
        return -1;
    }

    if (buffer->count >= MAX_EVENTS)
    {
        fprintf(stderr, "Client event buffer is full. Event not scheduled.\n");
        return -1;
    }
    
    if (buffer->count > 0)
    {
        new_event.exec_time = buffer->events[buffer->tail].exec_time + delay;
    }
    else
    {
        new_event.exec_time = _api->current_time_units + delay;
    }

    new_event.callback = callback;
    new_event.data = data;
    
    buffer->events[buffer->tail] = new_event;
    buffer->tail = (buffer->tail + 1) % MAX_EVENTS;
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

    api = _api ? _api : m_time;
    if (!api)
    {
        fprintf(stderr, "Time API not initialized.\n");
        return ERROR;
    }

    while (buffer->count > 0)
    {
        ev = &buffer->events[buffer->head];
        if (ev->exec_time <= _api->current_time_units)
        {
            if (ev->callback)
                ev->callback(ev->data);

            buffer->head = (buffer->head + 1) % MAX_EVENTS;
            buffer->count--;
        }
        else
        {
            break;
        }
    }
    return SUCCESS;
}
