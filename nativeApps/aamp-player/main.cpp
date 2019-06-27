/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2018 RDK Management
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
#include <unistd.h>
#include <stdio.h>
#include <sys/prctl.h>
#include <sys/stat.h>
#include <glib.h>
#include <gst/gst.h>
#include <rtRemote.h>
#include "aampplayer.h"
#include "logger.h"
#include "glib_tools.h"

#ifdef ENABLE_SIGNAL_HANDLE
#include <signal.h>
#ifdef Q_OS_LINUX
//#include <unistd.h>
#endif
#endif

GMainLoop *gMainLoop = 0;
int gPipefd[2];

#ifdef ENABLE_BREAKPAD
#include <client/linux/handler/exception_handler.h>

namespace
{

bool breakpadCallback(const google_breakpad::MinidumpDescriptor& /*descriptor*/, void* /*context*/, bool succeeded) {
  rtRemoteShutdown();
  return succeeded;
}

void installExceptionHandler() {
  static google_breakpad::ExceptionHandler* excHandler = NULL;
  delete excHandler;
  excHandler = new google_breakpad::ExceptionHandler(
      google_breakpad::MinidumpDescriptor("/opt/minidumps"),
      NULL,
      breakpadCallback,
      NULL,
      true,
      -1);
}

}  // namespace
#endif

#ifdef ENABLE_SIGNAL_HANDLER
void signalHandler(int signum)
{
    printf("signalHandler %d\n", signum);
    rtRemoteShutdown();
    signal(signum, SIG_DFL);
    kill(getpid(), signum);
}
#endif

void rtMainLoopCb(void*)
{
    rtError err = rtRemoteProcessSingleItem();
    if(err != RT_OK && err != RT_ERROR_QUEUE_EMPTY)
    {
        LOG_WARNING("rtRemoteProcessSingleItem() returned %d", err);
    }
}

void rtRemoteCallback(void*)
{
  //LOG_TRACE("queueReadyHandler entered");
  static char temp[1];
  int ret = HANDLE_EINTR_EAGAIN(write(gPipefd[PIPE_WRITE], temp, 1));
  if (ret == -1)
    perror("can't write to pipe");
}

int main(int argc, char *argv[]) {

  prctl(PR_SET_PDEATHSIG, SIGHUP);

#ifdef ENABLE_BREAKPAD
  installExceptionHandler();
#endif

#ifdef ENABLE_SIGNAL_HANDLER
  signal(SIGINT,  signalHandler);
  signal(SIGQUIT, signalHandler);
  signal(SIGTERM, signalHandler);
  signal(SIGILL,  signalHandler);
  signal(SIGABRT, signalHandler);
  signal(SIGFPE,  signalHandler);
  signal(SIGSEGV,  signalHandler);
#endif

  gst_init(0, 0);
  log_init();

  LOG_INFO("Init rtRemote");
  rtError rc;
  gMainLoop = g_main_loop_new(0, FALSE);

  auto *source = pipe_source_new(gPipefd, rtMainLoopCb, nullptr);
  g_source_attach(source, g_main_loop_get_context(gMainLoop));

  rtRemoteRegisterQueueReadyHandler( rtEnvironmentGetGlobal(), rtRemoteCallback, nullptr );

  rc = rtRemoteInit();
  ASSERT(rc == RT_OK);

  const char* objectName = getenv("PX_WAYLAND_CLIENT_REMOTE_OBJECT_NAME");
  if (!objectName) objectName = "aampplayer";
  LOG_INFO("Register: %s", objectName);

  rtObjectRef playerRef = new AAMPPlayer;
  rc = rtRemoteRegisterObject(objectName, playerRef);
  ASSERT(rc == RT_OK);
  
  {
    LOG_INFO("Start main loop");
    g_main_loop_run(gMainLoop);
    g_main_loop_unref(gMainLoop);
    gMainLoop = 0;
  }

  playerRef = nullptr;

  LOG_INFO("Shutdown");
  rc = rtRemoteShutdown();
  ASSERT(rc == RT_OK);

  gst_deinit();
  g_source_unref(source);

  return 0;
}
