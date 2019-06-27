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

#include <string>
#include <cstring>
#include <cstdlib>

#include <essos.h>

#include <unistd.h>
#include <linux/input.h>
#include <cstdio>
#include <libgen.h>
#include "glib_tools.h"
#include <glib-unix.h>

#include <rtRemote.h>
#include <rtLog.h>

#include <glib/gstdio.h>
#include "mediasourcepipeline.h"

#include "wayland-client.h"


#define UNUSED( x ) ((void)(x))

std::string files_path_;
int gPipefd[2];

bool ParseCommandLine(int argc, char** argv) {
  if (argc > 2) {
    printf(
        "Please specify a directory containing raw frame files for media "
        "source playback!\n");
    return false;
  } else if (argc == 2) {
    files_path_ = argv[1];
  }

  return true;
}

static void keyPressed( void* data, unsigned int key )
{
  MediaSourcePipeline* pi = (MediaSourcePipeline*) data;
  pi->HandleKeyboardInput(key); 
}

static void keyReleased( void *, unsigned int )
{
}

static EssKeyListener keyListener=
{
   keyPressed,
   keyReleased
};
void rtMainLoopCb(void*)
{
  // This will be called on the glib main loop thread
  rtError err;
  err = rtRemoteProcessSingleItem();
  if (err == RT_ERROR_QUEUE_EMPTY) {
   //printf("queue was empty upon processing event\n");
  }
  else if (err != RT_OK) {
   fprintf(stderr,"rtRemoteProcessSingleItem() returned %d\n", err);
  }
}

void rtRemoteCallback(void*)
{
  //printf("queueReadyHandler entered\n");
  static char temp[1];
  int ret = HANDLE_EINTR_EAGAIN(write(gPipefd[PIPE_WRITE], temp, 1));
  if (ret == -1)
    fprintf(stderr,"can't write to pipe");
}

bool initRt(GMainLoop* main_loop, GSource* source, rtObjectRef pipeline)
{
  rtError rc;
  // Use pipe mechanism to process rt events efficiently
  source = pipe_source_new(gPipefd, rtMainLoopCb, nullptr);
  g_source_attach(source, g_main_loop_get_context(main_loop));

  rtRemoteRegisterQueueReadyHandler( rtEnvironmentGetGlobal(), rtRemoteCallback, nullptr );

  rc = rtRemoteInit();
  if(rc != RT_OK) return false;

  const char* objectName = getenv("PX_WAYLAND_CLIENT_REMOTE_OBJECT_NAME");
  if (!objectName) objectName = "MEDIASOURCE_PIPELINE_RT";
  printf("Register RT object: %s\n", objectName);

  rc = rtRemoteRegisterObject(objectName, pipeline);

  return (rc == RT_OK);
}

std::string getExePath()
{
  char result[255];
  ssize_t count = readlink("/proc/self/exe", result, 255);
  std::string full_path =  std::string( result, (count > 0) ? count : 0 );
  return dirname((char*) full_path.c_str());
}

gboolean essosIteration(gint fd,GIOCondition condition,gpointer user_data)
{
  // This will be called on the glib main loop thread
  EssCtx* ctx = (EssCtx*) user_data;
  EssContextRunEventLoopOnce( ctx );

  return TRUE;
}

gboolean runEssosEventLoop(EssCtx* ctx)
{
  EssContextRunEventLoopOnce( ctx );
  return TRUE;
}

int main(int argc, char** argv) {

  // Use Essos to simplify connecting to a wayland display and getting keyboard input from wayland
  EssCtx* ctx = EssContextCreate();
  GMainLoop* g_main_loop = NULL;
  GSource* source = NULL;

  if (!ParseCommandLine(argc, argv)) {
    fprintf(stderr, "Failed to parse command line\n");
    return -1;
  }

  gst_init(&argc, &argv);

  //Tell Essos to use wayland so it connects to a wayland display
  if ( !EssContextSetUseWayland( ctx, true ) )
  {
    printf("Failed to connect to wayland display, exiting...\n");
    exit(1);
  }

  bool have_folder = false;
  struct stat st;
  lstat(files_path_.c_str(), &st);
  if (S_ISDIR(st.st_mode))
    have_folder = true;

  if (!have_folder) {
    files_path_ = getExePath() + "/mse_frames";
    printf("No directory passed or directory invalid, using default path:%s\n",files_path_.c_str());

    lstat(files_path_.c_str(), &st);
    if (!S_ISDIR(st.st_mode))
    {
      printf("Default path:%s not found, exiting...\n",files_path_.c_str());
      return 1;
    }
  }
  else {
    printf("Using path:%s\n",files_path_.c_str());
  }

  MediaSourcePipeline* pi = new MediaSourcePipeline(files_path_);
  //rtObjectRef piRef = pi;

  if (!pi->Start()) {
    fprintf(stderr, "Failed to start pipeline!\n");
    return 1;
  }

  if ( !EssContextSetKeyListener( ctx, pi, &keyListener ) )
  {
    printf("Failed to connect to essos key listener\n");
  }

  if ( !EssContextStart( ctx ) )
  {
    printf("Failed to start essos context\n");
    return 1;
  }

  // Create a GLib Main Loop and set it to run
  g_main_loop = g_main_loop_new(NULL, FALSE);

  if(!initRt(g_main_loop,source,pi))
  {
    fprintf(stderr, "Failed to init rt!\n");
    return 1;
  }

  // use the display fd to know when to process, so we don't waste cpu cycles
  // running on idle, doesn't seem to work on raspberry pi though...
 /* 
  GIOCondition cond = (GIOCondition) (G_IO_IN | G_IO_ERR | G_IO_HUP);
  gint fd = wl_display_get_fd((wl_display*)EssContextGetWaylandDisplay(ctx));
  g_unix_fd_add (fd,cond,essosIteration,ctx);
  */
  
  // works on pi and comcast devices but not as cpu effiecient
  g_idle_add ((GSourceFunc) runEssosEventLoop, ctx);

  g_main_loop_run(g_main_loop);

  /* Free resources */
  g_main_loop_unref(g_main_loop);

  rtRemoteShutdown();
  gst_deinit();
  g_source_unref(source);

  EssContextDestroy( ctx );

  return 0;
}
