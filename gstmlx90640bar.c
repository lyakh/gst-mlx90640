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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstmlx90640bar
 *
 * The mlx90640bar element does FIXME stuff.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! mlx90640bar ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#define BORDER_BAR

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>

#include <gst/gst.h>
#include <gst/gstmeta.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "gstmlx90640.h"
#include "gstmlx90640bar.h"

GST_DEBUG_CATEGORY_STATIC(gst_mlx90640_bar_debug);
#define GST_CAT_DEFAULT gst_mlx90640_bar_debug

/* prototypes */

static void gst_mlx90640_bar_set_property(GObject *object, guint property_id,
					  const GValue *value, GParamSpec *pspec);
static void gst_mlx90640_bar_get_property(GObject *object, guint property_id,
					  GValue *value, GParamSpec *pspec);
static void gst_mlx90640_bar_finalize(GObject *object);

static GstFlowReturn gst_mlx90640_bar_transform_frame(GstVideoFilter *filter,
						      GstVideoFrame *inframe, GstVideoFrame *outframe);
static GstFlowReturn gst_mlx90640_bar_transform_frame_ip(GstVideoFilter *filter,
							 GstVideoFrame *frame);
static GstCaps *gst_mlx90640_bar_transform_caps(GstBaseTransform *trans, GstPadDirection direction,
						GstCaps *caps, GstCaps *filter);

enum {
	PROP_0
};

/* pad templates */

#define VIDEO_SRC_CAPS GST_VIDEO_CAPS_MAKE("{ RGBA }")
#define VIDEO_SINK_CAPS GST_VIDEO_CAPS_MAKE("{ RGBA }")

/* class initialization */

#define gst_mlx90640_bar_parent_class parent_class
G_DEFINE_TYPE(GstMlx90640Bar, gst_mlx90640_bar, GST_TYPE_VIDEO_FILTER);
GST_ELEMENT_REGISTER_DEFINE_WITH_CODE(mlx90640bar, "mlx90640_bar", GST_RANK_NONE,
			GST_TYPE_MLX90640_BAR,
			GST_DEBUG_CATEGORY_INIT(gst_mlx90640_bar_debug,
						"mlx90640_bar", 0,
						"debug category for mlx90640_bar element"));

/* The capabilities of the inputs and outputs. */

static GstStaticPadTemplate gst_mlx90640_bar_sink_template =
	GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
				GST_STATIC_CAPS(VIDEO_SINK_CAPS));

static GstStaticPadTemplate gst_mlx90640_bar_src_template =
	GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
				GST_STATIC_CAPS(VIDEO_SRC_CAPS));

static void gst_mlx90640_bar_class_init(GstMlx90640BarClass *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
	GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS(klass);
	GstElementClass *gstelement_class = GST_ELEMENT_CLASS(klass);

	/* Setting up pads and setting metadata should be moved to
	   base_class_init if you intend to subclass this class. */
	gst_element_class_add_static_pad_template(gstelement_class,
						  &gst_mlx90640_bar_sink_template);
	gst_element_class_add_static_pad_template(gstelement_class,
						  &gst_mlx90640_bar_src_template);

	gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
				"MLX90640 colour bar", "Generic", "MLX90640 colour bar writer",
				"Guennadi Liakhovetski <g.liakhovetski@gmx.de>");

	gobject_class->set_property = gst_mlx90640_bar_set_property;
	gobject_class->get_property = gst_mlx90640_bar_get_property;
	gobject_class->finalize = gst_mlx90640_bar_finalize;
	base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_mlx90640_bar_transform_caps);
	video_filter_class->transform_frame = GST_DEBUG_FUNCPTR(gst_mlx90640_bar_transform_frame);
	video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR(gst_mlx90640_bar_transform_frame_ip);
}

static void gst_mlx90640_bar_init(GstMlx90640Bar *mlx90640bar)
{
	mlx90640bar->frame_count = 0;
}

