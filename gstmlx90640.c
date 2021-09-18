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
 * Free Software Foundation, Inc., 51 Franklin Street, Suite 500,
 * Boston, MA 02110-1335, USA.
 */
/**
 * SECTION:element-gstmlx90640
 *
 * The mlx90640 element converts raw infrared frames to a viewable format.
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch-1.0 -v fakesrc ! mlx90640 ! FIXME ! fakesink
 * ]|
 * FIXME Describe what the pipeline does.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "MLX90640_API.h"
#include <stdint.h>
#include <stdio.h>

#include <gst/gst.h>
#include <gst/gstmeta.h>
#include <gst/video/video.h>
#include <gst/video/gstvideofilter.h>
#include "gstmlx90640.h"

GST_DEBUG_CATEGORY_STATIC(gst_mlx90640_debug);
#define GST_CAT_DEFAULT gst_mlx90640_debug

/* prototypes */

static void gst_mlx90640_set_property(GObject *object, guint property_id,
				      const GValue *value, GParamSpec *pspec);
static void gst_mlx90640_get_property(GObject *object, guint property_id,
				      GValue *value, GParamSpec *pspec);
static void gst_mlx90640_finalize(GObject *object);

static GstFlowReturn gst_mlx90640_transform_frame(GstVideoFilter *filter,
						  GstVideoFrame *inframe, GstVideoFrame *outframe);
static GstFlowReturn gst_mlx90640_transform_frame_ip(GstVideoFilter *filter,
						     GstVideoFrame *frame);
static GstCaps *gst_mlx90640_transform_caps(GstBaseTransform *trans, GstPadDirection direction,
					    GstCaps *caps, GstCaps *filter);

/* Filter signals and args. */
enum {
	LAST_SIGNAL
};

enum {
	PROP_0
};

/* pad templates */

#define VIDEO_SRC_CAPS GST_VIDEO_CAPS_MAKE("{ RGBA }")
#define VIDEO_SINK_CAPS GST_VIDEO_CAPS_MAKE("{ GRAY16_BE }")

/* class initialization */

GType gst_mlx90640_meta_api_get_type(void)
{
	static GType type;

	if (g_once_init_enter(&type)) {
		static const gchar *tags[] = {
			GST_META_TAG_VIDEO_STR,
			NULL
		};
		GType _type = gst_meta_api_type_register("GstMlx90640API", tags);
		g_once_init_leave(&type, _type);
	}
	return type;
}

static gboolean gst_mlx90640_meta_init(GstMeta *meta, gpointer params,
				       GstBuffer *buffer)
{
	GstMlx90640Meta *mlx_meta = (GstMlx90640Meta *)meta;

	/* MLX9640 target temperature range */
	mlx_meta->t_min = -40.;
	mlx_meta->t_max = 300.;

	return TRUE;
}

static const GstMetaInfo *gst_mlx90640_meta_get_info(void);

static gboolean gst_mlx90640_meta_transform(GstBuffer *dest, GstMeta *meta,
					    GstBuffer *buffer, GQuark type, gpointer data)
{
	GstMlx90640Meta *dmeta, *smeta;

	smeta = (GstMlx90640Meta *)meta;

	if (GST_META_TRANSFORM_IS_COPY(type) ||
	    GST_VIDEO_META_TRANSFORM_IS_SCALE(type)) {
		dmeta = (GstMlx90640Meta *)gst_buffer_add_meta(dest,
							       gst_mlx90640_meta_get_info(), NULL);

		if (!dmeta)
			return FALSE;

		dmeta->t_max = smeta->t_max;
		dmeta->t_min = smeta->t_min;
	}
	return TRUE;
}

static const GstMetaInfo *gst_mlx90640_meta_get_info(void)
{
	static GstMetaInfo *meta_info;

	if (g_once_init_enter(&meta_info)) {
		GstMetaInfo *mi = (GstMetaInfo *)gst_meta_register(gst_mlx90640_meta_api_get_type(),
								   "GstMlx90640Meta",
								   sizeof(GstMlx90640Meta),
								   gst_mlx90640_meta_init,
								   NULL,
								   gst_mlx90640_meta_transform);
		g_once_init_leave(&meta_info, mi);
	}

	return meta_info;
}

