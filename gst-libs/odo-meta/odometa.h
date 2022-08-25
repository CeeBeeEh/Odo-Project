//
// Created by chris on 8/9/22.
//

#ifndef GST_ODO_DETECTOR_ODOMETA_H
#define GST_ODO_DETECTOR_ODOMETA_H

#include <gst/gst.h>
#include <gst/gstmeta.h>
#include <array>
#include <opencv2/opencv.hpp>
#include <opencv4/opencv2/core/mat.hpp>

#include "../../include/DetectionData.h"

G_BEGIN_DECLS

#define GST_ODO_META_API_TYPE (gst_odo_meta_api_get_type())
#define GST_ODO_META_INFO  (gst_odo_meta_get_info())

typedef struct _GstOdoMeta GstOdoMeta;

struct _GstOdoMeta {
    GstMeta meta;

    gint inferenceInterval;
    gboolean isInferenceFrame;
    DetectionData *detections;
    //cv::Mat frame;
    ulong detectionCount;
};

GType gst_odo_meta_api_get_type(void);
void gst_odo_meta_free (GstMeta *meta, GstBuffer *buffer);
const GstMetaInfo *gst_odo_meta_get_info(void);

#define GST_ODO_META_GET(buf) ((GstOdoMeta *)gst_buffer_get_meta((buf), GST_ODO_META_API_TYPE))
GstOdoMeta *gst_buffer_get_odo_meta (GstBuffer * buffer);
GstOdoMeta *gst_odo_meta_add(GstBuffer *buffer, DetectionData *detections, ulong &detectCount,
                             gint inferenceInterval, gboolean isInferenceFrame);
#define GST_ODO_META_ADD(buf) ((GstOdoMeta *)gst_buffer_add_meta(buf,gst_odo_meta_get_info(),(gpointer)NULL))

G_END_DECLS

#endif //GST_ODO_DETECTOR_ODOMETA_H
