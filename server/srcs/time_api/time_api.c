#include "time_api.h"
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include "../log/log.h"

time_api *m_time = NULL;

/* ------------------------------------------------------------------ */
/*  System clock                                                       */
/* ------------------------------------------------------------------ */

long get_current_time_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000L + ts.tv_nsec / 1000000;
}

/* ------------------------------------------------------------------ */
/*  Init / free                                                        */
/* ------------------------------------------------------------------ */

time_api *time_api_init(int t)
{
    time_api *api = malloc(sizeof(time_api));
    if (!api) return NULL;

    api->t                    = t;
    api->start_time_ms        = get_current_time_ms();
    api->paused_ms            = api->start_time_ms; /* starts paused */
    api->current_time_units   = 0;
    return api;
}

int time_api_init_local(int t)
{
    m_time = time_api_init(t);
    if (!m_time)
    {
        log_msg(LOG_LEVEL_ERROR,
            "[TIME] Failed to allocate time_api (t=%d)\n", t);
        return ERROR;
    }
    log_msg(LOG_LEVEL_BOOT,
        "[TIME] Initialised: t=%d (1 time_unit = %.1f ms real)\n",
        t, 1000.0 / (double)t);
    return SUCCESS;
}

time_api *time_api_get_local(void)
{
    return m_time;
}

void time_api_free(time_api *api)
{
    if (api)
    {
        free(api);
        return;
    }
    free(m_time);
    m_time = NULL;
}

/* ------------------------------------------------------------------ */
/*  Compute time units (does not touch state)                          */
/* ------------------------------------------------------------------ */

uint64_t time_get_current_time_units(time_api *_api)
{
    time_api *api;
    long      now;
    uint64_t  elapsed_ms;

    api = _api ? _api : m_time;
    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "[TIME] time_get_current_time_units: not initialised\n");
        return (uint64_t)-1;
    }

    if (api->paused_ms > 0)
        return api->current_time_units; /* frozen while paused */

    now        = get_current_time_ms();
    elapsed_ms = (now > api->start_time_ms) ? (now - api->start_time_ms) : 0;
    return (uint64_t)(elapsed_ms * api->t / 1000);
}

/* ------------------------------------------------------------------ */
/*  Pause / run                                                        */
/* ------------------------------------------------------------------ */

void time_api_pause(time_api *_api)
{
    time_api *api = _api ? _api : m_time;
    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "[TIME] pause: not initialised\n");
        return;
    }
    if (api->paused_ms > 0)
        return; /* already paused */

    api->paused_ms = get_current_time_ms();
    log_time_paused(api->current_time_units);
}

void time_api_run(time_api *_api)
{
    time_api *api = _api ? _api : m_time;
    long      now;
    long      paused_duration_ms;

    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "[TIME] run: not initialised\n");
        return;
    }
    if (api->paused_ms == 0)
    {
        log_msg(LOG_LEVEL_DEBUG, "[TIME] run called but not paused — ignoring\n");
        return;
    }

    now                = get_current_time_ms();
    paused_duration_ms = now - api->paused_ms;
    api->start_time_ms += paused_duration_ms;
    api->paused_ms      = 0;

    log_time_resumed(api->current_time_units, paused_duration_ms);
    time_api_update(NULL);
}

/* ------------------------------------------------------------------ */
/*  Update (advance stored time_units)                                 */
/* ------------------------------------------------------------------ */

int time_api_update(time_api *_api)
{
    time_api *api;
    uint64_t  old_units;
    uint64_t  new_units;

    api = _api ? _api : m_time;
    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "[TIME] update: not initialised\n");
        return ERROR;
    }

    if (api->paused_ms > 0)
        return SUCCESS; /* frozen */

    old_units = api->current_time_units;
    new_units = time_get_current_time_units(api);

    if (new_units != old_units)
        log_time_tick(old_units, new_units);

    api->current_time_units = new_units;
    return SUCCESS;
}

/* ------------------------------------------------------------------ */
/*  Event buffer helpers                                               */
/* ------------------------------------------------------------------ */

int m_is_event_buffer_full(event_buffer *buffer)
{
    return (buffer->count == MAX_EVENTS);
}

/* Schedule a single (non-buffered) event */
int time_api_schedule_single_event(time_api *_api, event *ev, int delay,
    int (*callback)(void *, void *), void *data, void *arg)
{
    time_api *api = _api ? _api : m_time;

    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "[TIME] schedule_single_event: not initialised\n");
        return -1;
    }

    ev->exec_time = api->current_time_units + delay;
    ev->callback  = callback;
    ev->data      = data;
    ev->arg       = arg;

    log_msg(LOG_LEVEL_DEBUG,
        "[TIME][SINGLE_EVENT] scheduled: delay=%d exec_at=%lu\n",
        delay, ev->exec_time);
    return 0;
}

