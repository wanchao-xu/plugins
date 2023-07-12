// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media_player.h"

#include <dlfcn.h>

#include "log.h"
#include "video_player_error.h"

typedef void (*FuncEcoreWl2WindowGeometryGet)(void *window, int *x, int *y,
                                              int *width, int *height);

static int64_t player_index = 1;

static std::string RotationToString(player_display_rotation_e rotation) {
  switch (rotation) {
    case PLAYER_DISPLAY_ROTATION_NONE:
      return "PLAYER_DISPLAY_ROTATION_NONE";
    case PLAYER_DISPLAY_ROTATION_90:
      return "PLAYER_DISPLAY_ROTATION_90";
    case PLAYER_DISPLAY_ROTATION_180:
      return "PLAYER_DISPLAY_ROTATION_180";
    case PLAYER_DISPLAY_ROTATION_270:
      return "PLAYER_DISPLAY_ROTATION_270";
  }
  return std::string();
}

static std::string StateToString(player_state_e state) {
  switch (state) {
    case PLAYER_STATE_NONE:
      return "PLAYER_STATE_NONE";
    case PLAYER_STATE_IDLE:
      return "PLAYER_STATE_IDLE";
    case PLAYER_STATE_READY:
      return "PLAYER_STATE_READY";
    case PLAYER_STATE_PLAYING:
      return "PLAYER_STATE_PLAYING";
    case PLAYER_STATE_PAUSED:
      return "PLAYER_STATE_PAUSED";
  }
  return std::string();
}

MediaPlayer::MediaPlayer(flutter::PluginRegistrar *plugin_registrar,
                         void *native_window)
    : plugin_registrar_(plugin_registrar), native_window_(native_window) {}

MediaPlayer::~MediaPlayer() { Dispose(); }

int64_t MediaPlayer::Create(const std::string &uri, int drm_type,
                            const std::string &license_server_url) {
  LOG_INFO("uri: %s, drm_type: %d", uri.c_str(), drm_type);

  if (player_) {
    throw VideoPlayerError("Operation failed",
                           "Media player has already been created");
  }

  int ret = player_create(&player_);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_create failed", get_error_message(ret));
  }

  void *player_proxy_ = OpenMediaPlayerProxy();
  if (!player_proxy_) {
    throw VideoPlayerError("dlopen failed",
                           "Cannot open dynamic library of media player");
  }
  if (!InitMediaPlayerProxy(player_proxy_)) {
    CloseMediaPlayerProxy(player_proxy_);
    player_proxy_ = nullptr;
    throw VideoPlayerError(
        "dlsym failed",
        "Cannot get private api of media player from dynamic library");
  }

  if (drm_type != 0) {
    SetDrm(uri, drm_type, license_server_url);
  }
  SetDisplay();

  // media player proxy only be used for drm and display
  CloseMediaPlayerProxy(player_proxy_);
  player_proxy_ = nullptr;

  SetDisplayRoi(0, 0, 1, 1);

  ret = player_set_uri(player_, uri.c_str());
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_uri failed", get_error_message(ret));
  }

  ret = player_set_display_visible(player_, true);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_display_visible failed",
                           get_error_message(ret));
  }

  ret = player_set_buffering_cb(player_, OnBuffering, this);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_buffering_cb failed",
                           get_error_message(ret));
  }

  ret = player_set_completed_cb(player_, OnPlayCompleted, this);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_completed_cb failed",
                           get_error_message(ret));
  }

  ret = player_set_interrupted_cb(player_, OnInterrupted, this);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_interrupted_cb failed",
                           get_error_message(ret));
  }

  ret = player_set_error_cb(player_, OnError, this);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_error_cb failed",
                           get_error_message(ret));
  }

  ret = player_set_subtitle_updated_cb(player_, OnSubtitleUpdated, this);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_subtitle_updated_cb failed",
                           get_error_message(ret));
  }

  ret = player_prepare_async(player_, OnPrepared, this);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_prepare_async failed",
                           get_error_message(ret));
  }

  player_id_ = player_index++;
  SetUpEventChannel(player_id_, plugin_registrar_->messenger());

  return player_id_;
}

void MediaPlayer::Dispose() {
  LOG_INFO("Media player disposing.");

  if (player_) {
    if (is_initialized_) {
      player_unprepare(player_);
      is_initialized_ = false;
    }
    player_destroy(player_);
    player_ = nullptr;
  }

  if (player_proxy_) {
    CloseMediaPlayerProxy(player_proxy_);
    player_proxy_ = nullptr;
  }

  // drm should be released after destroy of player
  if (drm_manager_) {
    drm_manager_->ReleaseDrmSession();
  }
}

void MediaPlayer::SetDisplayRoi(int32_t x, int32_t y, int32_t width,
                                int32_t height) {
  LOG_INFO("Media player sets display roi, x = %d, y = %d, w = %d, h = %d", x,
           y, width, height);
  int ret = player_set_display_roi_area(player_, x, y, width, height);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_display_roi_area failed",
                           get_error_message(ret));
  }
}

