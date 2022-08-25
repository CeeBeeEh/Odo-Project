//
// Created by chris on 2022-08-17.
//

#ifndef ODO_ROIBOX_H
#define ODO_ROIBOX_H

#include <cstdint>

typedef struct RoiBox RoiBox;

struct RoiBox {
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
};

#endif //ODO_ROIBOX_H
