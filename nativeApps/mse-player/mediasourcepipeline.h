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

#ifndef MEDIASOURCEPIPELINE_H_
#define MEDIASOURCEPIPELINE_H_

#include <gst/app/gstappsrc.h>
#include <gst/gst.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include <rtRemote.h>
#include <rtError.h>

enum ReadStatus { kDone = 0, kFrameRead, kPerformSeek };
enum PipelineType { kAudioVideo = 0, kAudioOnly, kVideoOnly };

enum AVType { kAudio = 0, kVideo };

struct AVFrame {
  guint8* data_;
  int32_t size_;
  int64_t timestamp_us_;
};

class MediaSourcePipeline : public rtObject {
 public:
  rtDeclareObject(MediaSourcePipeline, rtObject);
  rtMethodNoArgAndNoReturn("suspend", suspend);
  rtMethodNoArgAndNoReturn("resume", resume);

  explicit MediaSourcePipeline(std::string frame_files_path);
  virtual ~MediaSourcePipeline();
  virtual bool Start();
  virtual void HandleKeyboardInput(unsigned int key);
  rtError suspend();
  rtError resume();

  // functions called by glib static functions
  gboolean HandleMessage(GstMessage* message);
  void StartFeedingAppSource(GstAppSrc* p_src);
  void StopFeedingAppSource(GstAppSrc* p_src);
  void SetNewAppSourceReadPosition(GstAppSrc* p_src, guint64 position);
  void OnAutoPadAddedMediaSource(GstElement* element, GstPad* pad);
  void OnAutoElementAddedMediaSource(GstElement* element);
  gboolean ReadVideoFrame();
  gboolean ReadAudioFrame();
  gboolean StatusPoll();
  gboolean ChunkDemuxerSeek();
  void sourceChanged();

 private:
  bool Build();
  void Init();
  void Destroy();
  void StopAllTimeouts();
  void CloseAllFiles();
  void PerformSeek();
  ReadStatus GetNextFrame(AVFrame* frame, AVType type);
  bool PushFrameToAppSrc(const AVFrame& frame, AVType type);
  bool ShouldBeReading(AVType av);
  void SetShouldBeReading(bool is_reading, AVType av);
  void CalculateCurrentEndTime();
  bool ShouldPerformSeek();
  int64_t GetCurrentStartTimeMicroseconds() const;
  bool IsPlaybackOver();
  void AddPlaybackPositionToHistory(int64_t position);
  bool IsPlaybackStalled();
  bool HasPlaybackAdvanced();
  void ResetPlaybackHistory();
  void DoPause();
  void finishPipelineLinkingAndStartPlaybackIfNeeded();

  std::string frame_files_path_;
  int32_t current_file_counter_;
  FILE* current_video_file_;
  FILE* current_video_timestamp_file_;
  FILE* current_audio_file_;
  FILE* current_audio_timestamp_file_;
  bool seeking_;
  GstElement* pipeline_;
  GstAppSrc* appsrc_source_video_;
  GstAppSrc* appsrc_source_audio_;
  GstElement* video_sink_;
  GstElement* audio_sink_;
  std::vector<GstElement*> ms_video_pipeline_;
  std::vector<GstElement*> ms_audio_pipeline_;
  bool should_be_reading_[2];
  float playback_position_secs_;
  float current_end_time_secs_;
  guint video_frame_timeout_handle_;
  guint audio_frame_timeout_handle_;
  guint status_timeout_handle_;
  int32_t current_playback_history_cnt_;
  std::vector<int64_t> playback_position_history_;
  bool playback_started_;
  bool is_playing_;
  PipelineType pipeline_type_;

  GstElement* source_;
  GstCaps    *appsrc_caps_video_;
  GstCaps    *appsrc_caps_audio_;
  bool pause_before_seek_;
  bool is_active_;
  int64_t seek_offset_;
  
};

#endif  // MEDIASOURCEPIPELINE_H_
