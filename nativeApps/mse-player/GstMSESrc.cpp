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

#include "GstMSESrc.h"

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#define GST_MSE_SRC_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE((obj), GST_MSE_TYPE_SRC, GstMSESrcPrivate))
struct _GstMSESrcPrivate {
  gchar* uri;
  guint pad_counter;
  gboolean configured;
};

enum {
    PROP_0,
    PROP_LOCATION
};

static GstStaticPadTemplate srcTemplate = GST_STATIC_PAD_TEMPLATE("src_%u",
                                                                  GST_PAD_SRC,
                                                                  GST_PAD_SOMETIMES,
                                                                  GST_STATIC_CAPS_ANY);

GST_DEBUG_CATEGORY_STATIC(gst_mse_src_debug);
#define GST_CAT_DEFAULT gst_mse_src_debug

static void gst_mse_src_uri_handler_init(gpointer gIface, gpointer ifaceData);

static void gst_mse_src_dispose(GObject*);
static void gst_mse_src_finalize(GObject*);
static void gst_mse_src_set_property(GObject*, guint propertyID, const GValue*, GParamSpec*);
static GstStateChangeReturn gst_mse_src_change_state(GstElement*, GstStateChange);
static void gst_mse_src_handle_message(GstBin*, GstMessage*);
static gboolean gst_mse_src_query_with_parent(GstPad*, GstObject*, GstQuery*);
static void gst_mse_src_get_property(GObject*, guint propertyID, GValue*, GParamSpec*);

#define gst_mse_src_parent_class parent_class
G_DEFINE_TYPE_WITH_CODE(GstMSESrc, gst_mse_src, GST_TYPE_BIN,
                        G_IMPLEMENT_INTERFACE(GST_TYPE_URI_HANDLER, gst_mse_src_uri_handler_init);
                        GST_DEBUG_CATEGORY_INIT(gst_mse_src_debug, "msesrc", 0, "mse src element"););

static void gst_mse_src_class_init(GstMSESrcClass* klass)
{
    GObjectClass* oklass = G_OBJECT_CLASS(klass);
    GstElementClass* eklass = GST_ELEMENT_CLASS(klass);
    GstBinClass* bklass = GST_BIN_CLASS(klass);

    oklass->dispose = gst_mse_src_dispose;
    oklass->finalize = gst_mse_src_finalize;
    oklass->set_property = gst_mse_src_set_property;
    oklass->get_property = gst_mse_src_get_property;

    gst_element_class_add_pad_template(eklass,
                                       gst_static_pad_template_get(&srcTemplate));

    g_object_class_install_property(oklass,
                                    PROP_LOCATION,
                                    g_param_spec_string("location",
                                                        "location",
                                                        "Location to read from",
                                                        0,
                                                        (GParamFlags) (G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS)));


    eklass->change_state = GST_DEBUG_FUNCPTR(gst_mse_src_change_state);
    // eklass->send_event = gst_mse_src_send_event;

    bklass->handle_message = GST_DEBUG_FUNCPTR(gst_mse_src_handle_message);

    g_type_class_add_private(klass, sizeof(GstMSESrcPrivate));
}

static void gst_mse_src_init(GstMSESrc* src)
{
    GstMSESrcPrivate* priv = GST_MSE_SRC_GET_PRIVATE(src);

    src->priv = priv;
    src->priv->configured = FALSE;
    src->priv->pad_counter = 0;

    g_object_set(GST_BIN(src), "message-forward", TRUE, NULL);
}

static void gst_mse_src_dispose(GObject* object)
{
    GST_CALL_PARENT(G_OBJECT_CLASS, dispose, (object));
}

static void gst_mse_src_finalize(GObject* object)
{
    GstMSESrc* src = GST_MSE_SRC(object);
    GstMSESrcPrivate* priv = src->priv;

    g_free(priv->uri);
    priv->~GstMSESrcPrivate();

    GST_CALL_PARENT(G_OBJECT_CLASS, finalize, (object));
}

