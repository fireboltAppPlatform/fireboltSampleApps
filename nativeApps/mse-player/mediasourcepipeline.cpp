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

#include "mediasourcepipeline.h"
#include "GstMSESrc.h"

#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <linux/input.h>

#include <algorithm>
#include <cmath>
#include <string>
#include <cstring>
#include <vector>
#include <unistd.h>

rtDefineObject (MediaSourcePipeline, rtObject);
rtDefineMethod (MediaSourcePipeline, suspend);
rtDefineMethod (MediaSourcePipeline, resume);

namespace {
const int kVideoReadDelayMs =
    25;  // add some artificial network latency to video frame reads
const int kAudioReadDelayMs =
    10;  // add some artificial network latency to audio frame reads
const int kStatusDelayMs =
    50;  // update interval for checking status, like playback position
const float kSeekEndDeltaSecs =
    0.0f;  // delta time from end of current playback file to trigger a seek
const int kChunkDemuxerSeekDelayMs =
    50;  // simulated chunk demuxer seek latency before a seek is completed
const int kPlaybackPositionHistorySize =
    10;  // size of history for collecting playback position to determine
        // end of a raw frame file playback
const int64_t kPlaybackPositionUpdateIntervalMs =
    1000;  // Update interval in milliseconds
           // of when playback position is outputted to stdout

}  // namespace

// #define DEBUG_PRINTS // define to get more verbose printing

unsigned getGstPlayFlag(const char* nick)
{
  static GFlagsClass* flagsClass = static_cast<GFlagsClass*>(g_type_class_ref(g_type_from_name("GstPlayFlags")));

  GFlagsValue* flag = g_flags_get_value_by_nick(flagsClass, nick);
  if (!flag)
    return 0;

  return flag->value;
}

static gboolean MessageCallbackStatic(GstBus*,
                                      GstMessage* message,
                                      MediaSourcePipeline* msp) {
  return msp->HandleMessage(message);
}

static void StartFeedStatic(GstAppSrc* appsrc,
                            guint size,
                            MediaSourcePipeline* msp) {
  msp->StartFeedingAppSource(appsrc);
}

static void StopFeedStatic(GstAppSrc* appsrc, MediaSourcePipeline* msp) {
  msp->StopFeedingAppSource(appsrc);
}

static gboolean SeekDataStatic(GstAppSrc* appsrc,
                               guint64 position,
                               MediaSourcePipeline* msp) {
  msp->SetNewAppSourceReadPosition(appsrc, position);
  return TRUE;
}

static void OnAutoPadAddedMediaSourceStatic(GstElement* decodebin2,
                                            GstPad* pad,
                                            MediaSourcePipeline* msp) {
  msp->OnAutoPadAddedMediaSource(decodebin2, pad);
}

static void OnAutoElementAddedMediaSourceStatic(GstBin* bin,
                                                GstElement* element,
                                                MediaSourcePipeline* msp) {
  msp->OnAutoElementAddedMediaSource(element);
}

static gboolean readVideoFrameStatic(MediaSourcePipeline* msp) {
  return msp->ReadVideoFrame();
}

static gboolean readAudioFrameStatic(MediaSourcePipeline* msp) {
  return msp->ReadAudioFrame();
}

static gboolean StatusPollStatic(MediaSourcePipeline* msp) {
  return msp->StatusPoll();
}

static gboolean ChunkDemuxerSeekStatic(MediaSourcePipeline* msp) {
  return msp->ChunkDemuxerSeek();
}

static void sourceChangedCallback(GstElement* element, GstElement* source, gpointer data)
{
  MediaSourcePipeline* msp = (MediaSourcePipeline*) data;
  msp->sourceChanged();
}

void MediaSourcePipeline::sourceChanged()
{
  if (!source_)
    g_object_get(pipeline_, "source", &source_, NULL);

  printf("sourceChanged!:%p\n",source_);
}

