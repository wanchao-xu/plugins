// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_PLUS_PLAYER_H_
#define FLUTTER_PLUGIN_PLUS_PLAYER_H_

#include <flutter/plugin_registrar.h>

#include "drm_manager.h"
#include "plus_player_proxy.h"
#include "video_player.h"

class PlusPlayer : public VideoPlayer {
 public:
  explicit PlusPlayer(flutter::PluginRegistrar *plugin_registrar,
                      void *native_window);
  ~PlusPlayer();

  int64_t Create(const std::string &uri, int drm_type,
                 const std::string &license_server_url) override;
  void Dispose() override;

  void SetDisplayRoi(int32_t x, int32_t y, int32_t width,
                     int32_t height) override;
  void Play() override;
  void Pause() override;
  void SetLooping(bool is_looping) override;
  void SetVolume(double volume) override;
  void SetPlaybackSpeed(double speed) override;
  void SeekTo(int32_t position, SeekCompletedCallback callback) override;
  int32_t GetPosition() override;
  int32_t GetDuration() override;
  void GetVideoSize(int32_t *width, int32_t *height) override;
  bool isReady() override;

 private:
  void SetDisplay();
  void SetDrm(const std::string &uri, int drm_type,
              const std::string &license_server_url);

  static void OnPrepared(bool ret, void *user_data);
  static void OnBuffering(int percent, void *user_data);
  static void OnSeekCompleted(void *user_data);
  static void OnPlayCompleted(void *user_data);
  static void OnError(const plusplayer::ErrorType &error_code, void *user_data);
  static void OnErrorMessage(const plusplayer::ErrorType &error_code,
                             const char *error_msg, void *user_data);
  static void OnSubtitleUpdated(char *data, const int size,
                                const plusplayer::SubtitleType &type,
                                const uint64_t duration, void *user_data);
  static void OnAdaptiveStreamingControl(
      const plusplayer::StreamingMessageType &type,
      const plusplayer::MessageParam &msg, void *user_data);
  static void OnDrmInitData(int *drmhandle, unsigned int len,
                            unsigned char *psshdata, plusplayer::TrackType type,
                            void *user_data);
  static bool OnLicenseAcquired(int *drm_handle, unsigned int length,
                                unsigned char *pssh_data, void *user_data);

  PlusPlayerHandle player_;
  PlusPlayerListener listener_;
  flutter::PluginRegistrar *plugin_registrar_;
  void *native_window_;
  std::unique_ptr<DrmManager> drm_manager_;
  bool is_buffering_ = false;
  SeekCompletedCallback on_seek_completed_;
};

#endif  // VIDEO_PLAYER_H_
