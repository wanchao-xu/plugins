// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_VIDEO_PLAYER_H_
#define FLUTTER_PLUGIN_VIDEO_PLAYER_H_

#include <dart_api_dl.h>
#include <flutter/encodable_value.h>
#include <flutter/event_channel.h>

class VideoPlayer {
 public:
  using SeekCompletedCallback = std::function<void()>;

  VideoPlayer() = default;
  VideoPlayer(const VideoPlayer &) = delete;
  VideoPlayer &operator=(const VideoPlayer &) = delete;
  virtual ~VideoPlayer();

  virtual int64_t Create(const std::string &uri, int drm_type,
                         const std::string &license_server_url) = 0;
  virtual void Dispose() = 0;

  virtual void SetDisplayRoi(int32_t x, int32_t y, int32_t width,
                             int32_t height) = 0;
  virtual void Play() = 0;
  virtual void Pause() = 0;
  virtual void SetLooping(bool is_looping) = 0;
  virtual void SetVolume(double volume) = 0;
  virtual void SetPlaybackSpeed(double speed) = 0;
  virtual void SeekTo(int32_t position, SeekCompletedCallback callback) = 0;
  virtual int32_t GetPosition() = 0;
  virtual int32_t GetDuration() = 0;
  virtual void GetVideoSize(int32_t *width, int32_t *height) = 0;
  virtual bool isReady() = 0;

  // send port is used for drm
  void RegisterSendPort(Dart_Port send_port) { send_port_ = send_port; }

 protected:
  void SetUpEventChannel(int32_t player_id,
                         flutter::BinaryMessenger *messenger);
  void SendInitialized();
  void SendBufferingStart();
  void SendBufferingUpdate(int32_t value);
  void SendBufferingEnd();
  void SendSubtitleUpdate(int32_t duration, const std::string &text);
  void SendPlayCompleted();
  void SendError(const std::string &error_code,
                 const std::string &error_message);

  void OnLicenseChallenge(const void *challenge, unsigned long challenge_len,
                          void **response, unsigned long *response_len);

  bool is_initialized_ = false;

 private:
  std::unique_ptr<flutter::EventChannel<flutter::EncodableValue>>
      event_channel_;
  std::unique_ptr<flutter::EventSink<flutter::EncodableValue>> event_sink_;
  Dart_Port send_port_;
};

#endif  // FLUTTER_PLUGIN_VIDEO_PLAYER_H_