static GstMlx90640Meta *
gst_buffer_add_mlx90640_meta(GstBuffer *buffer,
			     float t_min, float t_max)
{
	GstMlx90640Meta *meta;

	g_return_val_if_fail(GST_IS_BUFFER(buffer), NULL);
	g_return_val_if_fail(t_max >= t_min, NULL);

	meta = (GstMlx90640Meta *)gst_buffer_add_meta(buffer,
						      gst_mlx90640_meta_get_info(), NULL);
	g_return_val_if_fail(meta != NULL, NULL);

	meta->t_min = t_min;
	meta->t_max = t_max;

	return meta;
}

#define gst_mlx90640_parent_class parent_class
G_DEFINE_TYPE(GstMlx90640, gst_mlx90640, GST_TYPE_VIDEO_FILTER);
GST_ELEMENT_REGISTER_DEFINE_WITH_CODE(mlx90640, "mlx90640", GST_RANK_NONE, GST_TYPE_MLX90640,
				      GST_DEBUG_CATEGORY_INIT(gst_mlx90640_debug,
							      "mlx90640", 0,
							      "debug category for mlx90640 element"));

/* The capabilities of the inputs and outputs. */

static GstStaticPadTemplate gst_mlx90640_sink_template =
	GST_STATIC_PAD_TEMPLATE("sink", GST_PAD_SINK, GST_PAD_ALWAYS,
				GST_STATIC_CAPS(VIDEO_SINK_CAPS));

static GstStaticPadTemplate gst_mlx90640_src_template =
	GST_STATIC_PAD_TEMPLATE("src", GST_PAD_SRC, GST_PAD_ALWAYS,
				GST_STATIC_CAPS(VIDEO_SRC_CAPS));

static GstStaticPadTemplate thermal_template = GST_STATIC_PAD_TEMPLATE("thermal-src",
					GST_PAD_SRC, GST_PAD_ALWAYS,
					GST_STATIC_CAPS("text/x-raw, format= { pango-markup, utf8 }"));

static void gst_mlx90640_class_init(GstMlx90640Class *klass)
{
	GObjectClass *gobject_class = G_OBJECT_CLASS(klass);
	GstBaseTransformClass *base_transform_class = GST_BASE_TRANSFORM_CLASS(klass);
	GstVideoFilterClass *video_filter_class = GST_VIDEO_FILTER_CLASS(klass);
	GstElementClass *element_class = GST_ELEMENT_CLASS(klass);

	gst_element_class_set_static_metadata(GST_ELEMENT_CLASS(klass),
					      "MLX90640 IR camera filter", "Generic",
					      "MLX90640 infrared camera video filter",
					      "Guennadi Liakhovetski <g.liakhovetski@gmx.de>");

	/* Setting up pads and setting metadata should be moved to
	   base_class_init if you intend to subclass this class. */
	gst_element_class_add_static_pad_template(element_class,
						  &gst_mlx90640_sink_template);
	gst_element_class_add_static_pad_template(element_class,
						  &gst_mlx90640_src_template);
	gst_element_class_add_static_pad_template(element_class,
						  &thermal_template);

	gobject_class->set_property = gst_mlx90640_set_property;
	gobject_class->get_property = gst_mlx90640_get_property;
	gobject_class->finalize = gst_mlx90640_finalize;
	/* Override .transform_caps to support format change */
	base_transform_class->transform_caps = GST_DEBUG_FUNCPTR(gst_mlx90640_transform_caps);
	video_filter_class->transform_frame = GST_DEBUG_FUNCPTR(gst_mlx90640_transform_frame);
	video_filter_class->transform_frame_ip = GST_DEBUG_FUNCPTR(gst_mlx90640_transform_frame_ip);
}

