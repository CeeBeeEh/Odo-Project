/* 
 * GStreamer
 * Copyright (C) 2006 Stefan Kost <ensonic@users.sf.net>
 * Copyright (C) 2020 Niels De Graef <niels.degraef@gmail.com>
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

#ifndef __GST_ODODETECTOR_H__
#define __GST_ODODETECTOR_H__

#include <gst/gst.h>
#include <gst/video/gstvideofilter.h>
#include <string>

G_BEGIN_DECLS

#define GST_TYPE_ODODETECTOR (gst_odo_detector_get_type())

G_DECLARE_FINAL_TYPE (GstOdoDetector, gst_odo_detector, GST, ODODETECTOR, GstVideoFilter)

struct _GstOdoDetector {
    GstVideoFilter parent;
    gint width;
    gint height;
    gfloat confidence_threshold;
    gfloat nms_threshold;
    gboolean cuda;
    std::string lib_path;
    std::string net_classes;
    std::string net_config;
    std::string net_weights;
    gboolean silent;
};

G_END_DECLS

#endif /* __GST_ODODETECTOR_H__ */
