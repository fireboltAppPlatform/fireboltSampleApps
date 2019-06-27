/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2017 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef GstMSESrc_h
#define GstMSESrc_h

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_MSE_TYPE_SRC            (gst_mse_src_get_type ())
#define GST_MSE_SRC(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_MSE_TYPE_SRC, GstMSESrc))
#define GST_MSE_SRC_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GST_MSE_TYPE_SRC, GstMSESrcClass))
#define GST_IS_MSE_SRC(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_MSE_TYPE_SRC))
#define GST_IS_MSE_SRC_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_MSE_TYPE_SRC))

typedef struct _GstMSESrc        GstMSESrc;
typedef struct _GstMSESrcClass   GstMSESrcClass;
typedef struct _GstMSESrcPrivate GstMSESrcPrivate;

struct _GstMSESrc {
    GstBin parent;

    GstMSESrcPrivate *priv;
};

struct _GstMSESrcClass {
    GstBinClass parentClass;
};

GType gst_mse_src_get_type(void);

void gst_mse_src_register_player(GstElement*, GstElement*);
void gst_mse_src_unregister_player(GstElement*, GstElement*);
void gst_mse_src_configuration_done(GstElement*);
gboolean gst_mse_src_configured(GstElement*);
G_END_DECLS

#endif