gboolean MediaSourcePipeline::HandleMessage(GstMessage* message) {
  GError* error;
  gchar* debug;
  switch (GST_MESSAGE_TYPE(message)){
    case GST_MESSAGE_ERROR:
      gst_message_parse_error(message, &error, &debug);
      printf("gstBusCallback() error! code: %d, %s, Debug: %s\n", error->code, error->message, debug);
      g_error_free(error);
      g_free(debug);
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipeline_), GST_DEBUG_GRAPH_SHOW_ALL, "error-pipeline");
      break;
    case GST_MESSAGE_WARNING:
      gst_message_parse_warning(message, &error, &debug);
      printf("gstBusCallback() warning! code: %d, %s, Debug: %s\n", error->code, error->message, debug);
      g_error_free(error);
      g_free(debug);
      break;
    case GST_MESSAGE_EOS: {

      printf("Gstreamer EOS message received\n");
      break;
    }
    case GST_MESSAGE_STATE_CHANGED:
      gchar* filename;
      GstState oldstate, newstate, pending;
      gst_message_parse_state_changed (message, &oldstate, &newstate, &pending);


      // Ignore messages not coming directly from the pipeline.
      if (GST_ELEMENT(GST_MESSAGE_SRC(message)) != pipeline_)
        break;

      filename = g_strdup_printf("%s-%s", gst_element_state_get_name(oldstate), gst_element_state_get_name(newstate));
      GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipeline_), GST_DEBUG_GRAPH_SHOW_ALL, filename);
      g_free(filename);

      // get the name and state
      if (GST_MESSAGE_SRC_NAME(message)){
        //printf("gstBusCallback() Got state message from %s\n", GST_MESSAGE_SRC_NAME (message));
      }
      printf("gstBusCallback() old_state %s, new_state %s, pending %s\n",
                gst_element_state_get_name (oldstate), gst_element_state_get_name (newstate), gst_element_state_get_name (pending));

      if (oldstate == GST_STATE_NULL && newstate == GST_STATE_READY) {
      } else if (oldstate == GST_STATE_READY && newstate == GST_STATE_PAUSED) {
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipeline_), GST_DEBUG_GRAPH_SHOW_ALL, "paused-pipeline");
        printf("Ready to Paused finished!\n");
      } else if (oldstate == GST_STATE_PAUSED && newstate == GST_STATE_PAUSED) {
      } else if (oldstate == GST_STATE_PAUSED && newstate == GST_STATE_PLAYING) {
        printf("Pipeline is now in play state!\n");
        GST_DEBUG_BIN_TO_DOT_FILE_WITH_TS(GST_BIN(pipeline_), GST_DEBUG_GRAPH_SHOW_ALL, "playing-pipeline");
      } else if (oldstate == GST_STATE_PLAYING && newstate == GST_STATE_PAUSED) {
         printf("Pipline finished from play to pause\n");
      } else if (oldstate == GST_STATE_PAUSED && newstate == GST_STATE_READY) {
      } else if (oldstate == GST_STATE_READY && newstate == GST_STATE_NULL) {
      }
      break;
    default:
        break;
    }
  return true;
}

bool MediaSourcePipeline::ShouldPerformSeek() {
  float seek_delta_secs =
      std::fabs(playback_position_secs_ - current_end_time_secs_);

  if (seek_delta_secs < kSeekEndDeltaSecs || playback_position_secs_ >= current_end_time_secs_)
    return true;

  // Since it's possible that the playback position won't reach the time
  // reported by the last possible frame timestamp, we will assume that we are
  // done with a frame file if we are playing and the playback position stays
  // the same for kPlaybackPositionHistorySize
  if (is_playing_ && IsPlaybackStalled())
    return true;

  return false;
}

bool MediaSourcePipeline::IsPlaybackOver() {
  // playback is over when there are no more files to play

  std::ostringstream counter_stream;
  counter_stream << (current_file_counter_ + 1);

  std::string audio_timestamp_path =
      frame_files_path_ + "/raw_audio_frames_" + counter_stream.str() + ".txt";
  std::string video_timestamp_path =
      frame_files_path_ + "/raw_video_frames_" + counter_stream.str() + ".txt";

  FILE* audio_timestamp_file = fopen(audio_timestamp_path.c_str(), "r");
  FILE* video_timestamp_file = fopen(video_timestamp_path.c_str(), "r");

  if (audio_timestamp_file == NULL && video_timestamp_file == NULL) {
    return true;
  } else {
    if (audio_timestamp_file)
      fclose(audio_timestamp_file);
    if (video_timestamp_file)
      fclose(video_timestamp_file);
    return false;
  }
}

bool MediaSourcePipeline::HasPlaybackAdvanced() {
  int32_t pos = current_playback_history_cnt_ - 1;
  if (pos < 0)
    pos = kPlaybackPositionHistorySize - 1;

  int32_t counter = 1;
  int64_t val = playback_position_history_[pos];
  bool did_advance = true;
  while (counter < kPlaybackPositionHistorySize) {
    pos--;
    if (pos < 0)
      pos = kPlaybackPositionHistorySize - 1;

    if (val <= playback_position_history_[pos] ||
        playback_position_history_[pos] == -1) {
      did_advance = false;
      break;
    }

    val = playback_position_history_[pos];
    ++counter;
  }

  return did_advance;
}

void MediaSourcePipeline::AddPlaybackPositionToHistory(int64_t position) {
  GstState state;
  gst_element_get_state(pipeline_, &state, 0, 0);

  if (state == GST_STATE_PLAYING) {
    playback_position_history_[current_playback_history_cnt_] = position;
    current_playback_history_cnt_ =
        (current_playback_history_cnt_ + 1) % kPlaybackPositionHistorySize;
  }
}

