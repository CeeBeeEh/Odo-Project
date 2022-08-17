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
#include <vector>
#include <cstdio>
#include <string>
#include <opencv2/opencv.hpp>

#include "gstodoosd.h"
#include "config.h"

GST_DEBUG_CATEGORY_STATIC (gst_odo_osd_debug);
#define GST_CAT_DEFAULT gst_odo_osd_debug

/* Filter signals and args */
enum {
    // gstreamer signals
    SIGNAL_ODO_CONFIDENCE,
    SIGNAL_ODO_NMS,
    SIGNAL_ODO_WIDTH,
    SIGNAL_ODO_HEIGHT,
    LAST_SIGNAL
};


#define DEFAULT_PROP_WIDTH 	416
#define DEFAULT_PROP_HEIGHT	416
#define DEFAULT_PROP_CONFIDENCE_THRESHOLD	0.3
#define DEFAULT_PROP_NMS_THRESHOLD	0.4

#define NUM_COLOURS 80

enum {
    PROP_0,
    PROP_WIDTH,
    PROP_HEIGHT,
    PROP_CONFIDENCE_THRESHOLD,
    PROP_NMS_THRESHOLD,
    PROP_CUDA,
    PROP_SILENT,
};

cv::Scalar colours[NUM_COLOURS];

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

#define gst_odo_osd_parent_class parent_class

G_DEFINE_TYPE (GstOdoOsd, gst_odo_osd, GST_TYPE_VIDEO_FILTER);

GST_ELEMENT_REGISTER_DEFINE (odo_osd, "odo_osd", GST_RANK_NONE, GST_TYPE_ODO_OSD);

static void gst_odo_osd_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec);

static void gst_odo_osd_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec);

static gboolean gst_odo_osd_start(GstBaseTransform *btrans);

static gboolean gst_odo_osd_stop(GstBaseTransform *btrans);

static GstFlowReturn gst_odo_osd_transform_ip(GstVideoFilter *base, GstVideoFrame *outbuf);

/* initialize the plugin's class */
static void gst_odo_osd_class_init(GstOdoOsdClass *klass) {
    GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
    GstElementClass *gstelement_class = (GstElementClass *) klass;
    GstBaseTransformClass *gstbasetransform_class = (GstBaseTransformClass *) klass;
    GstVideoFilterClass *gstvideofilter_class = (GstVideoFilterClass *) klass;

    GST_LOG_OBJECT (gstbasetransform_class, "Odo Osd Class Init");

    parent_class = g_type_class_peek_parent (klass);

    gobject_class->set_property = gst_odo_osd_set_property;
    gobject_class->get_property = gst_odo_osd_get_property;

    gstbasetransform_class->start = GST_DEBUG_FUNCPTR (gst_odo_osd_start);

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

    g_object_class_install_property(gobject_class, PROP_SILENT,
                                    g_param_spec_boolean("silent", "Silent",
                                                         "Produce verbose output ?",
                                                         FALSE,
                                                         GParamFlags(G_PARAM_READABLE |
                                                                     G_PARAM_WRITABLE |
                                                                     G_PARAM_CONSTRUCT)));

    gst_element_class_set_details_simple(gstelement_class,
                                         "OdoOsd",
                                         "Object Box Display Drawing",
                                         "Odo OSD plugin",
                                         "Chris Blasko cblasko@gmail.com");

    gst_element_class_add_static_pad_template(gstelement_class, &sink_template);
    gst_element_class_add_static_pad_template(gstelement_class, &src_template);

    gstbasetransform_class->transform_ip_on_passthrough = FALSE;
    gstvideofilter_class->transform_frame_ip = GST_DEBUG_FUNCPTR (gst_odo_osd_transform_ip);

    // debug category for filtering log messages
    GST_DEBUG_CATEGORY_INIT (gst_odo_osd_debug, "odoosd", 0, "Odo OSD plugin");
}

/* initialize the new element
 * initialize instance structure
 */
static void gst_odo_osd_init(GstOdoOsd *odo) {
    GstBaseTransform *base = GST_BASE_TRANSFORM (odo);
    // We will not be generating a new buffer. Just adding/updating metadata.
    gst_base_transform_set_in_place(base, TRUE);
    // We do not want to change the input caps. Set to passthrough. transform_ip is still called.
    //gst_base_transform_set_passthrough(GST_BASE_TRANSFORM (base), TRUE);
}

/**
 * Initialize all resources and start the output thread
 */
static gboolean gst_odo_osd_start(GstBaseTransform *base) {
    GstOdoOsd *odo = GST_ODOOSD (base);

    for (auto & colour : colours) {
        colour = cv::Scalar((double)std::rand() / RAND_MAX * 255,
                            (double)std::rand() / RAND_MAX * 255,
                            (double)std::rand() / RAND_MAX * 255);
    }

    return TRUE;
}