static void gst_mlx90640_bar_set_property(GObject *object, guint property_id,
					  const GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void gst_mlx90640_bar_get_property(GObject *object, guint property_id,
					  GValue *value, GParamSpec *pspec)
{
	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

void gst_mlx90640_bar_finalize(GObject *object)
{
	/* clean up object here */

	G_OBJECT_CLASS(gst_mlx90640_bar_parent_class)->finalize(object);
}

#define GST_MLX90640_BAR_HEIGHT 80

static gboolean gst_mlx90640_bar_transform_dimension_value(const GValue *src_val,
							   gint dw, GValue *dest_val)
{
	g_value_init(dest_val, G_VALUE_TYPE(src_val));

	if (G_VALUE_HOLDS_INT(src_val)) {
		gint ival = g_value_get_int(src_val);

		g_value_set_int(dest_val, ival + dw);

		return TRUE;
	}

	if (GST_VALUE_HOLDS_INT_RANGE(src_val)) {
		gint min = gst_value_get_int_range_min(src_val);
		gint max = gst_value_get_int_range_max(src_val);

		if (min >= max) {
			g_value_unset(dest_val);
			return FALSE;
		}

		gst_value_set_int_range(dest_val, min + dw, max + dw);

		return TRUE;
	}

	if (GST_VALUE_HOLDS_LIST(src_val)) {
		gint j;

		for (j = 0; j < gst_value_list_get_size(src_val); ++j) {
			const GValue *list_val;
			GValue newval = { 0, };

			list_val = gst_value_list_get_value(src_val, j);
			if (gst_mlx90640_bar_transform_dimension_value(list_val, dw, &newval)) {
				gst_value_list_append_value(dest_val, &newval);
				g_value_unset(&newval);
			}
		}

		return TRUE;
	}

	return FALSE;
}

static GstCaps *gst_mlx90640_bar_transform_caps(GstBaseTransform *trans, GstPadDirection direction,
						GstCaps *from, GstCaps *filter)
{
	GstMlx90640Bar *mlx90640bar = GST_MLX90640_BAR(trans);
	GstCaps *ret, *to;
	gint i;

	to = gst_caps_new_empty();

	for (i = 0; i < gst_caps_get_size(from); i++) {
		GstStructure *structure = gst_structure_copy(gst_caps_get_structure(from, i));
		/* gst_structure_get_value() doesn't seem to take reference, no need to dereference */
		const GValue *src_val = gst_structure_get_value(structure, "height");
		GValue w_val = { 0, };
		gint dw = direction == GST_PAD_SINK ? GST_MLX90640_BAR_HEIGHT :
			-GST_MLX90640_BAR_HEIGHT;

		if (!gst_mlx90640_bar_transform_dimension_value(src_val, dw, &w_val)) {
			GST_WARNING_OBJECT(mlx90640bar,
					   "could not transform caps structure=%" GST_PTR_FORMAT,
					   structure);
			gst_structure_free(structure);
			gst_caps_unref(to);
			return gst_caps_new_empty();
		}

		gst_structure_set_value(structure, "height", &w_val);
		g_value_unset(&w_val);

		gst_caps_append_structure(to, structure);
	}

	/* filter against set allowed caps on the pad */
	GstPad *other = direction == GST_PAD_SINK ? trans->srcpad : trans->sinkpad;
	GstCaps *templ = gst_pad_get_pad_template_caps(other);

	ret = gst_caps_intersect(to, templ);
	gst_caps_unref(to);
	gst_caps_unref(templ);

	if (ret && filter) {
		GstCaps *intersection = gst_caps_intersect_full(filter, ret,
								GST_CAPS_INTERSECT_FIRST);
		gst_caps_unref(ret);
		ret = intersection;
	}

	return ret;
}

/* transform */

static GstFlowReturn gst_mlx90640_bar_transform_frame(GstVideoFilter *filter, GstVideoFrame *inframe,
						      GstVideoFrame *outframe)
{
	GstMlx90640Bar *mlx90640bar = GST_MLX90640_BAR(filter);
	const unsigned int width = GST_VIDEO_FRAME_WIDTH(inframe);
	/* 2 lines of metadata */
	const unsigned int height = GST_VIDEO_FRAME_HEIGHT(inframe);

	if (width % 32 || height % 26 || width / 32 != height / 26)
		return GST_FLOW_ERROR;

	guint32 *in_pix = GST_VIDEO_FRAME_PLANE_DATA(inframe, 0);
	guint32 *out_pix = GST_VIDEO_FRAME_PLANE_DATA(outframe, 0);
	GstMeta *meta = gst_buffer_get_meta(inframe->buffer, gst_mlx90640_meta_api_get_type());
	GstMlx90640Meta *mlx_meta = (GstMlx90640Meta *)meta;
	gint i;

	gint img_size = height * width;
	memcpy(out_pix, in_pix, img_size * sizeof(out_pix[0]));

	/* skip 5 lines */
	guint32 *line = out_pix + img_size + width * 5;

	for (i = 0; i < width; i++) {
		unsigned int delta = 256 * i / width;
		unsigned int blue = 256 - delta;
		unsigned int red = delta;

		line[i] = red | (blue << 16);
	}

	for (i = 1; i < 20; i++)
		memcpy(line + width * i, line, width * sizeof(line[0]));
	memset(line + width * 20, 0, width * (GST_MLX90640_BAR_HEIGHT - 25) * sizeof(line[0]));

	GST_DEBUG_OBJECT(mlx90640bar, "frame %u, %g <= t <= %g", mlx90640bar->frame_count++,
			 mlx_meta->t_min, mlx_meta->t_max);

	return GST_FLOW_OK;
}

static GstFlowReturn gst_mlx90640_bar_transform_frame_ip(GstVideoFilter *filter,
							 GstVideoFrame *frame)
{

	return GST_FLOW_OK;
}

static gboolean plugin_init(GstPlugin *plugin)
{
	return GST_ELEMENT_REGISTER(mlx90640bar, plugin);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.1"
#endif
#ifndef PACKAGE
#define PACKAGE "mlx90640_bar_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "mlx90640_bar_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://open-technology.de/"
#endif

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
		  GST_VERSION_MINOR,
		  mlx90640bar,
		  "MLX90640 colour bar writer",
		  plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
