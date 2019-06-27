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
#include "aampplayer.h"
#include <glib.h>
#include <limits>
#include <cmath>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include "libIBus.h"
#include "intrect.h"
#include "logger.h"
#include "main_aamp.h"

struct ReleaseOnScopeEnd {
    rtObject& obj;
    ReleaseOnScopeEnd(rtObject& o) : obj(o)
    {
    }
    ~ReleaseOnScopeEnd()
    {
        obj.Release();
    }
};

rtDefineObject    (AAMPPlayer, rtObject);
rtDefineProperty  (AAMPPlayer, url);
rtDefineMethod    (AAMPPlayer, setVideoRectangle);
rtDefineMethod    (AAMPPlayer, stop);
rtDefineMethod    (AAMPPlayer, setListener);
rtDefineMethod    (AAMPPlayer, delListener);
rtDefineMethod    (AAMPPlayer, destroy);
rtDefineMethod    (AAMPPlayer, suspend);
rtDefineMethod    (AAMPPlayer, resume);

#define CALL_ON_MAIN_THREAD(body) \
    do { \
        g_timeout_add_full( \
            G_PRIORITY_HIGH, \
            0, \
            [](gpointer data) -> gboolean \
            { \
                AAMPPlayer &self = *static_cast<AAMPPlayer*>(data); \
                body \
                return G_SOURCE_REMOVE; \
            }, \
            this, \
            0); \
    } while(0)


class AAMPListener : public AAMPEventListener
{
public:
    AAMPListener(AAMPPlayer* player, PlayerInstanceAAMP* aamp) : m_player(player), m_aamp(aamp)
    {
        reset();
    }
    void reset()
    {
        m_eventTuned = m_eventPlaying = m_didProgress = m_tuned = false;
        m_lastPosition = std::numeric_limits<double>::max();
        m_duration = 0;
        m_tuneStartTime = g_get_monotonic_time();
    }
    void Event(const AAMPEvent & e)
    {
        switch (e.type)
        {
        case AAMP_EVENT_TUNED:
            LOG_INFO("event tuned");
            m_eventTuned = true;
            break;
        case AAMP_EVENT_TUNE_FAILED:
            LOG_INFO("tune failed. code:%d desc:%s failure:%d", e.data.mediaError.code, e.data.mediaError.description, e.data.mediaError.failure);
            m_player->getEventEmitter().send(OnErrorEvent(e.data.mediaError.code, e.data.mediaError.description));
            break;
        case AAMP_EVENT_SPEED_CHANGED:
            m_player->getEventEmitter().send(OnSpeedChangeEvent(e.data.speedChanged.rate));
            break;
        case AAMP_EVENT_EOS:
            m_player->stop();//automatically call stop on player
            break;
        case AAMP_EVENT_PLAYLIST_INDEXED:
            break;
        case AAMP_EVENT_JS_EVENT:
            break;
        case AAMP_EVENT_VIDEO_METADATA:
        {
            std::string lang;
            for(int i = 0; i < e.data.metadata.languageCount; ++i)
            {
                if(i > 0)
                    lang.append(",");
                lang.append(e.data.metadata.languages[i]);
            }
            m_player->updateVideoMetadata(lang, "-64,-32,-16,-4,-1,0,1,4,16,32,64", e.data.metadata.durationMiliseconds, e.data.metadata.width, e.data.metadata.height);
            m_duration = e.data.metadata.durationMiliseconds;
            break;
        }
        case AAMP_EVENT_ENTERING_LIVE:
            break;
        case AAMP_EVENT_BITRATE_CHANGED:
            LOG_INFO("bitrate changed");
            m_player->getEventEmitter().send(OnBitrateChanged(e.data.bitrateChanged.bitrate, e.data.bitrateChanged.description));
            break;
        case AAMP_EVENT_TIMED_METADATA:
            break;
        case AAMP_EVENT_STATUS_CHANGED:
            switch(e.data.stateChanged.state)
            {
	            case eSTATE_IDLE: break;
	            case eSTATE_INITIALIZING: break;
	            case eSTATE_INITIALIZED: break;
	            case eSTATE_PREPARING: break;
	            case eSTATE_PREPARED: break;
	            case eSTATE_BUFFERING: break;
	            case eSTATE_PAUSED: 
                    break;
	            case eSTATE_SEEKING: break;
	            case eSTATE_PLAYING: 
                    if(!m_tuned)
                    {
                        LOG_INFO("event playing");
                        m_eventPlaying = true;
                    }
                    break;
	            case eSTATE_STOPPING: break;
	            case eSTATE_STOPPED: break;
	            case eSTATE_COMPLETE: break;
	            case eSTATE_ERROR: break;
	            case eSTATE_RELEASED: break;
            }
            break;
        default:
            break;
        }
    }
private:
    AAMPPlayer* m_player;
    PlayerInstanceAAMP* m_aamp;
    bool m_eventTuned, m_eventPlaying, m_didProgress, m_tuned;
    int m_duration;
    double m_lastPosition;
    gint64 m_tuneStartTime;
};

