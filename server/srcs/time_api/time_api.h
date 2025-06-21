#ifndef TIME_API_H
#define TIME_API_H

#include <sys/time.h>
#include "../game/game_structs.h"
#include <error_codes.h>
#include <ft_malloc.h>
#include <stdint.h>

typedef struct
{
    int t;
    long start_time_ms;
    uint64_t current_time_units; /* No overflow protection, but goodluck trying to overflow :) */
} time_api;

/* Initializes the time_api with the time divider 't'.
 * Returns a pointer to an allocated time_api or NULL on failure.
 */
time_api *time_api_init(int t);
void time_api_free(time_api *api);
time_api *time_api_get_local();

/* Initializes the local time_api with the time divider 't'.
 * This function should be called once at the start of the program.
 */
int time_api_init_local(int t);

/* Returns the current game time units based on the elapsed real time. */
uint64_t time_get_current_time_units(time_api *api);

/* Updates the API's notion of current game time.
 * Should be called periodically in the main loop.
 */
int time_api_update(time_api *_api);

/* Schedules a client event into the given fixed-size event buffer.
 * delay: number of time units from now when the event must trigger.
 * Returns 0 on success or -1 if the event buffer is full.
 */
int time_api_schedule_client_event(time_api *_api, event_buffer *buffer, int delay, int (*callback)(void *, void *), void *data, void *arg);

int time_api_schedule_client_event_front(time_api *_api, event_buffer *buffer, int delay, int (*callback)(void *, void *), void *data, void *arg);

/* Processes all scheduled client events in the provided event buffer whose execution
 * time is less than or equal to the current game time. Events are processed in FIFO order.
 */
int time_api_process_client_events(time_api *_api, event_buffer *buffer);

#endif /* TIME_API_H */
