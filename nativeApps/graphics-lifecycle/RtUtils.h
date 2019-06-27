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

#ifndef _RT_UTILS_H_
#define _RT_UTILS_H_
#include <queue>

#include <rtRemote.h>
#include <rtError.h>

class RtUtils {
public:
    class Event
    {
        protected:
            rtObjectRef m_object;
            rtString name() const
            {
                return m_object.get<rtString>("name");
            }
            Event(rtObjectRef o) : m_object(o)
            {
            }
        public:
            Event(const char* eventName) : m_object(new rtMapObject)
            {
                m_object.set("name", eventName);
            }
            rtObjectRef object() const
            {
                return m_object;
            }
            virtual ~Event() { }
            friend class EventEmitter;
    };

    class EventEmitter
    {
    public:
        EventEmitter()
            : m_emit(new rtEmit)
              , m_timeoutId(0)
              , mRemoteReady(false) { 
                  pthread_mutex_init(&mMutex, NULL); 
              }

        ~EventEmitter() {
            pthread_mutex_destroy(&mMutex);
        }

        rtError setListener(const char* eventName, rtIFunction* f)
        {
            return m_emit->setListener(eventName, f);
        }
        rtError delListener(const char* eventName, rtIFunction* f)
        {
            return m_emit->delListener(eventName, f);
        }
        void remoteObjectReady()
        {
            mRemoteReady = true;
            processEvents();
        }
        rtError send(Event* event);
    private:
        void processEvents();
        rtEmitRef m_emit;
        std::queue<rtObjectRef> mEventQueue;
        pthread_mutex_t mMutex;
        int m_timeoutId;
        bool mRemoteReady;
    };

    struct RtProcessThreadData
    {
        pthread_t   mThread;
        pthread_cond_t mCond;
        pthread_mutex_t mMutex;
        bool mRunning;
    };

    RtUtils();
    virtual ~RtUtils();
    virtual rtError initRt();

    static void rtRemoteCallback(void* ctx);
    void setStarted(bool started);
    void send(Event* event);
    rtError setListener(rtString eventName, const rtFunctionRef& f);
    rtError delListener(rtString  eventName, const rtFunctionRef& f);
    void remoteObjectReady();

private:
    void setDedicatedThreadRunning(bool running);
    bool isDedicatedThreadRunning();
    static void * RtMessageThread(void * ctx);
    void ProcessRtItem();
    RtProcessThreadData mThreadData;
    bool mStarted;
    EventEmitter mEventEmitter;
    bool mRemoteReady;
};

#endif /* _NF_RT_UTILS_H_ */
