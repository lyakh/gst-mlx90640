/* GStreamer
 * Copyright (C) 2021 Guennadi Liakhovetski <g.liakhovetski@gmx.de>
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
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef _GST_MLX90640_H_
#define _GST_MLX90640_H_

#include <gst/gst.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_MLX90640		(gst_mlx90640_get_type())
#define GST_MLX90640(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), GST_TYPE_MLX90640, \
					 GstMlx90640))
#define GST_MLX90640_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), GST_TYPE_MLX90640, \
					 GstMlx90640Class))
#define GST_IS_MLX90640(obj)		(G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_MLX90640))
#define GST_IS_MLX90640_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_MLX90640))

typedef struct _GstMlx90640 GstMlx90640;
typedef struct _GstMlx90640Class GstMlx90640Class;

struct _GstMlx90640
{
	GstVideoFilter element;
	GstPad *therm_pad;
	float t_min;
	float t_max;
	gint count;
};

struct _GstMlx90640Class
{
	GstVideoFilterClass parent_class;
};

typedef struct _GstMlx90640Meta {
	GstMeta meta;
	gfloat t_min;
	gfloat t_max;
} GstMlx90640Meta;

GST_VIDEO_API
GType gst_mlx90640_meta_api_get_type (void);

G_END_DECLS

#endif