#ifndef __x86_64__
static paramsMLX90640 mlx90640_param;
static uint16_t eeprom[832];
#endif

static void gst_mlx90640_init(GstMlx90640 *mlx90640)
{
	mlx90640->therm_pad = gst_pad_new_from_static_template(&thermal_template, "thermal-src");
	gst_pad_use_fixed_caps(mlx90640->therm_pad);
	gst_pad_set_caps(mlx90640->therm_pad,
			 gst_static_pad_template_get_caps(&thermal_template));
	gst_element_add_pad(GST_ELEMENT(mlx90640), mlx90640->therm_pad);
	mlx90640->count = 0;
}

static void gst_mlx90640_set_property(GObject *object, guint property_id,
				      const GValue *value, GParamSpec *pspec)
{
	GstMlx90640 *mlx90640 = GST_MLX90640(object);

	GST_DEBUG_OBJECT(mlx90640, "set_property");

	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void gst_mlx90640_get_property(GObject *object, guint property_id,
				      GValue *value, GParamSpec *pspec)
{
	GstMlx90640 *mlx90640 = GST_MLX90640(object);

	GST_DEBUG_OBJECT(mlx90640, "get_property");

	switch (property_id) {
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void gst_mlx90640_finalize(GObject *object)
{
	GstMlx90640 *mlx90640 = GST_MLX90640(object);

	GST_DEBUG_OBJECT(mlx90640, "finalize");

	/* clean up object here */

	G_OBJECT_CLASS(gst_mlx90640_parent_class)->finalize(object);
}

static GstCaps *gst_mlx90640_transform_caps(GstBaseTransform *trans, GstPadDirection direction,
					    GstCaps *caps, GstCaps *filter)
{
	GstCaps *ret;

	if (direction == GST_PAD_SINK)
		ret = gst_static_pad_template_get_caps(&gst_mlx90640_src_template);
	else
		ret = gst_static_pad_template_get_caps(&gst_mlx90640_sink_template);

	if (filter) {
		GstCaps *tmp = ret;
		ret = gst_caps_intersect_full(filter, tmp, GST_CAPS_INTERSECT_FIRST);
		gst_caps_unref(tmp);
	}

	return ret;
}

/* transform */

static GstFlowReturn push_text(GstVideoFilter *filter, GstBuffer *src_buf, float t_min, float t_max)
{
	GstMlx90640 *mlx90640 = GST_MLX90640(filter);
	char buf[128];

	if (!(mlx90640->count++ & 15)) {
#ifdef __x86_64__
		t_min += (float)random() / (1 << 30);
		t_max -= (float)random() / (1 << 30);
#endif
		mlx90640->t_min = t_min;
		mlx90640->t_max = t_max;
	}

	size_t len = snprintf(buf, sizeof(buf),
			      "%.1f                                                                  %.1f",
			      mlx90640->t_min,
			      mlx90640->t_max);
	GstBuffer *buffer = gst_buffer_new_and_alloc(len + 1);
	gst_buffer_fill(buffer, 0, buf, len + 1);
	gst_buffer_resize(buffer, 0, len);
	GST_BUFFER_TIMESTAMP(buffer) = GST_BUFFER_TIMESTAMP(src_buf);
	GST_BUFFER_DURATION(buffer) = GST_BUFFER_DURATION(src_buf);
	return gst_pad_push(mlx90640->therm_pad, buffer);
}

static GstFlowReturn gst_mlx90640_transform(GstVideoFilter *filter, GstVideoFrame *inframe,
					    GstVideoFrame *outframe)
{
	const unsigned int width = GST_VIDEO_FRAME_WIDTH(inframe);
	/* 2 lines of metadata */
	const unsigned int height = GST_VIDEO_FRAME_HEIGHT(inframe) - 2;

	if (width != 32 || height != 24)
		return GST_FLOW_ERROR;

	guint16 *in_pix = GST_VIDEO_FRAME_PLANE_DATA(inframe, 0);
	guint32 *out_pix = GST_VIDEO_FRAME_PLANE_DATA(outframe, 0);
	unsigned int i;

#ifdef __x86_64__
	float tmin = 10., tmax=70., range = 60.;

	for (i = 0; i < 32 * 24; i++) {
		uint32_t red = (in_pix[i] - 0x8000) / range * ((1 << 8) - 1) * 2;
		uint32_t blue = 255 - red;
		out_pix[i] = (red & 0xff) | ((blue << 16) & 0xff0000);
	}
#else // __x86_64__
	float ta = MLX90640_GetTa(in_pix, &mlx90640_param);
	float tpxl[32 * 24];

	MLX90640_CalculateTo(in_pix, &mlx90640_param, 1., ta, tpxl);
	float tmin = 20., tmax = 30., range;

	for (i = 0; i < 32 * 24; i++)
		if (tpxl[i] < tmin)
			tmin = tpxl[i];
		else if (tpxl[i] > tmax)
			tmax = tpxl[i];

	range = tmax - tmin;

	for (i = 0; i < 32 * 24; i++) {
		uint32_t red = (tpxl[i] - tmin) / range * ((1 << 8) - 1);
		uint32_t blue = 255 - red;
		out_pix[i] = (red & 0xff) | ((blue << 16) & 0xff0000);
	}
#endif // __x86_64__

	gst_buffer_add_mlx90640_meta(outframe->buffer, tmin, tmax);

	return push_text(filter, inframe->buffer, tmin, tmax);
}

static GstFlowReturn gst_mlx90640_transform_frame(GstVideoFilter *filter, GstVideoFrame *inframe,
						  GstVideoFrame *outframe)
{
	GstMlx90640 *mlx90640 = GST_MLX90640(filter);

	GST_DEBUG_OBJECT(mlx90640, "transform_frame %d", mlx90640->element.negotiated);

	return gst_mlx90640_transform(filter, inframe, outframe);
}

static GstFlowReturn gst_mlx90640_transform_frame_ip(GstVideoFilter *filter, GstVideoFrame *frame)
{
	GstMlx90640 *mlx90640 = GST_MLX90640(filter);

	GST_DEBUG_OBJECT(mlx90640, "transform_frame_ip");

	return gst_mlx90640_transform(filter, frame, frame);
}

#ifndef __x86_64__
#define SYS_PATH "/sys/bus/nvmem/devices/mlx90640_nvram0/nvmem"
#endif

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

static gboolean plugin_init(GstPlugin *plugin)
{
#ifndef __x86_64__
	FILE *f = fopen(SYS_PATH, "r");

	if (f) {
		size_t n = fread(eeprom, sizeof(eeprom[0]), ARRAY_SIZE(eeprom), f);
		if (n != ARRAY_SIZE(eeprom))
			return FALSE;

		int mlx_err = MLX90640_ExtractParameters(eeprom, &mlx90640_param);
		if (mlx_err < 0)
			return FALSE;

		mlx90640_param.resolution = 0x800; // simulated default ADC resolution 18 bit
	}
#endif
	return GST_ELEMENT_REGISTER(mlx90640, plugin);
}

/* FIXME: these are normally defined by the GStreamer build system.
   If you are creating an element to be included in gst-plugins-*,
   remove these, as they're always defined.  Otherwise, edit as
   appropriate for your external plugin package. */
#ifndef VERSION
#define VERSION "0.0.1"
#endif
#ifndef PACKAGE
#define PACKAGE "mlx90640_package"
#endif
#ifndef PACKAGE_NAME
#define PACKAGE_NAME "mlx90640_package_name"
#endif
#ifndef GST_PACKAGE_ORIGIN
#define GST_PACKAGE_ORIGIN "https://open-technology.de/"
#endif

GST_PLUGIN_DEFINE(GST_VERSION_MAJOR,
		  GST_VERSION_MINOR,
		  mlx90640,
		  "MLX90640 infrared camera data converter",
		  plugin_init, VERSION, "LGPL", PACKAGE_NAME, GST_PACKAGE_ORIGIN)
