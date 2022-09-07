//
// Created by chris on 2022-08-17.
//

#pragma once

#ifndef ODO_DETECTIONDATA_H
#define ODO_DETECTIONDATA_H

#include <cstdint>
#include "RoiBox.h"

typedef struct DetectionData DetectionData;

struct DetectionData {
    char label[64] = {0};
    bool is_matched = false;
    bool alert_generated = false;
    int16_t num_matches = 0;
    //char source_name[256] = {0};
    int16_t source_id = 0;
    uint64_t track_id = 0;
    uint64_t class_id = 0;
    uint64_t parent_track_id = 0;
    int16_t frame_hits = 0;
    int16_t inference_hits = 0;
    double confidence = 0;
    RoiBox box;
    long detection_time;    // in milliseconds
};

#endif //ODO_DETECTIONDATA_H
