/*
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) YEAR AUTHOR_NAME AUTHOR_EMAIL
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <gst/video/video.h>
#include <dlfcn.h>
#include <utility>
#include <vector>
#include <cstdio>

#include "config.h"
#include "gstodotrack.h"
#include "../../gst-libs/odo-meta/odometa.h"
//#include "../../include/common.h"

GST_DEBUG_CATEGORY_STATIC (gst_odo_track_debug);
#define GST_CAT_DEFAULT gst_odo_track_debug

/* Filter signals and args */
enum {
    // gstreamer signals
    LAST_SIGNAL
};

#define DEFAULT_PROP_TRACKER_TYPE       "BOOSTING"
#define GST_OPENCV_TRACKER_ALGORITHM (tracker_algorithm_get_type ())

enum {
    PROP_0,
    PROP_TRACKER_TYPE,
    PROP_SILENT,
};

cv::Ptr<cv::legacy::MultiTracker> TRACKER;
ulong LAST_COUNT = 0;
DetectionData LAST_DETECTION[DETECTION_MAX] = {};

// the capabilities of the inputs and outputs.
static GstStaticPadTemplate sink_template =
        GST_STATIC_PAD_TEMPLATE ("sink",
                                 GST_PAD_SINK,
                                 GST_PAD_ALWAYS,
                                 GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ BGR }")));

static GstStaticPadTemplate src_template =
        GST_STATIC_PAD_TEMPLATE ("src",
                                 GST_PAD_SRC,
                                 GST_PAD_ALWAYS,
                                 GST_STATIC_CAPS (GST_VIDEO_CAPS_MAKE ("{ BGR }")));

#define gst_odo_track_parent_class parent_class

G_DEFINE_TYPE (GstOdoTrack, gst_odo_track, GST_TYPE_VIDEO_FILTER);

GST_ELEMENT_REGISTER_DEFINE (odo_track, "odo_track", GST_RANK_NONE, GST_TYPE_ODOTRACK);

static void gst_odo_track_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void gst_odo_track_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gboolean gst_odo_track_start(GstBaseTransform *btrans);

static gboolean gst_odo_track_stop(GstBaseTransform *btrans);

static GstFlowReturn gst_odo_track_transform_frame_ip(GstVideoFilter *vfilter, GstVideoFrame *frame);

static GType tracker_algorithm_get_type(void) {
    static GType algorithm = 0;
    static const GEnumValue algorithms[] = {{GST_OPENCV_TRACKER_ALGORITHM_BOOSTING,   "the Boosting tracker",                                    "Boosting"},
                                            {GST_OPENCV_TRACKER_ALGORITHM_CSRT,       "the CSRT tracker",                                        "CSRT"},
                                            {GST_OPENCV_TRACKER_ALGORITHM_KCF,        "the KCF (Kernelized Correlation Filter) tracker",         "KCF"},
                                            {GST_OPENCV_TRACKER_ALGORITHM_MEDIANFLOW, "the Median Flow tracker",                                 "MedianFlow"},
                                            {GST_OPENCV_TRACKER_ALGORITHM_MIL,        "the MIL tracker",                                         "MIL"},
                                            {GST_OPENCV_TRACKER_ALGORITHM_MOSSE,      "the MOSSE (Minimum Output Sum of Squared Error) tracker", "MOSSE"},
                                            {GST_OPENCV_TRACKER_ALGORITHM_TLD,        "the TLD (Tracking, learning and detection) tracker",      "TLD"},
                                            {0, NULL, NULL},};

    if (!algorithm) {
        algorithm = g_enum_register_static("GstOpenCVTrackerAlgorithm", algorithms);
    }
    return algorithm;
}

