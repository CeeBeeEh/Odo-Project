#ifndef OPENCV_YOLO_LIBRARY_H
#define OPENCV_YOLO_LIBRARY_H

#include <cstdint>
#include "../../include/DetectionData.h"

void odo_load(bool use_cuda, float _confidence, float _nms, int net_width, int net_height,
              const char *class_list_path, const char *config_path, const char *weights_path);
void odo_detect(ulong &num, DetectionData *detectData, unsigned char *img,
                size_t img_format, int img_width, int img_height);
void odo_free();

#endif //OPENCV_YOLO_LIBRARY_H
