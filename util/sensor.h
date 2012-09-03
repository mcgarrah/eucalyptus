// -*- mode: C; c-basic-offset: 4; tab-width: 4; indent-tabs-mode: nil -*-
// vim: set softtabstop=4 shiftwidth=4 tabstop=4 expandtab:

/*************************************************************************
 * Copyright 2009-2012 Eucalyptus Systems, Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/.
 *
 * Please contact Eucalyptus Systems, Inc., 6755 Hollister Ave., Goleta
 * CA 93117, USA or visit http://www.eucalyptus.com/licenses/ if you need
 * additional information or have any questions.
 ************************************************************************/

#ifndef INCLUDE_SENSOR_H
#define INCLUDE_SENSOR_H

#include "ipc.h"

#ifndef _UNIT_TEST
#define MAX_SENSOR_NAME_LEN    64
#define MAX_SENSOR_VALUES      32 // by default 20
#define MAX_SENSOR_DIMENSIONS  32 // root, ephemeral[0-1], vol-XYZ
#define MAX_SENSOR_COUNTERS    2  // we only have two types of counters (summation|average) for now
#define MAX_SENSOR_METRICS     16 // currently 9 are implemented
#else
#define MAX_SENSOR_NAME_LEN    64
#define MAX_SENSOR_VALUES      5  // smaller sizes, for easier testing of limits
#define MAX_SENSOR_DIMENSIONS  3
#define MAX_SENSOR_COUNTERS    1
#define MAX_SENSOR_METRICS     2
#endif

#define DEFAULT_SENSOR_SLEEP_DURATION_USEC 15000000L
#define MIN_COLLECTION_INTERVAL_MS 1000L // below 1 second is too frequent
#define MAX_COLLECTION_INTERVAL_MS 86400000L // above 24 hours is too infrequent

typedef struct {
    long long timestampMs; // in milliseconds
    double value;          // measurement
    char available;        // if '1' then value is valid, otherwise it is not
} sensorValue;

typedef struct {
    char dimensionName [MAX_SENSOR_NAME_LEN]; // e.g. "default", "root", "vol-123ABC"
    sensorValue values [MAX_SENSOR_VALUES];   // array of values (not pointers, to simplify shared-memory region use)
    int valuesLen;                            // size of the array
    int firstValueIndex;                      // index into values[] of the first value (one that matches sequenceNum)
} sensorDimension;

static char * sensorCounterTypeName [] = {
    "[unused]",
    "summation",
    "average"
};

typedef struct {
    enum { SENSOR_UNUSED=0, SENSOR_SUMMATION, SENSOR_AVERAGE } type;
    long long collectionIntervalMs;                     // the spacing of values, based on sensor's configuration
    long long sequenceNum;                              // starts with 0 when sensor is reset and monotonically increases
    sensorDimension dimensions [MAX_SENSOR_DIMENSIONS]; // array of values (not pointers, to simplify shared-memory region use)
    int dimensionsLen;                                  // size of the array
} sensorCounter;

typedef struct {
    char metricName [MAX_SENSOR_NAME_LEN];        // e.g. "CPUUtilization"
    sensorCounter counters [MAX_SENSOR_COUNTERS]; // array of values (not pointers, to simplify shared-memory region use)
    int countersLen;                              // size of the array
} sensorMetric;

typedef struct {
    char resourceName [MAX_SENSOR_NAME_LEN];   // e.g. "i-1234567"
    char resourceType [MAX_SENSOR_NAME_LEN];   // e.g. "instance"
    sensorMetric metrics [MAX_SENSOR_METRICS]; // array of values (not pointers, to simplify shared-memory region use)
    int metricsLen;                            // size of the array
} sensorResource;

typedef struct {
    long long collection_interval_time_ms;
    int history_size;
    boolean initialized;
    int max_resources;
    int used_resources;
    sensorResource resources[1]; // if struct should be allocated with extra space after it for additional cache elements
} sensorResourceCache;

int sensor_init (sem * sem, sensorResourceCache * resources, int resources_size, boolean run_bottom_half);
int sensor_config (int new_history_size, long long new_collection_interval_time_ms);
int sensor_str2type (const char * counterType);
const char * sensor_type2str (int type);
int sensor_res2str (char * buf, int bufLen, sensorResource **res, int resLen);
int sensor_get_instance_data (const char * instanceId, const char ** sensorIds, int sensorIdsLen, sensorResource ** sr, int srLen);
int sensor_get_dummy_instance_data (long long sn, const char * instanceId, const char ** sensorIds, int sensorIdsLen, sensorResource ** srs, int srsLen); // TODO3.2: remove
int sensor_add_value (const char * instanceId,
                      const char * metricName,
                      const int counterType,
                      const char * dimensionName,
                      const long long sequenceNum,
                      const long long timestampMs,
                      const boolean available,
                      const double value);
int sensor_merge_records (const sensorResource * srs[], int srsLen, boolean fail_on_oom);

#endif
