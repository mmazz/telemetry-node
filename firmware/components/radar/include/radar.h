#ifndef RADAR_H
#define RADAR_H

#include <stdint.h>

void radar_init(void);
float radar_get_last_distance_cm(void);
void radar_set_paused(bool paused);

#endif

