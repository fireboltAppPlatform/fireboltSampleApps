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
#include "rtevents.h"

rtError EventEmitter::send(Event&& event) {
  auto handleEvent = [](gpointer data) -> gboolean {
    EventEmitter& self = *static_cast<EventEmitter*>(data);

    while (!self.m_eventQueue.empty())
    {
      rtObjectRef obj = self.m_eventQueue.front();
      self.m_eventQueue.pop();

      rtError rc = self.m_emit.send(obj.get<rtString>("name"), obj);
      ASSERT(RT_OK == rc);
    }

    self.m_timeoutId = 0;
    return G_SOURCE_REMOVE;
  };

  m_eventQueue.push(event.object());

  if (m_timeoutId == 0) {
    m_timeoutId = g_timeout_add(0, handleEvent, (void*) this);
  }

  return RT_OK;
}
