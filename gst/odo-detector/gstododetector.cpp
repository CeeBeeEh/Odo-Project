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

/**
 * SECTION:element-plugin
 *
 * FIXME:Describe plugin here.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch -v -m fakesrc ! plugin ! fakesink silent=TRUE
 * ]|
 * </refsect2>
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
#include <opencv4/opencv2/core/types.hpp>

#include "config.h"
#include "gstododetector.h"
#include "../../gst-libs/odo-meta/odometa.h"
#include "../../include/common.h"

GST_DEBUG_CATEGORY_STATIC (gst_odo_detector_debug);
#define GST_CAT_DEFAULT gst_odo_detector_debug

void (*odo_load_ptr)(bool, float, float, int, int, const char*, const char*, const char*);
void (*odo_detect_ptr)(ulong&, DetectionData*, cv::Mat);
void* handle = nullptr;
const char* error_message = nullptr;

gint INFERENCE_COUNT = 0;

/* Filter signals and args */
enum {
    // gstreamer signals
    LAST_SIGNAL
};

#define DEFAULT_PROP_WIDTH 	416
#define DEFAULT_PROP_HEIGHT	416
#define DEFAULT_PROP_INFERENCE_INTERVAL 15
#define DEFAULT_PROP_CONFIDENCE_THRESHOLD	0.3
#define DEFAULT_PROP_NMS_THRESHOLD	0.4
#define DEFAULT_PROP_LIB_PATH       "libOdoLib_OpenCV_Yolo.so"
#define DEFAULT_PROP_NET_CLASSES	"classes.txt"
#define DEFAULT_PROP_NET_CONFIG 	"yolo.cfg"
#define DEFAULT_PROP_NET_WEIGHTS     "yolov4.weights"

enum {
    PROP_0,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_INFERENCE_INTERVAL,
    PROP_CONFIDENCE_THRESHOLD,
    PROP_NMS_THRESHOLD,
    PROP_CUDA,
    PROP_LIB_PATH,
    PROP_NET_CLASSES,
    PROP_NET_CONFIG,
    PROP_NET_WEIGHTS,
    PROP_SILENT,
};

// { RGBA, BGRA, RGB, BGR, I420 }
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

#define gst_odo_detector_parent_class parent_class

G_DEFINE_TYPE (GstOdoDetector, gst_odo_detector, GST_TYPE_VIDEO_FILTER);

GST_ELEMENT_REGISTER_DEFINE (odo_detector, "odo_detector", GST_RANK_NONE, GST_TYPE_ODODETECTOR);

static void gst_odo_detector_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void gst_odo_detector_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gboolean gst_odo_detector_start(GstBaseTransform *btrans);

static gboolean gst_odo_detector_stop(GstBaseTransform *btrans);

static GstFlowReturn gst_odo_detector_transform_frame_ip(GstVideoFilter *vfilter, GstVideoFrame *gstFrame);