bool MediaSourcePipeline::IsPlaybackStalled() {
  int64_t first_pos = playback_position_history_[0];

  for (size_t i = 1; i < playback_position_history_.size(); i++) {
    if (playback_position_history_[i] != first_pos ||
        playback_position_history_[i] < 0)
      return false;
  }

  return true; //playback_started_;
}

void MediaSourcePipeline::finishPipelineLinkingAndStartPlaybackIfNeeded()
{
  if (source_ && !gst_mse_src_configured(source_)) {
     if(pipeline_type_ != kAudioOnly)
       gst_mse_src_register_player(source_, (GstElement*) appsrc_source_video_);
     if(pipeline_type_ != kVideoOnly)
       gst_mse_src_register_player(source_, (GstElement*) appsrc_source_audio_);

     gst_mse_src_configuration_done(source_);

     printf("Finished linking pipeline and putting it in play!\n");
     gst_element_set_state(pipeline_, GST_STATE_PLAYING);
     is_playing_ = true;
  }
}

gboolean MediaSourcePipeline::StatusPoll() {
  GstFormat fmt = GST_FORMAT_TIME;
  gint64 position = -1;

  // finish linking all of pipeline if we haven't yet
  finishPipelineLinkingAndStartPlaybackIfNeeded();

  if(pipeline_type_ != kAudioOnly) // westeros sink gives a more accurate position
    gst_element_query_position(video_sink_, fmt, &position);
  else
    gst_element_query_position(pipeline_, fmt, &position);

  position += (seek_offset_ * 1000);
  if (position != static_cast<gint64>(GST_CLOCK_TIME_NONE)) {
    AddPlaybackPositionToHistory(position);
    if (!playback_started_)
      playback_started_ = HasPlaybackAdvanced();
    playback_position_secs_ = (static_cast<double>(position) / GST_SECOND);

    static int64_t position_update_cnt = 0;
    if (position_update_cnt == 0)
      printf("playback position: %f secs\n", playback_position_secs_);

    position_update_cnt = (position_update_cnt + kStatusDelayMs) %
                          kPlaybackPositionUpdateIntervalMs;
  }

#ifdef DEBUG_PRINTS
  printf("playback started:%d\n", playback_started_);
#endif
  if (ShouldPerformSeek()) {
    if (IsPlaybackOver()) {
      printf("Current end time:%f\n", current_end_time_secs_);
      printf("Playback Complete! Starting over...\n");

      // reset file counter back to before beginning
      current_file_counter_ = -1;
      PerformSeek();
    } else {
      printf("Performing Seek!\n");
      PerformSeek();
    }
  }

  return TRUE;
}

gboolean MediaSourcePipeline::ReadVideoFrame() {
  if (seeking_) {
    video_frame_timeout_handle_ = 0;
    return FALSE;
  }

  AVFrame video_frame;
  ReadStatus read_status = GetNextFrame(&video_frame, kVideo);
#ifdef DEBUG_PRINTS
  printf("Video frame read status:%d\n", read_status);
#endif

  if (read_status != kFrameRead) {
    video_frame_timeout_handle_ = 0;
    return FALSE;
  }

#ifdef DEBUG_PRINTS
  float frame_time_seconds = video_frame.timestamp_us_ / 1000000.0f;
  printf("read video frame: time:%f secs, size:%d bytes\n",
         frame_time_seconds,
         video_frame.size_);
#endif

  PushFrameToAppSrc(video_frame, kVideo);

  return TRUE;
}

gboolean MediaSourcePipeline::ReadAudioFrame() {
  if (seeking_) {
    audio_frame_timeout_handle_ = 0;
    return FALSE;
  }

  AVFrame audio_frame;
  ReadStatus read_status = GetNextFrame(&audio_frame, kAudio);

#ifdef DEBUG_PRINTS
  printf("Audio frame read status:%d\n", read_status);
#endif

  if (read_status != kFrameRead) {
    audio_frame_timeout_handle_ = 0;
    return FALSE;
  }

#ifdef DEBUG_PRINTS
  float frame_time_seconds = audio_frame.timestamp_us_ / 1000000.0f;
  printf("read audio frame: time:%f secs, size:%d bytes\n",
         frame_time_seconds,
         audio_frame.size_);
#endif

  PushFrameToAppSrc(audio_frame, kAudio);

  return TRUE;
}

