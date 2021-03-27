/* GStreamer
 * Copyright (C) 2008 Wim Taymans <wim.taymans at gmail.com>
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

#include <zmq.hpp>
#include <gst/gst.h>
#include <iostream>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb/stb_image_write.h>

#include <gst/rtsp-server/rtsp-server.h>

#define WIDTH 1024
#define HEIGHT 768

typedef struct
{
  gboolean white;
  GstClockTime timestamp;
} MyContext;


zmq::context_t ctx;
zmq::socket_t sock(ctx, zmq::socket_type::pull);

/* called when we need to give data to appsrc */
static void
need_data (GstElement * appsrc, guint unused, MyContext * ctx)
{
    GstBuffer *buffer;
    guint size;
    GstFlowReturn ret;
    GstMapInfo map;

    zmq::message_t msg;

    sock.recv(msg, zmq::recv_flags::none);

    size = WIDTH * HEIGHT * 4;

    buffer = gst_buffer_new_allocate (NULL, size, NULL);
    // buffer = gst_buffer_new_and_alloc(msg.size());
    gst_buffer_map (buffer, &map, GST_MAP_WRITE);
    memcpy (map.data, msg.data(), size);

    gst_buffer_unmap (buffer, &map);
    
    /* increment the timestamp every 1/2 second */
    GST_BUFFER_PTS (buffer) = ctx->timestamp;
    GST_BUFFER_DURATION (buffer) = gst_util_uint64_scale_int (1, GST_SECOND, 2);
    ctx->timestamp += GST_BUFFER_DURATION (buffer);

    g_signal_emit_by_name (appsrc, "push-buffer", buffer, &ret);

    if (ret != GST_FLOW_OK) {
            g_debug("push buffer returned %d for %d bytes \n", ret, size);
    }    
    gst_buffer_unref (buffer);
    return; 
}

/* called when a new media pipeline is constructed. We can query the
 * pipeline and configure our appsrc */
static void
media_configure (GstRTSPMediaFactory * factory, GstRTSPMedia * media,
    gpointer user_data)
{
    GstElement *element, *appsrc;
    MyContext *ctx;

    /* get the element used for providing the streams of the media */
    element = gst_rtsp_media_get_element (media);

    /* get our appsrc, we named it 'mysrc' with the name property */
    appsrc = gst_bin_get_by_name_recurse_up (GST_BIN (element), "mysrc");

    /* this instructs appsrc that we will be dealing with timed buffer */
    gst_util_set_object_arg (G_OBJECT (appsrc), "format", "time");
    /* configure the caps of the video */
    g_object_set (G_OBJECT (appsrc), "caps",
        gst_caps_new_simple ("video/x-raw",
            "format", G_TYPE_STRING, "RGBA",
            "width", G_TYPE_INT, WIDTH,
            "height", G_TYPE_INT, HEIGHT,
            "framerate", GST_TYPE_FRACTION, 60, 1, NULL), NULL);

    ctx = g_new0 (MyContext, 1);
    ctx->white = FALSE;
    ctx->timestamp = 0;
    /* make sure ther datais freed when the media is gone */
    g_object_set_data_full (G_OBJECT (media), "my-extra-data", ctx,
        (GDestroyNotify) g_free);

    /* install the callback that will be called when a buffer is needed */
    g_signal_connect (appsrc, "need-data", (GCallback) need_data, ctx);
    gst_object_unref (appsrc);
    gst_object_unref (element);
}

int
main (int argc, char *argv[])
{
    GMainLoop *loop;
    GstRTSPServer *server;
    GstRTSPMountPoints *mounts;
    GstRTSPMediaFactory *factory;

        // sock = zmq::socket_t(ctx, zmq::socket_type::pull);
    sock.connect("ipc:///tmp/streamer.zmq");

    gst_init (&argc, &argv);

    loop = g_main_loop_new (NULL, FALSE);

    /* create a server instance */
    server = gst_rtsp_server_new ();

    /* get the mount points for this server, every server has a default object
    * that be used to map uri mount points to media factories */
    mounts = gst_rtsp_server_get_mount_points (server);

    /* make a media factory for a test stream. The default media factory can use
    * gst-launch syntax to create pipelines.
    * any launch line works as long as it contains elements named pay%d. Each
    * element with pay%d names will be a stream */
    factory = gst_rtsp_media_factory_new ();
    gst_rtsp_media_factory_set_launch (factory,
        "( appsrc name=mysrc ! queue ! videoconvert ! x264enc ! rtph264pay name=pay0 pt=96 )");

    /* notify when our media is ready, This is called whenever someone asks for
    * the media and a new pipeline with our appsrc is created */
    g_signal_connect (factory, "media-configure", (GCallback) media_configure,
        NULL);

    /* attach the test factory to the /test url */
    gst_rtsp_mount_points_add_factory (mounts, "/test", factory);

    /* don't need the ref to the mounts anymore */
    g_object_unref (mounts);

    /* attach the server to the default maincontext */
    gst_rtsp_server_attach (server, NULL);

    /* start serving */
    g_print ("stream ready at rtsp://127.0.0.1:8554/test\n");
    g_main_loop_run (loop);

    return 0;
}