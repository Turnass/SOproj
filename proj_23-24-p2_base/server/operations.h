#ifndef SERVER_OPERATIONS_H
#define SERVER_OPERATIONS_H

#include <stddef.h>

/// Initializes the EMS state.
/// @param delay_us Delay in microseconds.
/// @return 0 if the EMS state was initialized successfully, 1 otherwise.
int ems_init(unsigned int delay_us);

/// Destroys the EMS state.
int ems_terminate();

/// Creates a new event with the given id and dimensions.
/// @param event_id Id of the event to be created.
/// @param num_rows Number of rows of the event to be created.
/// @param num_cols Number of columns of the event to be created.
/// @return 0 if the event was created successfully, 1 otherwise.
int ems_create(unsigned int event_id, size_t num_rows, size_t num_cols);

/// Creates a new reservation for the given event.
/// @param event_id Id of the event to create a reservation for.
/// @param num_seats Number of seats to reserve.
/// @param xs Array of rows of the seats to reserve.
/// @param ys Array of columns of the seats to reserve.
/// @return 0 if the reservation was created successfully, 1 otherwise.
int ems_reserve(unsigned int event_id, size_t num_seats, size_t *xs, size_t *ys);

/// Send the output of the event to the client.
/// @param resp_fd File descriptor to send the output through.
/// @param event_id Id of the event to send.
/// @return 0 if the event output was sent successfully, 1 otherwise.
int ems_show(int resp_pipe, unsigned int event_id);

/// Sends the output to the client of all the events ids.
/// @param resp_fd File descriptor to send the output through.
/// @return 0 if the event ids were sent successfully, 1 otherwise.
int ems_list_events(int resp_pipe);

#endif  // SERVER_OPERATIONS_H