// rtRemote methods
void AAMPPlayer::onInit()
{
    LOG_INFO("%s\n", __PRETTY_FUNCTION__);
    
    setCurrentURL("http://nh.lab.xcal.tv/integration/files/neil/master.m3u8");
}

// rtRemote properties
rtError AAMPPlayer::currentURL(rtString& s) const
{
    s = m_currentURL.c_str();
    return RT_OK;
}

rtError AAMPPlayer::startQueuedTune()
{
    LOG_INFO("enter %s\n", m_currentURL.c_str());

    m_urlQueued = false;

    if(AAMPPlayer::canPlayURL(m_currentURL))
    {
        LOG_INFO("Creating AAMPPlayer");
    }
    else
    {        
        LOG_WARNING("Unsupported media type!");
        return RT_FAIL;
    }

    doInit();
    
    CALL_ON_MAIN_THREAD (
        LOG_INFO("Tuning player to %s\n", self.m_currentURL.c_str());
        self.setTuneState(TuneStart);
        self.doLoad(self.m_currentURL);
        self.doSetVideoRectangle(self.m_videoRect);
    );
	return RT_OK;
}

rtError AAMPPlayer::setCurrentURL(rtString const& s)
{
    m_currentURL = s.cString();
    LOG_INFO("enter %s\n", m_currentURL.c_str());
    m_urlQueued = true;

    if(getTuneState() == TuneStart)
    {
        LOG_INFO("%s: stopping player\n");
        stop();
        return RT_OK;
    }
    else if(getTuneState() == TuneStop)
    {
        LOG_INFO("%s: waiting on player to stop\n");
        return RT_OK;
    }

    LOG_INFO("calling startQueuedTune");
    return startQueuedTune();
}

rtError AAMPPlayer::setVideoRectangle(int32_t x, int32_t y, int32_t w, int32_t h)
{
    LOG_INFO("%s %d %d %d %d\n", __PRETTY_FUNCTION__, x, y, w, h);
    m_videoRect = IntRect(x,y,w,h);
    
	CALL_ON_MAIN_THREAD(self.doSetVideoRectangle(self.m_videoRect););
	
    return RT_OK;
}


rtError AAMPPlayer::stop()
{
    LOG_INFO("%s\n", __PRETTY_FUNCTION__);
    
	if(getTuneState() == TuneStop)
	{
		return RT_OK;
	}
        
    setTuneState(TuneStop);
    CALL_ON_MAIN_THREAD(

		self.doStop();
		self.setTuneState(TuneNone);

        if(self.m_urlQueued)
        {
            LOG_INFO("calling startQueuedTune");
            self.startQueuedTune();
        }
    );
    return RT_OK;
}

rtError AAMPPlayer::setListener(rtString eventName, const rtFunctionRef& f)
{
    LOG_INFO("%s\n", __PRETTY_FUNCTION__);
    return m_eventEmitter.setListener(eventName, f);
}

rtError AAMPPlayer::delListener(rtString  eventName, const rtFunctionRef& f)
{
    LOG_INFO("%s\n", __PRETTY_FUNCTION__);
    return m_eventEmitter.delListener(eventName, f);
}

rtError AAMPPlayer::destroy()
{
    LOG_INFO("%s\n", __PRETTY_FUNCTION__);
    exit(0);
}

EventEmitter& AAMPPlayer::getEventEmitter()
{
    LOG_INFO("AAMPPlayer::getEventEmitter");
    return m_eventEmitter;
}