void MediaSourcePipeline::StartFeedingAppSource(GstAppSrc* p_src) {
  if (seeking_)
    return;

  bool start_up_reading_again = false;

  start_up_reading_again =
      !ShouldBeReading(p_src == appsrc_source_video_ ? kVideo : kAudio);
  if (start_up_reading_again)
    SetShouldBeReading(true, p_src == appsrc_source_video_ ? kVideo : kAudio);

  if (start_up_reading_again) {
    if (p_src == appsrc_source_video_) {
      video_frame_timeout_handle_ =
          g_timeout_add(kVideoReadDelayMs,
                        reinterpret_cast<GSourceFunc>(readVideoFrameStatic),
                        this);
    } else {  // audio
      audio_frame_timeout_handle_ =
          g_timeout_add(kAudioReadDelayMs,
                        reinterpret_cast<GSourceFunc>(readAudioFrameStatic),
                        this);
    }
  }
}

void MediaSourcePipeline::StopFeedingAppSource(GstAppSrc* p_src) {
  if (p_src == appsrc_source_video_) {
    if (video_frame_timeout_handle_) {
      g_source_remove(video_frame_timeout_handle_);
      video_frame_timeout_handle_ = 0;
    }
    SetShouldBeReading(false, kVideo);
  } else {
    if (audio_frame_timeout_handle_) {
      g_source_remove(audio_frame_timeout_handle_);
      audio_frame_timeout_handle_ = 0;
    }
    SetShouldBeReading(false, kAudio);
  }
}

void MediaSourcePipeline::SetNewAppSourceReadPosition(GstAppSrc* p_src,
                                                      guint64 position) {}

void MediaSourcePipeline::OnAutoPadAddedMediaSource(GstElement* element,
                                                    GstPad* pad) {
  GstCaps* caps;
  GstStructure* structure;
  const gchar* name;
  std::vector<GstElement*>::iterator src_it;
  std::vector<GstElement*>::iterator dest_it;

  caps = gst_pad_query_caps(pad,NULL);
  structure = gst_caps_get_structure(caps, 0);
  name = gst_structure_get_name(structure);

  // find the element in the pipeline that created the pad, then link it with
  // its neighbor
  if (g_strrstr(name, "video")) {
    src_it = std::find(
        ms_video_pipeline_.begin(), ms_video_pipeline_.end(), element);
    dest_it = src_it + 1;
    if (src_it != ms_video_pipeline_.end() &&
        dest_it != ms_video_pipeline_.end()) {
      if (gst_element_link(*src_it, *dest_it) == FALSE) {
        g_print("Couldn't link auto pad added elements in video pipeline\n");
      }
    }
  } else if (g_strrstr(name, "audio")) {
    src_it = std::find(
        ms_audio_pipeline_.begin(), ms_audio_pipeline_.end(), element);
    dest_it = src_it + 1;
    if (src_it != ms_audio_pipeline_.end() &&
        dest_it != ms_audio_pipeline_.end()) {
      if (gst_element_link(*src_it, *dest_it) == FALSE) {
        g_print("Couldn't link auto pad added elements in audio pipeline\n");
      }
    }
  }

  gst_caps_unref(caps);
}

void MediaSourcePipeline::OnAutoElementAddedMediaSource(GstElement* element) {
  // check for the dynamically adding of the real audio sink
  if (g_strrstr(GST_ELEMENT_NAME(element), "audio") &&
      g_strrstr(GST_ELEMENT_NAME(element), "sink")) {
    audio_sink_ = element;
  }
}

MediaSourcePipeline::MediaSourcePipeline(std::string frame_files_path)
  : frame_files_path_(frame_files_path)
{
    Init();
}

MediaSourcePipeline::~MediaSourcePipeline() { Destroy(); }

void MediaSourcePipeline::Init()
{
  current_file_counter_ = 0;
  current_video_file_ = NULL;
  current_video_timestamp_file_ = NULL;
  current_audio_file_ = NULL;
  current_audio_timestamp_file_ = NULL;
  seeking_ = false;
  pipeline_ = NULL;
  appsrc_source_video_ = NULL;
  appsrc_source_audio_ = NULL;
  video_sink_ = NULL;
  audio_sink_ = NULL;
  playback_position_secs_ = 0;
  current_end_time_secs_ = 0;
  video_frame_timeout_handle_ = 0;
  audio_frame_timeout_handle_ = 0;
  status_timeout_handle_ = 0;
  current_playback_history_cnt_ = 0;
  playback_started_ = false;
  is_playing_ = false;
  pipeline_type_ = kAudioVideo;
  source_ = NULL;
  appsrc_caps_video_ = NULL;
  appsrc_caps_audio_ = NULL;
  pause_before_seek_  = false;
  is_active_ = true;
  seek_offset_ = 0;
 

  memset(&should_be_reading_, 0, sizeof(should_be_reading_));

  playback_position_history_.resize(kPlaybackPositionHistorySize, 0);
  ResetPlaybackHistory();
}

bool MediaSourcePipeline::ShouldBeReading(AVType av) {
  return should_be_reading_[av];
}

void MediaSourcePipeline::SetShouldBeReading(bool is_reading, AVType av) {
  should_be_reading_[av] = is_reading;
}

