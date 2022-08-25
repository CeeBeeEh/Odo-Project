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

#ifndef __GST_ODOTRACK_H__
#define __GST_ODOTRACK_H__

#include <gst/gst.h>
#include <gst/video/gstvideofilter.h>
#include <string>
#include <opencv2/opencv.hpp>
#include <opencv2/tracking.hpp>
#include <opencv2/tracking/tracking_legacy.hpp>

#include "../../include/DetectionData.h"
#include "../../include/common.h"

G_BEGIN_DECLS

#define GST_TYPE_ODOTRACK (gst_odo_track_get_type())

G_DECLARE_FINAL_TYPE (GstOdoTrack, gst_odo_track, GST, ODOTRACK, GstVideoFilter)

typedef enum {
    GST_OPENCV_TRACKER_ALGORITHM_BOOSTING,
    GST_OPENCV_TRACKER_ALGORITHM_CSRT,
    GST_OPENCV_TRACKER_ALGORITHM_KCF,
    GST_OPENCV_TRACKER_ALGORITHM_MEDIANFLOW,
    GST_OPENCV_TRACKER_ALGORITHM_MIL,
    GST_OPENCV_TRACKER_ALGORITHM_MOSSE,
    GST_OPENCV_TRACKER_ALGORITHM_TLD,
} GstOpenCVTrackerAlgorithm;

struct _GstOdoTrack {
    GstVideoFilter parent;
    gint tracker_type;
    gboolean silent;
};

G_END_DECLS

#endif /* __GST_ODOTRACK_H__ */