/* initialize the plugin's class */
static void gst_odo_track_class_init(GstOdoTrackClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GstElementClass *gstelement_class = (GstElementClass *) klass;
    GstBaseTransformClass *gstbasetransform_class = (GstBaseTransformClass *) klass;
    GstVideoFilterClass *gstvideofilter_class = (GstVideoFilterClass *) klass;

    GST_LOG_OBJECT (gstbasetransform_class, "Odo Track Class Init");

    parent_class = g_type_class_peek_parent (klass);

    gobject_class->set_property = gst_odo_track_set_property;
    gobject_class->get_property = gst_odo_track_get_property;

    gstbasetransform_class->start = GST_DEBUG_FUNCPTR (gst_odo_track_start);

    /*g_object_class_install_property(gobject_class, PROP_CUDA,
                                    g_param_spec_boolean("cuda", "Enable Cuda",
                                                         "Enable the use of Nvidia Cuda acceleration",
                                                         FALSE,
                                                         GParamFlags(G_PARAM_READABLE |
                                                                     G_PARAM_WRITABLE |
                                                                     G_PARAM_CONSTRUCT)));*/

    g_object_class_install_property(gobject_class, PROP_TRACKER_TYPE,
                                    g_param_spec_enum("type", "Tracker algorithm",
                                                      "Algorithm for tracking objects",
                                                      GST_OPENCV_TRACKER_ALGORITHM,
                                                      GST_OPENCV_TRACKER_ALGORITHM_MEDIANFLOW,
                                                      GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property(gobject_class, PROP_SILENT,
                                    g_param_spec_boolean("silent", "Silent",
                                                         "Produce verbose output ?",
                                                         FALSE,
                                                         GParamFlags(G_PARAM_READABLE |
                                                                     G_PARAM_WRITABLE |
                                                                     G_PARAM_CONSTRUCT)));

    gst_element_class_set_details_simple(gstelement_class,
                                         "OdoTrack",
                                         "Object Detection/Filter",
                                         "Odo Track plugin",
                                         "Chris Blasko cblasko@gmail.com");

    gst_element_class_add_static_pad_template(gstelement_class, &sink_template);
    gst_element_class_add_static_pad_template(gstelement_class, &src_template);

    gstbasetransform_class->transform_ip_on_passthrough = FALSE;
    //gstbasetransform_class->transform_ip = GST_DEBUG_FUNCPTR (gst_odo_track_transform_ip);
    gstvideofilter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_odo_track_transform_frame_ip);

    // debug category for filtering log messages
    GST_DEBUG_CATEGORY_INIT (gst_odo_track_debug, "odotrack", 0, "Odo Track plugin");

    gst_type_mark_as_plugin_api(GST_OPENCV_TRACKER_ALGORITHM, (GstPluginAPIFlags) 0);
}

/* initialize the new element
 * initialize instance structure
 */
static void gst_odo_track_init(GstOdoTrack *odo) {
    // We will not be generating a new buffer. Just adding/updating metadata.
    gst_base_transform_set_in_place(GST_BASE_TRANSFORM (odo), TRUE);
    odo->tracker_type = GST_OPENCV_TRACKER_ALGORITHM_MEDIANFLOW;
}

static cv::Ptr<cv::legacy::Tracker> create_cvtracker(GstOdoTrack *odo) {
    cv::Ptr<cv::legacy::Tracker> tracker;
    switch (odo->tracker_type) {
        case GST_OPENCV_TRACKER_ALGORITHM_BOOSTING:
            tracker = cv::legacy::TrackerBoosting::create();
            break;
        case GST_OPENCV_TRACKER_ALGORITHM_CSRT:
            tracker = cv::legacy::TrackerCSRT::create();
            break;
        case GST_OPENCV_TRACKER_ALGORITHM_KCF:
            tracker = cv::legacy::TrackerKCF::create();
            break;
        case GST_OPENCV_TRACKER_ALGORITHM_MEDIANFLOW:
            tracker = cv::legacy::TrackerMedianFlow::create();
            break;
        case GST_OPENCV_TRACKER_ALGORITHM_MIL:
            tracker = cv::legacy::TrackerMIL::create();
            break;
        case GST_OPENCV_TRACKER_ALGORITHM_MOSSE:
            tracker = cv::legacy::TrackerMOSSE::create();
            break;
        case GST_OPENCV_TRACKER_ALGORITHM_TLD:
            tracker = cv::legacy::TrackerTLD::create();
            break;
    }
    return tracker;
}

/**
 * Initialize all resources and start the output thread
 */
static gboolean gst_odo_track_start(GstBaseTransform *base) {
    //GstOdoTrack *odo = GST_ODOTRACK (base);
    TRACKER = cv::legacy::MultiTracker::create();
    return TRUE;
}

static void gst_odo_track_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    GstOdoTrack *filter = GST_ODOTRACK(object);

    switch (prop_id) {
        case PROP_0:
            break;
        case PROP_TRACKER_TYPE:
            filter->tracker_type = g_value_get_enum(value);
            break;
/*        case PROP_CUDA:
            filter->cuda = g_value_get_boolean(value);
            break;*/
        case PROP_SILENT:
            filter->silent = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void gst_odo_track_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    GstOdoTrack *filter = GST_ODOTRACK(object);

    switch (prop_id) {
        case PROP_0:
            break;
        case PROP_TRACKER_TYPE:
            g_value_set_enum(value, filter->tracker_type);
            break;
/*        case PROP_CUDA:
            g_value_set_boolean(value, filter->cuda);
            break;*/
        case PROP_SILENT:
            g_value_set_boolean(value, filter->silent);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

/* GstVideoFilter method implementations */

// this function does the actual processing
static GstFlowReturn gst_odo_track_transform_frame_ip(GstVideoFilter *vfilter, GstVideoFrame *frame) {
    GstOdoTrack *odo = GST_ODOTRACK(vfilter);

    GST_LOG_OBJECT (vfilter, "Transforming in-place");

    frame->buffer = gst_buffer_make_writable (frame->buffer);
    GstOdoMeta *meta = gst_buffer_get_odo_meta(frame->buffer);

    GstMapInfo map;
    gst_buffer_map(frame->buffer, &map, GST_MAP_WRITE);
    auto img_width = GST_VIDEO_FRAME_WIDTH (frame);
    auto img_height = GST_VIDEO_FRAME_HEIGHT (frame);

    cv::Mat img = cv::Mat(cv::Size(img_width, img_height), CV_8UC3, map.data);

    if (meta->isInferenceFrame) {
        LAST_COUNT = meta->detectionCount;
        TRACKER->clear();
        TRACKER = cv::legacy::MultiTracker::create();
        for (int i=0; i<LAST_COUNT; i++) {
            LAST_DETECTION[i].class_id = meta->detections[i].class_id;
            LAST_DETECTION[i].confidence = meta->detections[i].confidence;
            strcpy(LAST_DETECTION[i].label, meta->detections[i].label);
            cv::Rect2i rect = cv::Rect2i(meta->detections[i].box.x, meta->detections[i].box.y,
                                         meta->detections[i].box.width, meta->detections[i].box.height);
            TRACKER->add(create_cvtracker(odo), img, rect);
        }
        GST_DEBUG("Added %zu objects to tracker", LAST_COUNT);
    }
    else {
        meta->detectionCount = LAST_COUNT;

        std::vector<cv::Rect2d> tracked;
        if (!TRACKER->update(img, tracked)) GST_ERROR("Error tracking objects");

        GST_DEBUG("Infer count=%lu, tracked count=%zu", LAST_COUNT, tracked.size());

        for (int i=0; i<tracked.size(); i++) {
            meta->detections[i].box.x = tracked[i].x;
            meta->detections[i].box.y = tracked[i].y;
            meta->detections[i].box.width = tracked[i].width;
            meta->detections[i].box.height = tracked[i].height;
            meta->detections[i].class_id = LAST_DETECTION[i].class_id;
            meta->detections[i].confidence = LAST_DETECTION[i].confidence;
            strcpy(meta->detections[i].label, LAST_DETECTION[i].label);
        }
    }

    return GST_FLOW_OK;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean plugin_init(GstPlugin *odotrack) {
    return GST_ELEMENT_REGISTER (odo_track, odotrack);
}

// gstreamer looks for this structure to register detection-libs
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, odotrack, "odo_track", plugin_init, PACKAGE_VERSION,
                   GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)