bool MediaSourcePipeline::PushFrameToAppSrc(const AVFrame& frame, AVType type) {
  GstFlowReturn ret = GST_FLOW_OK;

  GstBuffer* gst_buffer = gst_buffer_new_wrapped(frame.data_, frame.size_);
  GstSample* sample = NULL;
  GST_BUFFER_TIMESTAMP(gst_buffer) = (frame.timestamp_us_ - seek_offset_) * 1000;

  if (type == kVideo)
  {
     sample = gst_sample_new(gst_buffer, appsrc_caps_video_, NULL, NULL);
     ret = gst_app_src_push_sample(GST_APP_SRC(appsrc_source_video_), sample);
  }
  else  // kAudio
  {
     sample = gst_sample_new(gst_buffer, appsrc_caps_audio_, NULL, NULL);
     ret = gst_app_src_push_sample(GST_APP_SRC(appsrc_source_audio_), sample);
  }

  gst_buffer_unref(gst_buffer);
  gst_sample_unref(sample);

  if (ret != GST_FLOW_OK) {
    fprintf(stderr, "APPSRC PUSH FAILED!\n");
    return false;
  }

  return true;
}

int64_t MediaSourcePipeline::GetCurrentStartTimeMicroseconds() const {
  std::ostringstream counter_stream;
  counter_stream << current_file_counter_;

  std::string audio_timestamp_path =
      frame_files_path_ + "/raw_audio_frames_" + counter_stream.str() + ".txt";
  std::string video_timestamp_path =
      frame_files_path_ + "/raw_video_frames_" + counter_stream.str() + ".txt";

  FILE* audio_timestamp_file = fopen(audio_timestamp_path.c_str(), "r");
  FILE* video_timestamp_file = fopen(video_timestamp_path.c_str(), "r");

  int64_t smallest_time_ms = -1;

  int64_t timestamp_us;
  int32_t frame_size;
  if (audio_timestamp_file) {
    if (fscanf(audio_timestamp_file,
               "%" PRId64 ",%d,",
               &timestamp_us,
               &frame_size) == 2) {
      smallest_time_ms = timestamp_us;
    }
    fclose(audio_timestamp_file);
  }

  if (video_timestamp_file) {
    if (fscanf(video_timestamp_file,
               "%" PRId64 ",%d,",
               &timestamp_us,
               &frame_size) == 2) {
      if (smallest_time_ms == -1)
        smallest_time_ms = timestamp_us;
      else
        smallest_time_ms = std::min(smallest_time_ms, timestamp_us);
    }
    fclose(video_timestamp_file);
  }

  return smallest_time_ms;
}

void MediaSourcePipeline::CalculateCurrentEndTime() {
  std::ostringstream counter_stream;
  counter_stream << current_file_counter_;
  current_end_time_secs_ = 0;
  bool have_audio = false;
  bool have_video = false;

  std::string audio_timestamp_path =
      frame_files_path_ + "/raw_audio_frames_" + counter_stream.str() + ".txt";
  std::string video_timestamp_path =
      frame_files_path_ + "/raw_video_frames_" + counter_stream.str() + ".txt";

  FILE* audio_timestamp_file = fopen(audio_timestamp_path.c_str(), "r");
  FILE* video_timestamp_file = fopen(video_timestamp_path.c_str(), "r");

  float greatest_time_secs = 0;
  float greatest_video_time_secs = 0;
  float greatest_audio_time_secs = 0;

  int64_t timestamp_us;
  int32_t frame_size;
  if (audio_timestamp_file) {
    while (fscanf(audio_timestamp_file,
                  "%" PRId64 ",%d,",
                  &timestamp_us,
                  &frame_size) == 2) {
      float timestamp_secs = timestamp_us / 1000000.0f;
      if (greatest_audio_time_secs < timestamp_secs)
        greatest_audio_time_secs = timestamp_secs;
    }
    fclose(audio_timestamp_file);
    have_audio = true;
  }

  if (video_timestamp_file) {
    while (fscanf(video_timestamp_file,
                  "%" PRId64 ",%d,",
                  &timestamp_us,
                  &frame_size) == 2) {
      float timestamp_secs = timestamp_us / 1000000.0f;
      if (greatest_video_time_secs < timestamp_secs)
        greatest_video_time_secs = timestamp_secs;
    }
    fclose(video_timestamp_file);
    have_video = true;
  }

  if (greatest_audio_time_secs > 0 && greatest_video_time_secs > 0)
    greatest_time_secs =
        std::min(greatest_audio_time_secs, greatest_video_time_secs);
  else if (greatest_audio_time_secs > 0)
    greatest_time_secs = greatest_audio_time_secs;
  else if (greatest_video_time_secs > 0)
    greatest_time_secs = greatest_video_time_secs;

#ifdef DEBUG_PRINTS
  printf("calculated end time, counter:%d, end time:%f\n",
         current_file_counter_,
         greatest_time_secs);
  printf("greatest audio time:%f, greatest video time:%f\n",
         greatest_audio_time_secs,
         greatest_video_time_secs);
#endif

  if(have_audio && have_video)
    pipeline_type_ = kAudioVideo;
  else if(have_audio)
    pipeline_type_ = kAudioOnly;
  else if(have_video)
    pipeline_type_ = kVideoOnly;

  if (greatest_time_secs > 0)
    current_end_time_secs_ = greatest_time_secs;
}

