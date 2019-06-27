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
#ifndef _COMMON_RT_EVENTS_H_
#define _COMMON_RT_EVENTS_H_

#include <queue>
#include <glib.h>
#include <rtRemote.h>
#include "logger.h"

class Event
{
protected:
    rtObjectRef m_object;
    rtString name() const
    {
        return m_object.get<rtString>("name");
    }
    rtObjectRef object() const
    {
        return m_object;
    }
    Event(rtObjectRef o) : m_object(o)
    {
    }
public:
    Event(const char* eventName) : m_object(new rtMapObject)
    {
        m_object.set("name", eventName);
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
    { }

    ~EventEmitter() {
      if (m_timeoutId != 0)
        g_source_remove(m_timeoutId);
    }

    rtError setListener(const char* eventName, rtIFunction* f)
    {
        return m_emit->addListener(eventName, f);//call addListener instead of setListener due to marksForDelete bug in rtObject.cpp
    }
    rtError delListener(const char* eventName, rtIFunction* f)
    {
        return m_emit->delListener(eventName, f);
    }
    rtError send(Event&& event);
private:
    rtEmitRef m_emit;
    std::queue<rtObjectRef> m_eventQueue;
    int m_timeoutId;
};

#endif