void MediaPlayer::Play() {
  LOG_INFO("Media player plays video.");

  player_state_e state = PLAYER_STATE_NONE;
  int ret = player_get_state(player_, &state);
  if (ret == PLAYER_ERROR_NONE) {
    LOG_INFO("[VideoPlayer] Player state: %s", StateToString(state).c_str());
    if (state != PLAYER_STATE_PAUSED && state != PLAYER_STATE_READY) {
      return;
    }
  }

  ret = player_start(player_);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_start failed", get_error_message(ret));
  }
}

void MediaPlayer::Pause() {
  LOG_INFO("Media player pauses video.");

  player_state_e state = PLAYER_STATE_NONE;
  int ret = player_get_state(player_, &state);
  if (ret == PLAYER_ERROR_NONE) {
    LOG_INFO("[VideoPlayer] Player state: %s", StateToString(state).c_str());
    if (state != PLAYER_STATE_PLAYING) {
      return;
    }
  }

  ret = player_pause(player_);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_pause failed", get_error_message(ret));
  }
}

void MediaPlayer::SetLooping(bool is_looping) {
  LOG_INFO("Media player sets looping(%d)", is_looping);

  int ret = player_set_looping(player_, is_looping);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_looping failed", get_error_message(ret));
  }
}

void MediaPlayer::SetVolume(double volume) {
  LOG_INFO("Media player sets volume(%f)", volume);

  int ret = player_set_volume(player_, volume, volume);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_volume failed", get_error_message(ret));
  }
}

void MediaPlayer::SetPlaybackSpeed(double speed) {
  LOG_INFO("Media player sets playback speed(%f)", speed);

  int ret = player_set_playback_rate(player_, speed);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_playback_rate failed",
                           get_error_message(ret));
  }
}

void MediaPlayer::SeekTo(int32_t position, SeekCompletedCallback callback) {
  LOG_INFO("Media player seeks to position(%d)", position);

  on_seek_completed_ = std::move(callback);
  int ret =
      player_set_play_position(player_, position, true, OnSeekCompleted, this);
  if (ret != PLAYER_ERROR_NONE) {
    on_seek_completed_ = nullptr;
    throw VideoPlayerError("player_set_play_position failed",
                           get_error_message(ret));
  }
}

int32_t MediaPlayer::GetPosition() {
  int position = 0;
  int ret = player_get_play_position(player_, &position);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_get_play_position failed",
                           get_error_message(ret));
  }
  return position;
}

int32_t MediaPlayer::GetDuration() {
  int duration = 0;
  int ret = player_get_duration(player_, &duration);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_get_duration failed",
                           get_error_message(ret));
  }
  LOG_INFO("Video duration: %d", duration);
  return duration;
}

void MediaPlayer::GetVideoSize(int32_t *width, int32_t *height) {
  int w = 0, h = 0;
  int ret = player_get_video_size(player_, &w, &h);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_get_video_size failed",
                           get_error_message(ret));
  }
  LOG_INFO("Video width: %d, height: %d", w, h);

  player_display_rotation_e rotation = PLAYER_DISPLAY_ROTATION_NONE;
  ret = player_get_display_rotation(player_, &rotation);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_get_display_rotation failed",
                           get_error_message(ret));
  }
  LOG_DEBUG("Video rotation: %s", RotationToString(rotation).c_str());
  if (rotation == PLAYER_DISPLAY_ROTATION_90 ||
      rotation == PLAYER_DISPLAY_ROTATION_270) {
    std::swap(w, h);
  }

  *width = w;
  *height = h;
}

bool MediaPlayer::isReady() {
  player_state_e state = PLAYER_STATE_NONE;
  int ret = player_get_state(player_, &state);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_get_state failed", get_error_message(ret));
  }

  LOG_INFO("Media player state: %d", state);
  return PLAYER_STATE_READY == state;
}

void MediaPlayer::SetDisplay() {
  int x = 0, y = 0, width = 0, height = 0;
  void *ecore_lib_handle = dlopen("libecore_wl2.so.1", RTLD_LAZY);
  if (ecore_lib_handle) {
    FuncEcoreWl2WindowGeometryGet ecore_wl2_window_geometry_get =
        reinterpret_cast<FuncEcoreWl2WindowGeometryGet>(
            dlsym(ecore_lib_handle, "ecore_wl2_window_geometry_get"));
    if (ecore_wl2_window_geometry_get) {
      ecore_wl2_window_geometry_get(native_window_, &x, &y, &width, &height);
    } else {
      dlclose(ecore_lib_handle);
      throw VideoPlayerError(
          "dlsym failed",
          "Cannot get private api of ecore_wl2 from dynamic library");
    }
    dlclose(ecore_lib_handle);
  } else {
    throw VideoPlayerError("dlopen failed",
                           "Cannot open dynamic library of ecore_wl2");
  }

  int ret = player_set_ecore_wl_display(player_, PLAYER_DISPLAY_TYPE_OVERLAY,
                                        native_window_, x, y, width, height);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_ecore_wl_display failed",
                           get_error_message(ret));
  }

  ret = player_set_display_mode(player_, PLAYER_DISPLAY_MODE_DST_ROI);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_display_mode failed",
                           get_error_message(ret));
  }
}

