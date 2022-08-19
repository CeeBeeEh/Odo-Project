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

#ifndef __GST_ODOVIZ_H__
#define __GST_ODOVIZ_H__

#include <gst/gst.h>
#include <gst/video/gstvideofilter.h>
#include "../../gst-libs/odo-meta/odometa.h"

G_BEGIN_DECLS

#define GST_TYPE_ODO_VIZ (gst_odo_viz_get_type())

G_DECLARE_FINAL_TYPE (GstOdoViz, gst_odo_viz, GST, ODOVIZ, GstVideoFilter)

struct _GstOdoViz {
    GstVideoFilter parent;
    gint width;
    gint height;
    gfloat confidence_threshold;
    gfloat nms_threshold;
    gboolean cuda;
    gboolean silent;
};

void DrawRectangles(guint8 *img, gint img_width, gint img_height, GstOdoMeta *odoMeta);

G_END_DECLS

#endif /* __GST_ODOVIZ_H__ */
