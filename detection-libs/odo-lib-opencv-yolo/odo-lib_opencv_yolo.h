#ifndef OPENCV_YOLO_LIBRARY_H
#define OPENCV_YOLO_LIBRARY_H

#include <cstdint>
#include "../../include/DetectionData.h"

void odo_load(bool use_cuda, float _confidence, float _nms, int net_width, int net_height,
              const char *class_list_path, const char *config_path, const char *weights_path);
void odo_detect(ulong &num, DetectionData *detectData, cv::Mat frame);
void odo_free();

#endif //OPENCV_YOLO_LIBRARY_H
