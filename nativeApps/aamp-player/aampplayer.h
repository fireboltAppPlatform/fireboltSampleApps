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
#ifndef AAMPMEDIAPLAYER_H
#define AAMPMEDIAPLAYER_H

#include <string>
#include <intrect.h>
#include <rtRemote.h>
#include <rtError.h>
#include <map>
#include "rtevents.h"
#include "intrect.h"

class AAMPListener;
class AAMPEvent;
class PlayerInstanceAAMP;
class AAMPPlayer;

enum TuneState {
    TuneNone,
    TuneStart,
    TuneStop
};

//Events
struct OnMediaOpenedEvent: public Event
{
    OnMediaOpenedEvent(const char* mediaType, int duration, int width, int height, const char* availableSpeeds, const char* availableAudioLanguages, const char* availableClosedCaptionsLanguages) : Event("onMediaOpened")
    {
        m_object.set("mediaType", mediaType);
        m_object.set("duration", duration);
        m_object.set("width", width);
        m_object.set("height", height);
        m_object.set("availableSpeeds", availableSpeeds);
        m_object.set("availableAudioLanguages", availableAudioLanguages);
        m_object.set("availableClosedCaptionsLanguages", availableClosedCaptionsLanguages);
        //TODO handle the following 2
        m_object.set("customProperties", "");
        m_object.set("mediaSegments", "");
    }
};


struct OnErrorEvent: public Event
{
    OnErrorEvent(uint32_t code, const std::string& description) : Event("onError")
    {
        m_object.set("code", code);
        m_object.set("description", rtString(description.c_str()));
    }
};

struct OnSpeedChangeEvent: public Event
{
    OnSpeedChangeEvent(double speed) : Event("onSpeedChange")
    {
        m_object.set("speed", speed);
    }
};

struct OnBitrateChanged: public Event
{
    OnBitrateChanged(int bitrate, const char* reason) : Event("onBitrateChanged")
    {
        m_object.set("bitrate", bitrate);
        m_object.set("reason", reason);
    }
};

class AAMPPlayer : public rtObject
{
public:
	rtDeclareObject(AAMPPlayer, rtObject);
    rtProperty(url, currentURL, setCurrentURL, rtString);
    rtMethod4ArgAndNoReturn("setVideoRectangle", setVideoRectangle, int32_t, int32_t, int32_t, int32_t);
    rtMethodNoArgAndNoReturn("stop", stop);
    rtMethod2ArgAndNoReturn("on", setListener, rtString, rtFunctionRef);
    rtMethod2ArgAndNoReturn("delListener", delListener, rtString, rtFunctionRef);
    rtMethodNoArgAndNoReturn("destroy", destroy);
    rtMethodNoArgAndNoReturn("suspend", suspend);
    rtMethodNoArgAndNoReturn("resume", resume);

    virtual void onInit();

    //rtRemote properties
    rtError currentURL(rtString& s) const;
    rtError setCurrentURL(rtString const& s);
    
    //rtRemote methods
    rtError setVideoRectangle(int32_t x, int32_t y, int32_t w, int32_t h);
    rtError stop();
    rtError setListener(rtString eventName, const rtFunctionRef& f);
    rtError delListener(rtString  eventName, const rtFunctionRef& f);
    rtError destroy();
    rtError suspend();
    rtError resume();

    EventEmitter& getEventEmitter();

    //status updates from impls
    void updateVideoMetadata(const std::string& languages, const std::string& speeds, int duration, int width, int height);
    TuneState getTuneState()
    {
        return m_tuneState;
    }
    void setTuneState(TuneState state)
    {
        m_tuneState = state;
    }
    AAMPPlayer();
    ~AAMPPlayer();
    static bool canPlayURL(const std::string& url);

    bool doCanPlayURL(const std::string& url);
    void doInit();
    void doLoad(const std::string& url);
    void doSetVideoRectangle(const IntRect& rect);
    void doSetAudioLanguage(std::string& lang);
    void doStop(); 
    
private:
	TuneState m_tuneState;
    rtError startQueuedTune();

    AAMPPlayer* m_pImpl; 
    EventEmitter m_eventEmitter;

    std::string m_currentURL;
    IntRect m_videoRect;
    bool m_urlQueued;

    //status info
    std::string m_availableAudioLanguages;
    std::string m_availableClosedCaptionsLanguages;
    std::string m_availableSpeeds;
    static bool setContentType(const std::string &uri, std::string& contentType);
    PlayerInstanceAAMP* m_aampInstance;
    AAMPListener* m_aampListener;
    friend class AAMPListener;
};

#endif
