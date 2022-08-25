//
// Created by chris on 8/9/22.
//

#include <cstdio>
#include <iostream>
#include "odometa.h"

static gboolean gst_odo_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer);
static gboolean gst_odo_meta_transform(GstBuffer *transbuf, GstMeta *meta, GstBuffer *buffer,
                                             GQuark type, gpointer data);

GType gst_odo_meta_api_get_type(void) {
    static const gchar *tags[] = {nullptr};
    static GType type = 0;
    if (g_once_init_enter (&type)) {
        GType _type = gst_meta_api_type_register ("GstOdoMetaAPI", tags);
        g_once_init_leave (&type, _type);
    }
    return type;
}

static gboolean gst_odo_meta_init(GstMeta *meta, gpointer params, GstBuffer *buffer) {
    auto *gst_odo_meta = (GstOdoMeta *)meta;
    gst_odo_meta->detectionCount = 0;
    gst_odo_meta->inferenceInterval = 0;
    gst_odo_meta->isInferenceFrame = false;
    return TRUE;
}

const GstMetaInfo *gst_odo_meta_get_info (void) {
    static const GstMetaInfo *odo_meta_info = NULL;
    if (g_once_init_enter (& odo_meta_info)) {
        const GstMetaInfo *meta = gst_meta_register (GST_ODO_META_API_TYPE, "GstOdoMeta",
                                   sizeof (GstOdoMeta), (GstMetaInitFunction) gst_odo_meta_init,
                                   (GstMetaFreeFunction) NULL, gst_odo_meta_transform);
        g_once_init_leave (& odo_meta_info, meta);
    }
    return odo_meta_info;
}

static gboolean gst_odo_meta_transform(GstBuffer * dest_buf, GstMeta * meta,
                                       GstBuffer * src_buf, GQuark type, gpointer data) {
    GstOdoMeta *src_meta, *dest_meta;

    src_meta = (GstOdoMeta *) meta;
    dest_meta = (GstOdoMeta *) gst_buffer_add_meta (dest_buf, GST_ODO_META_INFO,NULL);

    if (!dest_meta)
        return FALSE;

    dest_meta->detections = src_meta->detections;
    dest_meta->detectionCount = src_meta->detectionCount;
    dest_meta->isInferenceFrame = src_meta->isInferenceFrame;
    dest_meta->inferenceInterval = src_meta->inferenceInterval;

    return TRUE;
}

void gst_odo_meta_free (GstMeta *meta, GstBuffer *buffer) {
    free(meta);
}

GstOdoMeta *gst_buffer_get_odo_meta (GstBuffer * buffer) {
    gpointer state = NULL;
    GstOdoMeta *odoMeta = NULL;
    GstMeta *meta;
    const GstMetaInfo *info = GST_ODO_META_INFO;

    while ((meta = gst_buffer_iterate_meta (buffer, &state))) {
        if (meta->info->api == info->api) {
            GstOdoMeta *vmeta = (GstOdoMeta *) meta;
/*            if (vmeta->id == 0)
                return vmeta;           // Early odoMeta for id 0 */
            if (odoMeta == nullptr)
                odoMeta = vmeta;
        }
    }
    return odoMeta;
}

GstOdoMeta * gst_odo_meta_add (GstBuffer * buffer, DetectionData *detections, ulong &detectCount,
                               gint inferenceInterval, gboolean isInferenceFrame) {
    GstOdoMeta *meta;

    meta = (GstOdoMeta *) gst_buffer_add_meta (buffer, GST_ODO_META_INFO, NULL);

    meta->detections = detections;
    meta->detectionCount = detectCount;
    meta->isInferenceFrame = isInferenceFrame;
    meta->inferenceInterval = inferenceInterval;

    return meta;
}