static void gst_odo_osd_set_property(GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec) {
    GstOdoOsd *filter = GST_ODOOSD(object);

    switch (prop_id) {
        case PROP_0:
            break;
        case PROP_WIDTH:
            filter->width = g_value_get_int(value);
            break;
        case PROP_HEIGHT:
            filter->height = g_value_get_int(value);
            break;
        case PROP_CONFIDENCE_THRESHOLD:
            filter->confidence_threshold = g_value_get_float(value);
            break;
        case PROP_NMS_THRESHOLD:
            filter->nms_threshold = g_value_get_float(value);
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

static void gst_odo_osd_get_property(GObject *object, guint prop_id, GValue *value, GParamSpec *pspec) {
    GstOdoOsd *filter = GST_ODOOSD(object);

    switch (prop_id) {
        case PROP_0:
            break;
        case PROP_WIDTH:
            g_value_set_int(value, filter->width);
            break;
        case PROP_HEIGHT:
            g_value_set_int(value, filter->height);
            break;
        case PROP_CONFIDENCE_THRESHOLD:
            g_value_set_double(value, filter->confidence_threshold);
            break;
        case PROP_NMS_THRESHOLD:
            g_value_set_double(value, filter->nms_threshold);
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
static GstFlowReturn gst_odo_osd_transform_ip(GstVideoFilter *vfilter, GstVideoFrame *frame) {
    GstOdoOsd *filter = GST_ODOOSD(vfilter);

    GST_LOG_OBJECT (vfilter, "Transforming in-place");

    GstOdoMeta *meta = gst_buffer_get_odo_meta(frame->buffer);

    GstMapInfo map;
    gst_buffer_map(frame->buffer, &map, GST_MAP_WRITE);
    auto* img = map.data;
    auto img_width = GST_VIDEO_FRAME_WIDTH (frame);
    auto img_height = GST_VIDEO_FRAME_HEIGHT (frame);
    auto img_format = GST_VIDEO_FRAME_FORMAT (frame);

    DrawRectangles(img, img_width, img_height, meta);

    map.data = img;

    return GST_FLOW_OK;
}

template <typename T>
std::string numToStr(T value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

void DrawRectangles(guint8 *img, gint img_width, gint img_height, GstOdoMeta *odoMeta) {
    cv::Mat frame = cv::Mat(cv::Size(img_width, img_height), CV_8UC3, img);
    if (odoMeta->detectionCount <= 0)
        return;

    for (ulong i=0; i<odoMeta->detectionCount; i++) {
        try {
            DetectionData detection = odoMeta->detections[i];
            gint lineDefault = 2;
            cv::Scalar white = cv::Scalar(255,255,255);
            cv::Scalar colourDefault = cv::Scalar(200,170,125);
            std::string text;
            gdouble fontScale = 1;
            int font = cv::FONT_HERSHEY_PLAIN;

            std::string dbgText = detection.label;
            dbgText.append("_" + numToStr(round( detection.confidence * 100.0 ) / 100.0));

            // [0] is x, [1] is y
            cv::Rect detectionRect = cv::Rect(detection.box.top_left[0],
                                              detection.box.top_left[1],
                                              detection.box.width,
                                              detection.box.height);
            //cv::Point topLeft = {odoMeta->detections[i].box.top_left[0], odoMeta->detections[i].box.top_left[1]};
            //cv::Point bottomRight = {odoMeta->detections[i].box.bottom_right[0], odoMeta->detections[i].box.bottom_right[1]};




            cv::rectangle(frame, detectionRect, colours[detection.class_id], lineDefault);

            cv::Size textSize = cv::getTextSize(dbgText, font, fontScale,
                                                lineDefault, nullptr);

            cv::Rect textRect = cv::Rect(detection.box.top_left[0] +10,
                                         detection.box.top_left[1] +26,
                                         detection.box.width +20,
                                         detection.box.height +20);
            cv::Point leftRect = cv::Point(detection.box.top_left[0],
                                           detection.box.bottom_right[1]);
            cv::Point leftText = cv::Point(detection.box.top_left[0],
                                           detection.box.bottom_right[1] + 15);

            cv::Point textPoint = cv::Point(leftRect.x + textSize.width + 20,
                                            leftRect.y + textSize.height + 20);
            //cv::rectangle(frame, textRect, colourDefault, -1);
            cv::putText(frame, dbgText, leftText, font, fontScale, white,
                        1,cv::LINE_4, false);
        }
        catch (std::exception &e) {
            fprintf (stderr, "Error with DrawRectangles: %s", e.what());
        }
    }
}

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean plugin_init(GstPlugin *odoosd) {
    return GST_ELEMENT_REGISTER (odo_osd, odoosd);
}

// gstreamer looks for this structure to register plugins
GST_PLUGIN_DEFINE (GST_VERSION_MAJOR, GST_VERSION_MINOR, odoosd, "odo_osd", plugin_init, PACKAGE_VERSION,
                   GST_LICENSE, GST_PACKAGE_NAME, GST_PACKAGE_ORIGIN)