void MediaPlayer::SetDrm(const std::string &uri, int drm_type,
                         const std::string &license_server_url) {
  drm_manager_ = std::make_unique<DrmManager>();
  if (!drm_manager_->CreateDrmSession(drm_type, false)) {
    throw VideoPlayerError("Drm error", "Failed to create drm session");
  }

  int drm_handle = 0;
  if (!drm_manager_->GetDrmHandle(&drm_handle)) {
    throw VideoPlayerError("Drm error", "Failed to get drm handle");
  }
  int ret = player_set_drm_handle(player_, PLAYER_DRM_TYPE_EME, drm_handle);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_drm_handle failed",
                           get_error_message(ret));
  }

  ret =
      player_set_drm_init_complete_cb(player_, OnDrmSecurityInitComplete, this);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_drm_init_complete_cb failed",
                           get_error_message(ret));
  }

  ret = player_set_drm_init_data_cb(player_, OnDrmUpdatePsshData, this);
  if (ret != PLAYER_ERROR_NONE) {
    throw VideoPlayerError("player_set_drm_init_data_cb failed",
                           get_error_message(ret));
  }

  if (license_server_url.empty()) {
    bool success = drm_manager_->SetChallenge(
        uri, [this](const void *challenge, unsigned long challenge_len,
                    void **response, unsigned long *response_len) {
          OnLicenseChallenge(challenge, challenge_len, response, response_len);
        });
    if (!success) {
      throw VideoPlayerError("Drm error", "Failed to set challenge");
    }
  } else {
    if (!drm_manager_->SetChallenge(uri, license_server_url)) {
      throw VideoPlayerError("Drm error", "Failed to set challenge");
    }
  }
}

void MediaPlayer::OnPrepared(void *user_data) {
  LOG_INFO("Media player prepared.");

  MediaPlayer *self = static_cast<MediaPlayer *>(user_data);
  if (!self->is_initialized_) {
    self->SendInitialized();
  }
}

void MediaPlayer::OnBuffering(int percent, void *user_data) {
  LOG_INFO("Buffering percent: %d", percent);

  MediaPlayer *self = static_cast<MediaPlayer *>(user_data);
  if (percent == 100) {
    self->SendBufferingEnd();
    self->is_buffering_ = false;
  } else if (!self->is_buffering_ && percent <= 5) {
    self->SendBufferingStart();
    self->is_buffering_ = true;
  } else {
    self->SendBufferingUpdate(percent);
  }
}

void MediaPlayer::OnSeekCompleted(void *user_data) {
  LOG_INFO("Seek completed.");

  MediaPlayer *self = static_cast<MediaPlayer *>(user_data);
  if (self->on_seek_completed_) {
    self->on_seek_completed_();
    self->on_seek_completed_ = nullptr;
  }
}

void MediaPlayer::OnPlayCompleted(void *user_data) {
  LOG_INFO("Play completed.");

  MediaPlayer *self = static_cast<MediaPlayer *>(user_data);
  self->SendPlayCompleted();
  self->Pause();
}

void MediaPlayer::OnInterrupted(player_interrupted_code_e code,
                                void *user_data) {
  LOG_ERROR("Interrupt code: %d", code);

  MediaPlayer *self = static_cast<MediaPlayer *>(user_data);
  self->SendError("Interrupted error", "Media player has been interrupted.");
}

void MediaPlayer::OnError(int error_code, void *user_data) {
  LOG_ERROR("An error occurred for media player, error: %d (%s)", error_code,
            get_error_message(error_code));

  MediaPlayer *self = static_cast<MediaPlayer *>(user_data);
  self->SendError("Media Player error", get_error_message(error_code));
}

void MediaPlayer::OnSubtitleUpdated(unsigned long duration, char *text,
                                    void *user_data) {
  LOG_INFO("Subtitle updated, duration: %ld, text: %s", duration, text);

  MediaPlayer *self = static_cast<MediaPlayer *>(user_data);
  self->SendSubtitleUpdate(duration, std::string(text));
}

bool MediaPlayer::OnDrmSecurityInitComplete(int *drm_handle,
                                            unsigned int length,
                                            unsigned char *pssh_data,
                                            void *user_data) {
  LOG_INFO("Drm init completed.");

  MediaPlayer *self = static_cast<MediaPlayer *>(user_data);
  if (self->drm_manager_) {
    return self->drm_manager_->SecurityInitCompleteCB(drm_handle, length,
                                                      pssh_data, self->player_);
  }
  return false;
}

int MediaPlayer::OnDrmUpdatePsshData(drm_init_data_type init_type, void *data,
                                     int data_length, void *user_data) {
  LOG_INFO("Drm update pssh data.");

  MediaPlayer *self = static_cast<MediaPlayer *>(user_data);
  if (self->drm_manager_) {
    return self->drm_manager_->UpdatePsshData(data, data_length);
  }
  return 0;
}
