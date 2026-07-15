#ifndef TELEMETRY_H
#define TELEMETRY_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include "imu.h"   // para imu_data_t

typedef struct
{
    float distance_cm;
    bool  distance_valid;
    struct tm time;
    bool  time_valid;
    imu_data_t imu;
    bool  imu_valid;
    int64_t timestamp_us;
} telemetry_data_t;

#endif // TELEMETRY_H
