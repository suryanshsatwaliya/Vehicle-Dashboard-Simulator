#ifndef LOGGER_H
#define LOGGER_H

#include "vehicle.h"

/* Appends a trip summary to a text file. Returns 0 on success. */
int logger_save_trip(const Vehicle *v, const char *path);

#endif