static void gst_mse_src_set_property(GObject* object, guint prop, const GValue* value, GParamSpec* pspec)
{
    GstMSESrc* src = GST_MSE_SRC(object);
    const gchar* uri = NULL;

    switch (prop) {
    case PROP_LOCATION:
        uri = g_value_get_string(value);
        gst_uri_handler_set_uri((GstURIHandler*)(src), uri, 0);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop, pspec);
        break;
    }
}


static GstStateChangeReturn gst_mse_src_change_state(GstElement* element, GstStateChange transition)
{
    GstMSESrc* src = GST_MSE_SRC(element);
    GstMSESrcPrivate* priv = src->priv;
    GstStateChangeReturn ret = GST_STATE_CHANGE_SUCCESS;

    ret = GST_ELEMENT_CLASS(parent_class)->change_state(element, transition);
    return ret;
}


// uri handler interface

static GstURIType gst_mse_src_uri_get_type(GType)
{
    return GST_URI_SRC;
}

const gchar* const* gst_mse_src_get_protocols(GType)
{
    static const char* protocols[] = {"mse", 0 };
    return protocols;
}

static gchar* gst_mse_src_get_uri(GstURIHandler* handler)
{
    GstMSESrc* src = GST_MSE_SRC(handler);
    gchar* ret;

    GST_OBJECT_LOCK(src);
    ret = g_strdup(src->priv->uri);
    GST_OBJECT_UNLOCK(src);
    return ret;
}

static gboolean gst_mse_src_set_uri(GstURIHandler* handler, const gchar* uri, GError** error)
{
    GstMSESrc* src = GST_MSE_SRC(handler);
    GstMSESrcPrivate* priv = src->priv;

    if (GST_STATE(src) >= GST_STATE_PAUSED) {
        GST_ERROR_OBJECT(src, "URI can only be set in states < PAUSED");
        return FALSE;
    }

    GST_OBJECT_LOCK(src);

    g_free(priv->uri);
    priv->uri = 0;

    if (!uri) {
      GST_OBJECT_UNLOCK(src);
        return TRUE;
    }

    priv->uri = g_strdup(uri);
    GST_OBJECT_UNLOCK(src);
    return TRUE;
}

static void gst_mse_src_uri_handler_init(gpointer gIface, gpointer)
{
    GstURIHandlerInterface* iface = (GstURIHandlerInterface *) gIface;

    iface->get_type = gst_mse_src_uri_get_type;
    iface->get_protocols = gst_mse_src_get_protocols;
    iface->get_uri = gst_mse_src_get_uri;
    iface->set_uri = gst_mse_src_set_uri;
}

static gboolean gst_mse_src_query_with_parent(GstPad* pad, GstObject* parent, GstQuery* query)
{
  GstMSESrc* src = GST_MSE_SRC(GST_ELEMENT(parent));
  gboolean result = FALSE;

  switch (GST_QUERY_TYPE(query)) {
  default:{
    GstPad* target = gst_ghost_pad_get_target(GST_GHOST_PAD_CAST(pad));
    // Forward the query to the proxy target pad.
    if (target)
      result = gst_pad_query(target, query);
    gst_object_unref(target);
    break;
  }
  }

  return result;
}

