#include <gst/gst.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <fcntl.h>

int BITRATE = 6000;
int FIFO_FD = 0;
char* FIFO_PATH;

/* Structure to contain all our information, so we can pass it to callbacks */
typedef struct _CustomData {
  GstElement *pipeline;
  GstElement *pulsesrc;
  GstElement *audioconvert;
  GstElement *opusenc;
  GstElement *rtpopuspay;
  GstElement *fdsink;
} CustomData;

int main(int argc, char *argv[]) {
  if (argc != 3) {
    fprintf(stderr, "usage: %s <fifo_path> <bitrate>\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  FIFO_PATH = argv[1];
  BITRATE = atoi(argv[2]);

  printf("info: FIFO_PATH=%s\n", FIFO_PATH);
  printf("info: BITRATE=%d\n", BITRATE);

  CustomData data;
  GstBus *bus;
  GstMessage *msg;
  GstStateChangeReturn ret;
  gboolean terminate = FALSE;

  int FIFO_FD = open(FIFO_PATH, O_WRONLY | O_NONBLOCK);
  if (FIFO_FD == -1) {
    fprintf(stderr, "error: FIFO_FD=%d\n", FIFO_FD);
    exit(EXIT_FAILURE);
  }

  /* Initialize GStreamer */
  gst_init (&argc, &argv);

  /* Create the elements */
  data.pulsesrc = gst_element_factory_make ("pulsesrc", "pulsesource");
  data.audioconvert = gst_element_factory_make ("audioconvert", "audioconvert");
  data.opusenc = gst_element_factory_make ("opusenc", "opusenc");
  data.rtpopuspay = gst_element_factory_make ("rtpopuspay", "rtpopuspay");
  data.fdsink = gst_element_factory_make ("fdsink", "fdsink");

  /* Create the empty pipeline */
  data.pipeline = gst_pipeline_new ("opus-pipeline");

  if (!data.pipeline || !data.pulsesrc || !data.audioconvert || !data.opusenc || !data.rtpopuspay || !data.fdsink) {
    g_printerr ("Not all elements could be created.\n");
    return -1;
  }

  /* Build the pipeline. Note that we are NOT linking the source at this
   * point. We will do it later. */
  gst_bin_add_many (GST_BIN (data.pipeline), data.pulsesrc, data.audioconvert, data.opusenc, data.rtpopuspay, data.fdsink, NULL);
  if (!gst_element_link_many (data.pulsesrc, data.audioconvert, data.opusenc, data.rtpopuspay, data.fdsink, NULL)) {
    g_printerr ("Elements could not be linked.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  /* Set the element parameters */
  g_object_set (data.fdsink, "fd", FIFO_FD, NULL);
  g_object_set (data.fdsink, "sync", false, NULL);
  g_object_set (data.fdsink, "async", false, NULL);
  g_object_set (data.opusenc, "bitrate", BITRATE, NULL);  

  /* Start playing */
  ret = gst_element_set_state (data.pipeline, GST_STATE_PLAYING);
  if (ret == GST_STATE_CHANGE_FAILURE) {
    g_printerr ("Unable to set the pipeline to the playing state.\n");
    gst_object_unref (data.pipeline);
    return -1;
  }

  /* Listen to the bus */
  bus = gst_element_get_bus (data.pipeline);
  do {
    msg = gst_bus_timed_pop_filtered (bus, GST_CLOCK_TIME_NONE,
        GST_MESSAGE_STATE_CHANGED | GST_MESSAGE_ERROR | GST_MESSAGE_EOS);

    /* Parse message */
    if (msg != NULL) {
      GError *err;
      gchar *debug_info;

      switch (GST_MESSAGE_TYPE (msg)) {
        case GST_MESSAGE_ERROR:
          gst_message_parse_error (msg, &err, &debug_info);
          g_printerr ("Error received from element %s: %s\n", GST_OBJECT_NAME (msg->src), err->message);
          g_printerr ("Debugging information: %s\n", debug_info ? debug_info : "none");
          g_clear_error (&err);
          g_free (debug_info);
          terminate = TRUE;
          break;
        case GST_MESSAGE_EOS:
          g_print ("End-Of-Stream reached.\n");
          terminate = TRUE;
          break;
        case GST_MESSAGE_STATE_CHANGED:
          /* We are only interested in state-changed messages from the pipeline */
          if (GST_MESSAGE_SRC (msg) == GST_OBJECT (data.pipeline)) {
            GstState old_state, new_state, pending_state;
            gst_message_parse_state_changed (msg, &old_state, &new_state, &pending_state);
            g_print ("Pipeline state changed from %s to %s:\n",
               gst_element_state_get_name (old_state), gst_element_state_get_name (new_state));
          }
          break;
        default:
          /* We should not reach here */
          g_printerr ("Unexpected message received.\n");
          break;
      }
      gst_message_unref (msg);
    }
  } while (!terminate);

  /* Free resources */
  gst_object_unref (bus);
  gst_element_set_state (data.pipeline, GST_STATE_NULL);
  gst_object_unref (data.pipeline);
  return 0;
}