void MediaSourcePipeline::ResetPlaybackHistory() {
  for (int i = 0; i < kPlaybackPositionHistorySize; i++)
    playback_position_history_[i] = -1;
}

void MediaSourcePipeline::PerformSeek() {
  // put us in a seeking state and stop any reading of the current file(s)
  seeking_ = true;
  playback_started_ = false;
  bool did_pause = false;
  ResetPlaybackHistory();
  StopFeedingAppSource(appsrc_source_video_);
  StopFeedingAppSource(appsrc_source_audio_);
  CloseAllFiles();

  // go to the next file(s), and calculate the end time of the file av segment
  current_file_counter_++;
  CalculateCurrentEndTime();

  if(pause_before_seek_) {
    if (is_playing_) {
        did_pause = true;
        is_playing_ = false;
        DoPause();
    }
  }

  // have gstreamer perform a seek
  gboolean seek_succeeded = FALSE;

  int64_t seek_time_us = GetCurrentStartTimeMicroseconds();
  seek_offset_ = seek_time_us;
  GstClockTime seek_time_ns =
      seek_time_us * 1000;  // GstClockTime is a time in nanoseconds

/*
  seek_succeeded = gst_element_seek(
      pipeline_,
      1.0,
      GST_FORMAT_TIME,
      (GstSeekFlags)(GST_SEEK_FLAG_FLUSH | GST_SEEK_FLAG_ACCURATE),
      GST_SEEK_TYPE_SET,
      seek_time_ns,
      GST_SEEK_TYPE_NONE,
      GST_CLOCK_TIME_NONE);
  */

  // A seek is just a flush of the pipeline
  seek_succeeded = gst_element_send_event(source_, gst_event_new_flush_start());
  if (!seek_succeeded)
    printf("failed to send flush-start event\n");

  seek_succeeded = gst_element_send_event(source_, gst_event_new_flush_stop(TRUE));
  if (!seek_succeeded)
    printf("failed to send flush-stop event\n");

  if (!seek_succeeded) {
    printf("Failed to seek!\n");
  } else {
    // if here gstreamer successfully seeked, now we need to simulate the
    // mse source performing its own seek before we can
    // starting reading data again

    g_timeout_add(kChunkDemuxerSeekDelayMs,
                  reinterpret_cast<GSourceFunc>(ChunkDemuxerSeekStatic),
                  this);
  }

  if(pause_before_seek_) {
    if (did_pause) {
        is_playing_ = true;
        DoPause();
    }
  }
}

gboolean MediaSourcePipeline::ChunkDemuxerSeek() {
  seeking_ = false;

  StartFeedingAppSource(appsrc_source_video_);
  StartFeedingAppSource(appsrc_source_audio_);

  return FALSE;
}

void MediaSourcePipeline::DoPause() {
    g_print ("Setting state to %s\n", is_playing_ ? "PLAYING" : "PAUSE");
    gst_element_set_state (pipeline_, is_playing_? GST_STATE_PLAYING : GST_STATE_PAUSED);
}

void MediaSourcePipeline::CloseAllFiles() {
  if (current_video_file_)
    fclose(current_video_file_);
  if (current_video_timestamp_file_)
    fclose(current_video_timestamp_file_);
  if (current_audio_file_)
    fclose(current_audio_file_);
  if (current_audio_timestamp_file_)
    fclose(current_audio_timestamp_file_);

  current_video_file_ = current_video_timestamp_file_ = current_audio_file_ =
      current_audio_timestamp_file_ = NULL;
}