void gst_mse_src_handle_message(GstBin* bin, GstMessage* message)
{
  GstMSESrc* src = GST_MSE_SRC(GST_ELEMENT(bin));

  switch (GST_MESSAGE_TYPE(message)) {
  case GST_MESSAGE_EOS: {
    gboolean emit_eos = TRUE;
    GstPad* pad = gst_element_get_static_pad(GST_ELEMENT(GST_MESSAGE_SRC(message)), "src");

    GST_DEBUG_OBJECT(src, "EOS received from %s", GST_MESSAGE_SRC_NAME(message));
    g_object_set_data(G_OBJECT(pad), "is-eos", GINT_TO_POINTER(1));
    gst_object_unref(pad);
    for (guint i = 0; i < src->priv->pad_counter; i++) {
      gchar* name = g_strdup_printf("src_%u", i);
      GstPad* src_pad = gst_element_get_static_pad(GST_ELEMENT(src), name);
      GstPad* target = gst_ghost_pad_get_target(GST_GHOST_PAD_CAST(src_pad));
      gint is_eos = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(target), "is-eos"));
      gst_object_unref(target);
      gst_object_unref(src_pad);
      g_free(name);

      if (!is_eos) {
        emit_eos = FALSE;
        break;
      }
    }

    gst_message_unref(message);

    if (emit_eos) {
      GST_DEBUG_OBJECT(src, "All appsrc elements are EOS, emitting event now.");
      gst_element_send_event(GST_ELEMENT(bin), gst_event_new_eos());
    }
    break;
  }
  default:
    GST_BIN_CLASS(parent_class)->handle_message(bin, message);
    break;
  }
}

void gst_mse_src_register_player(GstElement* element, GstElement* appsrc)
{
  GstMSESrc* src = GST_MSE_SRC(element);
  gchar* name = g_strdup_printf("src_%u", src->priv->pad_counter);

  src->priv->pad_counter++;

  gst_bin_add(GST_BIN(element), appsrc);
  GstPad* target = gst_element_get_static_pad(appsrc, "src");
  GstPad* pad = gst_ghost_pad_new(name, target);

  gst_pad_set_query_function(pad, gst_mse_src_query_with_parent);
  gst_pad_set_active(pad, TRUE);

  gst_element_add_pad(element, pad);
  GST_OBJECT_FLAG_SET(pad, GST_PAD_FLAG_NEED_PARENT);

  gst_element_sync_state_with_parent(appsrc);

  g_free(name);
  gst_object_unref(target);
}

void gst_mse_src_unregister_player(GstElement* element, GstElement* appsrc)
{
  GstMSESrc* src = GST_MSE_SRC(element);
  GstPad* pad = gst_element_get_static_pad(appsrc, "src");
  GstPad* peer = gst_pad_get_peer(pad);

  GST_DEBUG_OBJECT(src, "Unregistering player from pad %s, appsrc: %p", GST_PAD_NAME(pad), appsrc);

  if (peer) {
    GstPad* ghost = GST_PAD_CAST(gst_proxy_pad_get_internal(GST_PROXY_PAD(peer)));

    if (ghost) {
      gst_ghost_pad_set_target(GST_GHOST_PAD_CAST(ghost), NULL);
      gst_element_remove_pad(element, ghost);
      gst_object_unref(ghost);
    }

    gst_object_unref(peer);
  }

  gst_app_src_end_of_stream(GST_APP_SRC(appsrc));

  gst_element_set_state(appsrc, GST_STATE_NULL);
  gst_bin_remove(GST_BIN(src), appsrc);

  if (src->priv->pad_counter > 0)
    src->priv->pad_counter--;

  if (GST_BIN_NUMCHILDREN(src) == 0) {
    GST_DEBUG_OBJECT(src, "No player left, unconfiguring");
    src->priv->configured = FALSE;
  }
}

void gst_mse_src_configuration_done(GstElement* element)
{
  GstMSESrc* src = GST_MSE_SRC(element);

  src->priv->configured = TRUE;
  GST_DEBUG_OBJECT(src, "All players registered, proceeding with state-change completion");
  gst_element_no_more_pads(element);
}

gboolean gst_mse_src_configured(GstElement* element)
{
  GstMSESrc* src = GST_MSE_SRC(element);
  return src->priv->configured;
}

static void gst_mse_src_get_property(GObject* object, guint prop, GValue* value, GParamSpec* pspec)
{
    GstMSESrc* src = GST_MSE_SRC(object);
    GstMSESrcPrivate* priv = src->priv;

    GST_OBJECT_LOCK(src);
    switch (prop) {
    case PROP_LOCATION:
        g_value_set_string(value, priv->uri);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop, pspec);
        break;
    }
    GST_OBJECT_UNLOCK(src);
}
