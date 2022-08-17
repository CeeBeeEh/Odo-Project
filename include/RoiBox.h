//
// Created by chris on 2022-08-17.
//

#ifndef ODO_ROIBOX_H
#define ODO_ROIBOX_H

#include <cstdint>

typedef struct _RoiBox RoiBox;

struct _RoiBox {
    int32_t top_left[2] = {0,0};
    int32_t top_right[2] = {0,0};
    int32_t bottom_left[2] = {0,0};
    int32_t bottom_right[2] = {0,0};
    int32_t width = 0;
    int32_t height = 0;
};

RoiBox xyToBox(int32_t x, int32_t y, int32_t width, int32_t height) {
    RoiBox box;
    box.top_left[0] = x;
    box.top_left[1] = y;
    box.top_right[0] = x + width;
    box.top_right[1] = y;
    box.bottom_left[0] = x;
    box.bottom_left[0] = y + height;
    box.bottom_right[0] = x + width;
    box.bottom_right[1] = y + height;
    box.width = width;
    box.height = height;
    return box;
}

#endif //ODO_ROIBOX_H