ReadStatus MediaSourcePipeline::GetNextFrame(AVFrame* frame, AVType type) {
  FILE* current_file = NULL;
  FILE* current_timestamp_file = NULL;
  std::ostringstream counter_stream;
  counter_stream << current_file_counter_;

  if (type == kAudio) {
    if (current_audio_file_ == NULL) {
      std::string audio_path = frame_files_path_ + "/raw_audio_frames_" +
                               counter_stream.str() + ".bin";
      std::string audio_timestamp_path = frame_files_path_ +
                                         "/raw_audio_frames_" +
                                         counter_stream.str() + ".txt";
      current_audio_file_ = fopen(audio_path.c_str(), "rb");
      current_audio_timestamp_file_ = fopen(audio_timestamp_path.c_str(), "r");
    }
    current_file = current_audio_file_;
    current_timestamp_file = current_audio_timestamp_file_;
  } else {  // video
    if (current_video_file_ == NULL) {
      std::string video_path = frame_files_path_ + "/raw_video_frames_" +
                               counter_stream.str() + ".bin";
      std::string video_timestamp_path = frame_files_path_ +
                                         "/raw_video_frames_" +
                                         counter_stream.str() + ".txt";
      current_video_file_ = fopen(video_path.c_str(), "rb");
      current_video_timestamp_file_ = fopen(video_timestamp_path.c_str(), "r");
    }
    current_file = current_video_file_;
    current_timestamp_file = current_video_timestamp_file_;
  }

  if (current_file == NULL || current_timestamp_file == NULL)
    return kDone;

  // read data from both the timestamp file and the raw frames file
  // if we fail reading either assume we are at the end of the file
  // and we need to peform a seek (aka read the next timestamp/datafile)
  int ret = fscanf(current_timestamp_file,
                   "%" PRId64 ",%d,",
                   &frame->timestamp_us_,
                   &frame->size_);
  if (ret != 2)
    return kPerformSeek;

  frame->data_ = static_cast<guint8*>(g_malloc(frame->size_));
  ret = fread(frame->data_, 1, frame->size_, current_file);
  if (ret != frame->size_) {
    g_free(frame->data_);
    return kPerformSeek;
  }

  // if we make it here, we have succesfully read a frame
  return kFrameRead;
}

bool MediaSourcePipeline::Build()
{
  source_ = NULL;
  appsrc_caps_video_ = NULL;
  appsrc_caps_audio_ = NULL; 
  appsrc_source_video_ = (GstAppSrc*) gst_element_factory_make("appsrc", NULL);
  appsrc_source_audio_ = (GstAppSrc*) gst_element_factory_make("appsrc", NULL);

  g_signal_connect(
      appsrc_source_video_, "need-data", G_CALLBACK(StartFeedStatic), this);
  g_signal_connect(
      appsrc_source_video_, "enough-data", G_CALLBACK(StopFeedStatic), this);


  g_signal_connect(
      appsrc_source_audio_, "need-data", G_CALLBACK(StartFeedStatic), this);
  g_signal_connect(
      appsrc_source_audio_, "enough-data", G_CALLBACK(StopFeedStatic), this);

  g_object_set(G_OBJECT(appsrc_source_video_),
               "stream-type",
               GST_APP_STREAM_TYPE_SEEKABLE,
               "format",
               GST_FORMAT_TIME,
               NULL);

  g_object_set(G_OBJECT(appsrc_source_audio_),
               "stream-type",
               GST_APP_STREAM_TYPE_SEEKABLE,
               "format",
               GST_FORMAT_TIME,
               NULL);

  g_signal_connect(
      appsrc_source_video_, "seek-data", G_CALLBACK(SeekDataStatic), this);

  g_signal_connect(
      appsrc_source_audio_, "seek-data", G_CALLBACK(SeekDataStatic), this);

  //gchar* caps_string_video = g_strdup_printf("video/x-h264, alignment=(string)au, stream-format=(string)byte-stream");
  //gchar* caps_string_audio = g_strdup_printf("audio/mpeg, mpegversion=4");

  // original video caps
  //gchar* caps_string_video = g_strdup_printf("video/x-h264, stream-format=(string)avc, alignment=(string)au, level=(string)3.1, profile=(string)main, codec_data=(buffer)014d401fffe1001c674d401fe8802802dd80b501010140000003004000000c03c60c448001000468ebef20, width=(int)1280, height=(int)720, pixel-aspect-ratio=(fraction)1/1, framerate=(fraction)100000/4201");
  //gchar* caps_string_audio = g_strdup_printf("audio/mpeg, mpegversion=(int)4, framed=(boolean)true, stream-format=(string)raw, level=(string)2, base-profile=(string)lc, profile=(string)lc, codec_data=(buffer)1210, rate=(int)44100, channels=(int)2");

  // rdk video caps
  gchar* caps_string_video = g_strdup_printf("video/x-h264, stream-format=(string)avc, alignment=(string)au, level=(string)3.1, profile=(string)main, codec_data=(buffer)014d401fffe1001b674d401fe8802802dd80b5010101400000fa40003a9803c60c448001000468ebaf20, width=(int)1280, height=(int)720, pixel-aspect-ratio=(fraction)1/1, framerate=(fraction)100000/3357");
  gchar* caps_string_audio = g_strdup_printf("audio/mpeg, mpegversion=(int)4, framed=(boolean)true, stream-format=(string)raw, level=(string)2, base-profile=(string)lc, profile=(string)lc, codec_data=(buffer)1210, rate=(int)44100, channels=(int)2");

  appsrc_caps_video_ = gst_caps_from_string(caps_string_video);
  appsrc_caps_audio_ = gst_caps_from_string(caps_string_audio);
  g_free(caps_string_video);
  g_free(caps_string_audio);

  GstElementFactory* src_factory = gst_element_factory_find("msesrc");
  if (!src_factory) {
     gst_element_register(0, "msesrc", GST_RANK_PRIMARY + 100, GST_MSE_TYPE_SRC);
  } else {
     gst_object_unref(src_factory);
  }

  pipeline_ = gst_element_factory_make("playbin", NULL);
  g_signal_connect(pipeline_, "source-setup", G_CALLBACK(sourceChangedCallback), this);

  // make westeros sink our video sink
  video_sink_ = gst_element_factory_make("westerossink", "vsink");
  //g_object_set(G_OBJECT(video_sink_), "sync", 1, NULL );
  g_object_set(G_OBJECT(pipeline_), "video-sink", video_sink_, NULL );
  /* Secure video path - SVP is available for Broadcom platform 16.2 and above  */
  if( g_object_class_find_property( G_OBJECT_GET_CLASS( video_sink_ ), "secure-video" ) )
  {
     g_object_set( G_OBJECT( video_sink_ ), "secure-video", true, NULL );
  }

  unsigned flagAudio = getGstPlayFlag("audio");
  unsigned flagVideo = getGstPlayFlag("video");
  unsigned flagNativeVideo = getGstPlayFlag("native-video");
  unsigned flagBuffering = getGstPlayFlag("buffering");

  g_object_set(pipeline_, "uri", "mse://", "flags", flagAudio | flagVideo | flagNativeVideo | flagBuffering, NULL);

  GstBus* bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline_));
  gst_bus_add_signal_watch(bus);
  g_signal_connect(bus, "message", G_CALLBACK(MessageCallbackStatic), this);
  gst_object_unref(bus);

  return true;
}

