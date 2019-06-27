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

#include <pthread.h>
#include <rtLog.h>
#include <rtRemote.h>
#include <rtError.h>
#include <assert.h>

#include <sys/types.h>                                                                           
#include <sys/syscall.h>

#include "RtUtils.h"

/*
static void rtRemoteLogHandler(rtLogLevel level, const char* file, int line, int threadId, char* message) {
 switch (level) {
   case RT_LOG_DEBUG: Log::debug(TRACE_SYSTEM, "RT LOG line:%d, threadId:%d : %s", message); break;
   case RT_LOG_INFO:  Log::info(TRACE_SYSTEM, "RT LOG line:%d, threadId:%d : %s", message); break;
   case RT_LOG_WARN:  Log::warn(TRACE_SYSTEM, "RT LOG line:%d, threadId:%d : %s", message); break;
   case RT_LOG_ERROR: Log::error(TRACE_SYSTEM, "RT LOG line:%d, threadId:%d : %s", message); break;
   case RT_LOG_FATAL: Log::fatal(TRACE_SYSTEM, "RT LOG line:%d, threadId:%d : %s", message); break;
   default: Log::info(TRACE_SYSTEM, "RT LOG line:%d, threadId:%d : %s", message); break;
 }
}
*/

RtUtils::RtUtils() : mStarted(false), mEventEmitter(), mRemoteReady(true)
{
}

rtError RtUtils::initRt()
{
   rtError rc;

   memset(&mThreadData,0,sizeof(RtProcessThreadData));
   pthread_cond_init(&mThreadData.mCond,NULL);
   pthread_mutex_init(&mThreadData.mMutex,NULL);
   mThreadData.mRunning = true;
   pthread_create(&mThreadData.mThread, NULL, RtUtils::RtMessageThread, this);

   //TODO: Use and test log handler
   //rtLogSetLogHandler(rtRemoteLogHandler);
   rtRemoteRegisterQueueReadyHandler( rtEnvironmentGetGlobal(), rtRemoteCallback, this);

   rc = rtRemoteInit();
   if(rc != RT_OK)
   {
       printf("Failed to init rt!\n");
       return rc;
   }
}

RtUtils::~RtUtils()
{
   setDedicatedThreadRunning(false);
   void *returnValue;

   pthread_mutex_lock(&mThreadData.mMutex);
   pthread_cond_signal(&mThreadData.mCond);
   pthread_mutex_unlock(&mThreadData.mMutex);

   pthread_join(mThreadData.mThread, &returnValue);
   pthread_cond_destroy(&mThreadData.mCond);
   pthread_mutex_destroy(&mThreadData.mMutex);
   printf("Dedicated RT message thread exited!\n");

   rtError rc = RT_OK;
   printf("before rt remote shutdown\n"); fflush(stdout);
   // TODO: Revisit the mRemoteReady check, if px doesn't connect or isn't destroyed
   // the shutdown will hang unless a signal is sent again. mRemoteReady is only set
   // if the remote object actually communicated with us.
   if(mRemoteReady)
   {
      ProcessRtItem();
      rc = rtRemoteShutdown();
   }
   if(rc != RT_OK)
      printf("Failed to shutdown rt!\n");
   printf("after rt remote shutdown\n"); fflush(stdout);
}

void RtUtils::ProcessRtItem()
{
    rtError err = RT_OK;
    while(err == RT_OK) { // empty the queue
        err = rtRemoteProcessSingleItem();
        if (err == RT_ERROR_QUEUE_EMPTY)
            printf("queue was empty upon processing event\n");
        else if (err != RT_OK)
            printf("rtRemoteProcessSingleItem() returned %d\n", err);
    }
}

void * RtUtils::RtMessageThread(void * ctx)
{
    RtUtils* rtUtils = static_cast<RtUtils*>(ctx);

    pthread_mutex_lock(&rtUtils->mThreadData.mMutex);
    while(rtUtils->mThreadData.mRunning)
    {
        pthread_cond_wait(&rtUtils->mThreadData.mCond, &rtUtils->mThreadData.mMutex);

        printf("rtRemoteProcessSingleItem()\n");
        rtUtils->ProcessRtItem();
    }
    pthread_mutex_unlock(&rtUtils->mThreadData.mMutex);

    pthread_exit(NULL);
}

void RtUtils::setDedicatedThreadRunning(bool running)
{
    pthread_mutex_lock(&mThreadData.mMutex);
    mThreadData.mRunning = running;
    pthread_mutex_unlock(&mThreadData.mMutex);
}

bool RtUtils::isDedicatedThreadRunning()
{
    bool isRunning = false;

    pthread_mutex_lock(&mThreadData.mMutex);
    isRunning = mThreadData.mRunning;
    pthread_mutex_unlock(&mThreadData.mMutex);

    return isRunning;
}

void RtUtils::rtRemoteCallback(void* ctx)
{
  RtUtils* rtUtils = static_cast<RtUtils*>(ctx);

  pthread_mutex_lock(&rtUtils->mThreadData.mMutex);
  pthread_cond_signal(&rtUtils->mThreadData.mCond);
  pthread_mutex_unlock(&rtUtils->mThreadData.mMutex);
}

void RtUtils::setStarted(bool started)
{
    mStarted = started;
}

void RtUtils::send(Event* event)
{
    mEventEmitter.send(event);
}

void RtUtils::EventEmitter::processEvents()
{
  pthread_mutex_lock(&mMutex);
  while (!mEventQueue.empty())
  {
      rtObjectRef obj = mEventQueue.front();
      mEventQueue.pop();

      rtError rc = m_emit.send(obj.get<rtString>("name"), obj);
      if(rc == RT_OK)
          printf("SENDING EVENT SUCCEDED!\n");
      else
          printf("SENDING EVENT FAILED!\n");

      assert(RT_OK == rc);
  }
  pthread_mutex_unlock(&mMutex);
}

rtError RtUtils::EventEmitter::send(Event* event) {
  pthread_mutex_lock(&mMutex);
  mEventQueue.push(event->object());
  pthread_mutex_unlock(&mMutex);

  if(mRemoteReady)
  {
     processEvents();
  }

  return RT_OK;
}

rtError RtUtils::setListener(rtString eventName, const rtFunctionRef& f)
{
  return mEventEmitter.setListener(eventName, f);
}

rtError RtUtils::delListener(rtString  eventName, const rtFunctionRef& f)
{
  return mEventEmitter.delListener(eventName, f);
}

void RtUtils::remoteObjectReady()
{
    mRemoteReady=true;
    mEventEmitter.remoteObjectReady();
}