void AAMPPlayer::updateVideoMetadata(const std::string& languages, const std::string& speeds, int duration, int width, int height)
{
    LOG_INFO("AAMPPlayer::updateVideoMetadata");
    m_availableAudioLanguages = languages;
    m_availableSpeeds = speeds;
#ifndef DISABLE_CLOSEDCAPTIONS
    m_availableClosedCaptionsLanguages = m_closedCaptions.getAvailableTracks();
#endif
    std::string mediaType;//should be  "live", "liveTSB", or "recorded"
    getEventEmitter().send(OnMediaOpenedEvent(mediaType.c_str(), duration, width, height, m_availableSpeeds.c_str(), m_availableAudioLanguages.c_str(),  m_availableClosedCaptionsLanguages.c_str()));
}

rtError AAMPPlayer::suspend()
{
    LOG_INFO("AAMPPlayer is going to suspend\n");
    // Free all resources
    stop();
   
    return RT_OK;
}

rtError AAMPPlayer::resume()
{
    LOG_INFO("AAMPPlayer is going to resume\n");
    // Start playing again
    onInit();

    return RT_OK;
}

bool AAMPPlayer::canPlayURL(const std::string& url)
{
    std::string contentType;
    return AAMPPlayer::setContentType(url, contentType);
}


/**/
AAMPPlayer::AAMPPlayer() :     
    m_aampInstance(0),
    m_aampListener(0)
{
    LOG_INFO("AAMPPlayer ctor");
}

AAMPPlayer::~AAMPPlayer()
{
    if(m_aampListener)
        delete m_aampListener;
    if(m_aampInstance)
        delete m_aampInstance;
}


bool AAMPPlayer::doCanPlayURL(const std::string& url)
{
    return AAMPPlayer::canPlayURL(url);
}

void AAMPPlayer::doInit()
{
  LOG_INFO("AAMPPlayer started");
  m_aampInstance = new PlayerInstanceAAMP();
  m_aampListener = new AAMPListener(this,  m_aampInstance);
  m_aampInstance->RegisterEvents(m_aampListener);
  IARM_Bus_Init("AAMPPlayer");
  IARM_Bus_Connect();
}

void AAMPPlayer::doLoad(const std::string& url)
{
    if(m_aampListener)
    {
        m_aampListener->reset();
    }
    if(m_aampInstance)
    {
        m_aampInstance->Tune(url.c_str());
    }
}

void AAMPPlayer::doSetVideoRectangle(const IntRect& rect)
{
    if (rect.width() > 0 && rect.height() > 0)
    {
        LOG_INFO("set video rectangle: %dx%d %dx%d", rect.x(), rect.y(), rect.width(), rect.height());
        m_aampInstance->SetVideoRectangle(rect.x(), rect.y(), rect.width(), rect.height());
    }
}

void AAMPPlayer::doStop()
{
    LOG_INFO("stop()");
	if(m_aampInstance)
    {
		m_aampInstance->Stop();    
    }
}

bool AAMPPlayer::setContentType(const std::string &url, std::string& contentStr)
{
    LOG_INFO("enter %s", url.c_str());
    int contentId = 0;
    
    if(url.find(".m3u8") != std::string::npos)
    {
        if(url.find("cdvr") != std::string::npos)
        {
            contentId = 1;
            contentStr = "cdvr";
        }
        else if(url.find("col") != std::string::npos)
        {
            contentId = 2;
            contentStr = "vod";//and help
        }
        else if(url.find("linear") != std::string::npos)
        {
            contentId = 3;
            contentStr = "ip-linear";
        }
        else if(url.find("ivod") != std::string::npos)
        {
            contentId = 4;
            contentStr = "ivod";
        }
        else if(url.find("ip-eas") != std::string::npos)
        {
            contentId = 5;
            contentStr = "ip-eas";
        }
        else if(url.find("xfinityhome") != std::string::npos)
        {
            contentId = 6;
            contentStr = "camera";
        }
        else if(url.find(".mpd"))
        {
            contentId = 7;
            contentStr = "helio";
        }
    }

    LOG_INFO("returning %d", contentId);

    if(contentId)
    {
        return true;
    }
    else
    {
        contentStr = "unknown";
        return false;
    }
}