void MediaSourcePipeline::StopAllTimeouts()
{
  seeking_ = true;
  g_source_remove(status_timeout_handle_);
  StopFeedingAppSource(appsrc_source_video_);
  StopFeedingAppSource(appsrc_source_audio_);
}

void MediaSourcePipeline::Destroy() {
  StopAllTimeouts();
  CloseAllFiles();

  if (pipeline_) {
    GstState state;
    GstState pending;

    // clear out the pipeline
    gst_element_set_state (pipeline_, GST_STATE_NULL);
    gst_element_get_state (pipeline_,&state,&pending,GST_CLOCK_TIME_NONE);
    while (state!=GST_STATE_NULL) {
        gst_element_get_state (pipeline_,&state,&pending,GST_CLOCK_TIME_NONE);
        usleep(10000);
    }
    gst_object_unref(GST_OBJECT(pipeline_));

    if (source_)
      gst_object_unref(source_);
    if (appsrc_caps_video_)
      gst_caps_unref(appsrc_caps_video_);
    if (appsrc_caps_audio_)
      gst_caps_unref(appsrc_caps_audio_);

    pipeline_ = NULL;
    appsrc_source_video_ = NULL;
    appsrc_source_audio_ = NULL;
    video_sink_ = NULL;
    audio_sink_ = NULL;
    source_ = NULL;
    appsrc_caps_video_ = NULL;
    appsrc_caps_audio_ = NULL;

    printf("Pipeline Destroyed\n");
  }
}

bool MediaSourcePipeline::Start() {
  CalculateCurrentEndTime();

  if (!Build()) {
    fprintf(stderr, "Failed to build gstreamer pipeline\n");
    return false;
  }

  printf("Current end time:%f secs\n", current_end_time_secs_);

  printf("Pausing pipeline!\n");
  gst_element_set_state(pipeline_, GST_STATE_PAUSED);

  status_timeout_handle_ = g_timeout_add(
      kStatusDelayMs, reinterpret_cast<GSourceFunc>(StatusPollStatic), this);

  return true;
}

void MediaSourcePipeline::HandleKeyboardInput(unsigned int key) {

  switch (key) {
    case KEY_P:  // pause/play
      is_playing_ = !is_playing_;
      DoPause();
      break;
    default:
      break;
  }
}

rtError MediaSourcePipeline::suspend()
{
   if(is_active_)
   {
     printf("MediaSourcePipeline is going to suspend\n");
     // Destroy gstreamer pipeline to free all AV resources
     Destroy();
     is_active_ = false;
   }
   return RT_OK;
}

rtError MediaSourcePipeline::resume()
{
   if(!is_active_)
   {
     printf("MediaSourcePipeline is going to resume\n");
     // Re-create the pipeline and start playing again
     Init();
     Start();
     is_active_ = true;
   }
   return RT_OK;
}