/* initialize the plugin's class */
static void gst_odo_detector_class_init(GstOdoDetectorClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GstElementClass *gstelement_class = (GstElementClass *) klass;
    GstBaseTransformClass *gstbasetransform_class = (GstBaseTransformClass *) klass;
    GstVideoFilterClass *gstvideofilter_class = (GstVideoFilterClass *) klass;

    GST_LOG_OBJECT (gstbasetransform_class, "Odo Detector Class Init");

    parent_class = g_type_class_peek_parent (klass);

    gobject_class->set_property = gst_odo_detector_set_property;
    gobject_class->get_property = gst_odo_detector_get_property;

    gstbasetransform_class->start = GST_DEBUG_FUNCPTR (gst_odo_detector_start);

    g_object_class_install_property (G_OBJECT_CLASS (klass), PROP_WIDTH,
                                     g_param_spec_int ("width", "model width",
                                                        "Width of model input size", 32, 4096,
                                                        DEFAULT_PROP_WIDTH,
                                                       GParamFlags(G_PARAM_READABLE |
                                                                   G_PARAM_WRITABLE |
                                                                   G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_HEIGHT,
                                     g_param_spec_int ("height", "model height",
                                                       "Height of model input size", 32, 4096,
                                                       DEFAULT_PROP_HEIGHT,
                                                       GParamFlags(G_PARAM_READABLE |
                                                                   G_PARAM_WRITABLE |
                                                                   G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_INFERENCE_INTERVAL,
                                     g_param_spec_int ("interval", "inference interval",
                                                       "Height of model input size", 0, 255,
                                                       DEFAULT_PROP_INFERENCE_INTERVAL,
                                                       GParamFlags(G_PARAM_READABLE |
                                                                   G_PARAM_WRITABLE |
                                                                   G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_CONFIDENCE_THRESHOLD,
                                     g_param_spec_float("confidence", "confidence threshold",
                                                        "Confidence of detected objects", 0.05, 1,
                                                        DEFAULT_PROP_CONFIDENCE_THRESHOLD,
                                                        GParamFlags(G_PARAM_READABLE |
                                                                    G_PARAM_WRITABLE |
                                                                    G_PARAM_CONSTRUCT)));

    g_object_class_install_property (gobject_class, PROP_NMS_THRESHOLD,
                                     g_param_spec_float("nms", "nms threshold",
                                                         "Non-max suppression threshold", 0.05, 1,
                                                         DEFAULT_PROP_NMS_THRESHOLD,
                                                        GParamFlags(G_PARAM_READABLE |
                                                                    G_PARAM_WRITABLE |
                                                                    G_PARAM_CONSTRUCT)));

    g_object_class_install_property(gobject_class, PROP_CUDA,
                                    g_param_spec_boolean("cuda", "Enable Cuda",
                                                         "Enable the use of Nvidia Cuda acceleration",
                                                         FALSE,
                                                         GParamFlags(G_PARAM_READABLE |
                                                                     G_PARAM_WRITABLE |
                                                                     G_PARAM_CONSTRUCT)));

    g_object_class_install_property(gobject_class, PROP_LIB_PATH,
                                    g_param_spec_string("lib_path", "Library path",
                                                      "Location of the detection library",
                                                      DEFAULT_PROP_LIB_PATH,
                                                      GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property(gobject_class, PROP_NET_CLASSES,
                                    g_param_spec_string("net_classes", "Classes list",
                                                      "Location of the file containing the list of classes",
                                                      DEFAULT_PROP_NET_CLASSES,
                                                      GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property(gobject_class, PROP_NET_CONFIG,
                                    g_param_spec_string("net_config", "Config file",
                                                      "Location of the yolo config file",
                                                      DEFAULT_PROP_NET_CONFIG,
                                                      GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property(gobject_class, PROP_NET_WEIGHTS,
                                    g_param_spec_string("net_weights", "Weights file",
                                                      "Location of the yolo weights file",
                                                      DEFAULT_PROP_NET_WEIGHTS,
                                                      GParamFlags(G_PARAM_READWRITE | G_PARAM_CONSTRUCT)));

    g_object_class_install_property(gobject_class, PROP_SILENT,
                                    g_param_spec_boolean("silent", "Silent",
                                                         "Produce verbose output ?",
                                                         FALSE,
                                                         GParamFlags(G_PARAM_READABLE |
                                                                     G_PARAM_WRITABLE |
                                                                     G_PARAM_CONSTRUCT)));

    gst_element_class_set_details_simple(gstelement_class,
                                         "OdoDetector",
                                         "Object Detection/Filter",
                                         "Odo Detector plugin",
                                         "Chris Blasko cblasko@gmail.com");

    gst_element_class_add_static_pad_template(gstelement_class, &sink_template);
    gst_element_class_add_static_pad_template(gstelement_class, &src_template);

    gstbasetransform_class->transform_ip_on_passthrough = FALSE;
    //gstbasetransform_class->transform_ip = GST_DEBUG_FUNCPTR (gst_odo_detector_transform_ip);
    gstvideofilter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_odo_detector_transform_frame_ip);

    // debug category for filtering log messages
    GST_DEBUG_CATEGORY_INIT (gst_odo_detector_debug, "ododetector", 0, "Odo Detector plugin");
}

/* initialize the new element
 * initialize instance structure
 */
static void gst_odo_detector_init(GstOdoDetector *odo) {
    GstBaseTransform *base = GST_BASE_TRANSFORM (odo);
    // We will not be generating a new buffer. Just adding/updating metadata.
    gst_base_transform_set_in_place(base, TRUE);
}

/**
 * Initialize all resources and start the output thread
 */
static gboolean gst_odo_detector_start(GstBaseTransform *base) {
    GstOdoDetector *odo = GST_ODODETECTOR (base);

    fprintf(stdout, "Setting property WIDTH=%d\n", odo->width);
    fprintf(stdout, "Setting property HEIGHT=%d\n", odo->height);
    fprintf(stdout, "Setting property INFERENCE_INTERVAL=%d\n", odo->inference_interval);
    fprintf(stdout, "Setting property CONFIDENCE_THRESHOLD=%f\n", odo->confidence_threshold);
    fprintf(stdout, "Setting property NMS_THRESHOLD=%f\n", odo->nms_threshold);
    fprintf(stdout, "Setting property LIB_PATH=%s\n", odo->lib_path.c_str());
    fprintf(stdout, "Setting property NET_CLASSES=%s\n", odo->net_classes.c_str());
    fprintf(stdout, "Setting property NET_CONFIG=%s\n", odo->net_config.c_str());
    fprintf(stdout, "Setting property NET_WEIGHTS=%s\n", odo->net_weights.c_str());

    // on error dlopen returns NULL
    GST_LOG_OBJECT (base, "Opening library");
    handle = dlopen( odo->lib_path.c_str(), RTLD_LAZY );

    // check for error, if it is NULL
    if( !handle ) {
        GST_LOG_OBJECT (base, "Unable to open library <lib>, error is %s\n", dlerror());
        return FALSE;
    }
    GST_LOG_OBJECT (base, "Library successfully opened");

    GST_LOG_OBJECT (base, "Linking function pointers");
    odo_load_ptr = (void (*)(bool, float, float, int, int, const char*, const char*, const char*))
            dlsym( handle, "c_odo_load" );
    odo_detect_ptr = (void (*)(ulong&, DetectionData*, cv::Mat)) dlsym(handle, "c_odo_detect" );
    GST_LOG_OBJECT (base, "Function pointers successfully linked");

    fprintf(stdout, "Network params width=%d height=%d\n", odo->width, odo->height);
    GST_LOG_OBJECT (base, "Calling library load function");
    (*odo_load_ptr)(odo->cuda, odo->confidence_threshold, odo->nms_threshold, odo->width, odo->height,
                    odo->net_classes.c_str(), odo->net_config.c_str(), odo->net_weights.c_str());

    return TRUE;
}

static void gst_odo_detector_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    GstOdoDetector *filter = GST_ODODETECTOR(object);

    switch (prop_id) {
        case PROP_0:
            break;
        case PROP_WIDTH:
            filter->width = g_value_get_int(value);
            break;
        case PROP_HEIGHT:
            filter->height = g_value_get_int(value);
            break;
        case PROP_INFERENCE_INTERVAL:
            filter->inference_interval = g_value_get_int(value);
            break;
        case PROP_CONFIDENCE_THRESHOLD:
            filter->confidence_threshold = g_value_get_float(value);
            break;
        case PROP_NMS_THRESHOLD:
            filter->nms_threshold = g_value_get_float(value);
            break;
        case PROP_LIB_PATH:
            filter->lib_path = g_value_get_string(value);
            break;
        case PROP_NET_CLASSES:
            filter->net_classes = g_value_get_string(value);
            break;
        case PROP_NET_CONFIG:
            filter->net_config = g_value_get_string(value);
            break;
        case PROP_NET_WEIGHTS:
            filter->net_weights = g_value_get_string(value);
            break;
        case PROP_CUDA:
            filter->cuda = g_value_get_boolean(value);
            break;
        case PROP_SILENT:
            filter->silent = g_value_get_boolean(value);
            break;
        default:
            G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
            break;
    }
}

static void gst_odo_detector_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    GstOdoDetector *filter = GST_ODODETECTOR(object);

    switch (prop_id) {
        case PROP_0:
            break;
        case PROP_WIDTH:
            g_value_set_int(value, filter->width);
            break;
        case PROP_HEIGHT:
            g_value_set_int(value, filter->height);
            break;
        case PROP_INFERENCE_INTERVAL:
            g_value_set_int(value, filter->inference_interval);
            break;
        case PROP_CONFIDENCE_THRESHOLD:
            g_value_set_double(value, filter->confidence_threshold);
            break;
        case PROP_NMS_THRESHOLD:
            g_value_set_double(value, filter->nms_threshold);
            break;
        case PROP_LIB_PATH:
            g_value_set_string(value, filter->lib_path.c_str());
            break;
        case PROP_NET_CLASSES:
            g_value_set_string(value, filter->net_classes.c_str());
            break;
        case PROP_NET_CONFIG:
            g_value_set_string(value, filter->net_config.c_str());
            break;
        case PROP_NET_WEIGHTS:
            g_value_set_string(value, filter->net_weights.c_str());
            break;
        case PROP_CUDA:
            g_value_set_boolean(value, filter->cuda);
            break;
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
static GstFlowReturn gst_odo_detector_transform_frame_ip(GstVideoFilter *vfilter, GstVideoFrame *gstFrame) {
    GstOdoDetector *filter = GST_ODODETECTOR(vfilter);

    GST_LOG_OBJECT (vfilter, "Transforming in-place");

/*    if (GST_CLOCK_TIME_IS_VALID (GST_BUFFER_TIMESTAMP(outbuf)))
        gst_object_sync_values(GST_OBJECT (filter), GST_BUFFER_TIMESTAMP (outbuf));*/

    try {
        GstOdoMeta *meta;
        gstFrame->buffer = gst_buffer_make_writable (gstFrame->buffer);
        meta = GST_ODO_META_ADD(gstFrame->buffer);
        meta->inferenceInterval = filter->inference_interval;  // is this needed?

        static DetectionData detectData[DETECTION_MAX] = {};
        ulong detectCount = 0;

        GstBuffer *buffer = gst_buffer_copy_deep(gstFrame->buffer);
        GstMapInfo map;
        gst_buffer_map(buffer, &map, GST_MAP_WRITE);


        cv::Mat frame = cv::Mat(cv::Size(GST_VIDEO_FRAME_WIDTH (gstFrame),
                                       GST_VIDEO_FRAME_HEIGHT (gstFrame)),
                              GST_VIDEO_FRAME_FORMAT (gstFrame),
                              map.data);

        if (INFERENCE_COUNT == 0) {
            meta->isInferenceFrame = true;
            (*odo_detect_ptr)(detectCount, detectData, frame);
        }
        else {
            meta->isInferenceFrame = false;
        }

        meta->detections = detectData;
        meta->detectionCount = detectCount;

        if (INFERENCE_COUNT >= filter->inference_interval)
            INFERENCE_COUNT = 0;
        else
            INFERENCE_COUNT++;
    }
    catch (const std::exception &e) {
        GST_ERROR ("Error with detection, %s", e.what());
    }

    return GST_FLOW_OK;
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean plugin_init(GstPlugin *ododetector) {
    return GST_ELEMENT_REGISTER (odo_detector, ododetector);
}

// gstreamer looks for this structure to register detection-libs
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, ododetector, "odo_detector", plugin_init, PACKAGE_VERSION,
                   GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)