/* Schedule into a client's circular event buffer (at tail) */
int time_api_schedule_client_event(time_api *_api, event_buffer *buffer, int delay,
    int (*callback)(void *, void *), void *data, void *arg)
{
    event     new_event;
    time_api *api = _api ? _api : m_time;

    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "[TIME] schedule_client_event: not initialised\n");
        return -1;
    }

    if (buffer->count >= MAX_EVENTS)
    {
        log_msg(LOG_LEVEL_WARN,
            "[TIME][EVT] Buffer full! count=%d max=%d — event dropped\n",
            buffer->count, MAX_EVENTS);
        return -1;
    }

    new_event.exec_time = api->current_time_units + delay;
    new_event.callback  = callback;
    new_event.data      = data;
    new_event.arg       = arg;

    log_msg(LOG_LEVEL_DEBUG,
        "[TIME][EVT] Scheduled: delay=%d current=%lu exec_at=%lu buffer_count=%d\n",
        delay, api->current_time_units, new_event.exec_time, buffer->count + 1);

    buffer->events[buffer->tail] = new_event;
    buffer->tail  = (buffer->tail + 1) % MAX_EVENTS;
    buffer->count++;
    return 0;
}

/* Schedule into a client's circular event buffer (at head — high priority) */
int time_api_schedule_client_event_front(time_api *_api, event_buffer *buffer, int delay,
    int (*callback)(void *, void *), void *data, void *arg)
{
    time_api *api = _api ? _api : m_time;
    event     new_event;

    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "[TIME] schedule_client_event_front: not initialised\n");
        return -1;
    }

    if (m_is_event_buffer_full(buffer))
    {
        log_msg(LOG_LEVEL_WARN,
            "[TIME][EVT_FRONT] Buffer full — event dropped\n");
        return -1;
    }

    new_event.exec_time = api->current_time_units + delay;
    new_event.callback  = callback;
    new_event.data      = data;
    new_event.arg       = arg;

    buffer->head = (buffer->head - 1 + MAX_EVENTS) % MAX_EVENTS;
    buffer->events[buffer->head] = new_event;
    buffer->count++;

    log_msg(LOG_LEVEL_DEBUG,
        "[TIME][EVT_FRONT] Scheduled: delay=%d exec_at=%lu buffer_count=%d\n",
        delay, new_event.exec_time, buffer->count);
    return 0;
}

/* Process a single (non-buffered) event if due */
int time_api_process_single_event(time_api *_api, event *ev)
{
    time_api *api = _api ? _api : m_time;

    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "[TIME] process_single_event: not initialised\n");
        return ERROR;
    }

    if (ev->exec_time <= api->current_time_units)
    {
        log_msg(LOG_LEVEL_DEBUG,
            "[TIME][SINGLE_EVENT] Executing: exec_at=%lu now=%lu\n",
            ev->exec_time, api->current_time_units);

        if (ev->callback)
            ev->callback(ev->data, ev->arg);

        if (ev->arg)
        {
            free(ev->arg);
            ev->arg = NULL;
        }
        return SUCCESS;
    }
    return ERROR; /* not due yet */
}

/* Process events at the head of the client buffer that are now due */
int time_api_process_client_events(time_api *_api, event_buffer *buffer)
{
    time_api *api;
    event    *ev;
    int       prev_head;

    api = _api ? _api : m_time;
    if (!api)
    {
        log_msg(LOG_LEVEL_ERROR, "[TIME] process_client_events: not initialised\n");
        return ERROR;
    }

    if (buffer->count > 0)
    {
        ev = &buffer->events[buffer->head];
        if (ev->exec_time <= api->current_time_units)
        {
            log_msg(LOG_LEVEL_TRACE,
                "[TIME][EVT_PROC] Executing event: exec_at=%lu now=%lu remaining=%d\n",
                ev->exec_time, api->current_time_units, buffer->count - 1);

            buffer->head  = (buffer->head + 1) % MAX_EVENTS;
            buffer->count--;
            prev_head = buffer->head;

            if (ev->callback)
                ev->callback(ev->data, ev->arg);

            /* Free arg only if the callback did not insert a new front event
             * (which would have moved buffer->head backward again). */
            if (prev_head == buffer->head && ev->arg)
            {
                free(ev->arg);
                ev->arg = NULL;
            }
        }
    }
    return SUCCESS;
}
