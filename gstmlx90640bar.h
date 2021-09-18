/* GStreamer
 * Copyright (C) 2021 FIXME <g.liakhovetski@gmx.de>
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

#ifndef _GST_MLX90640_BAR_H_
#define _GST_MLX90640_BAR_H_

#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>

G_BEGIN_DECLS

#define GST_TYPE_MLX90640_BAR		(gst_mlx90640_bar_get_type())
#define GST_MLX90640_BAR(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), \
							GST_TYPE_MLX90640_BAR,GstMlx90640Bar))
#define GST_MLX90640_BAR_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST((klass), \
							GST_TYPE_MLX90640_BAR,GstMlx90640BarClass))
#define GST_IS_MLX90640_BAR(obj)	(G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_TYPE_MLX90640_BAR))
#define GST_IS_MLX90640_BAR_CLASS(obj)	(G_TYPE_CHECK_CLASS_TYPE((klass), GST_TYPE_MLX90640_BAR))

typedef struct _GstMlx90640Bar GstMlx90640Bar;
typedef struct _GstMlx90640BarClass GstMlx90640BarClass;

struct _GstMlx90640Bar
{
	GstVideoFilter element;
	gint frame_count;
};

struct _GstMlx90640BarClass
{
	GstVideoFilterClass parent_class;
};

GType gst_mlx90640_bar_get_type(void);

G_END_DECLS

#endif
