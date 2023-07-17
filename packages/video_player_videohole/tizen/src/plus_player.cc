// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "plus_player.h"

#include <app_manager.h>
#include <dlfcn.h>
#include <system_info.h>
#include <unistd.h>

#include "log.h"
#include "video_player_error.h"

PlusPlayer::PlusPlayer(flutter::PluginRegistrar *plugin_registrar,
                       void *native_window)
    : plugin_registrar_(plugin_registrar), native_window_(native_window) {}

PlusPlayer::~PlusPlayer() { Dispose(); }

int64_t PlusPlayer::Create(const std::string &uri, int drm_type,
                           const std::string &license_server_url) {
  LOG_INFO("Create plus player");
  PlusPlayerProxy &instance = PlusPlayerProxy::GetInstance();
  player_ = instance.CreatePlayer();
  if (!player_) {
    throw VideoPlayerError("Create failed", "Failed to create PlusPlayer");
  }

  if (!instance.Open(player_, uri)) {
    LOG_ERROR("Plus player failed to open uri %s", uri.c_str());
    throw VideoPlayerError("Open failed", "PlusPlayer failed to open video");
  }

  char *appId = nullptr;
  int ret = app_manager_get_app_id(getpid(), &appId);
  if (ret != APP_MANAGER_ERROR_NONE) {
    throw VideoPlayerError("app_manager_get_app_id failed",
                           get_error_message(ret));
  }
  instance.SetAppId(player_, appId);
  free(appId);

  listener_.buffering_callback = OnBuffering;
  listener_.adaptive_streaming_control_callback = OnAdaptiveStreamingControl;
  listener_.completed_callback = OnPlayCompleted;
  listener_.drm_init_data_callback = OnDrmInitData;
  listener_.error_callback = OnError;
  listener_.error_message_callback = OnErrorMessage;
  listener_.prepared_callback = OnPrepared;
  listener_.seek_completed_callback = OnSeekCompleted;
  listener_.subtitle_updated_callback = OnSubtitleUpdated;
  instance.RegisterListener(player_, &listener_, this);

  if (drm_type != 0) {
    SetDrm(uri, drm_type, license_server_url);
  }
  SetDisplay();
  SetDisplayRoi(0, 0, 1, 1);

  if (!instance.PrepareAsync(player_)) {
    throw VideoPlayerError("PrepareAsync failed",
                           "PlusPlayer failed to prepare async");
  }

  int64_t id = GeneratePlayerID();
  SetUpEventChannel(id, plugin_registrar_->messenger());

  return id;
}

void PlusPlayer::Dispose() {
  LOG_INFO("PlusPlayer disposing.");

  if (player_) {
    PlusPlayerProxy &instance = PlusPlayerProxy::GetInstance();
    instance.UnregisterListener(player_);
    instance.DestroyPlayer(player_);
    player_ = nullptr;
  }

  // drm should be released after destroy of player
  if (drm_manager_) {
    drm_manager_->ReleaseDrmSession();
  }
}

void PlusPlayer::SetDisplayRoi(int32_t x, int32_t y, int32_t width,
                               int32_t height) {
  LOG_INFO("PlusPlayer sets display roi, x = %d, y = %d, w = %d, h = %d", x, y,
           width, height);
  if (player_ == nullptr) {
    throw VideoPlayerError("Invalid PlusPlayer", "PlusPlayer is not created");
  }

  plusplayer::Geometry roi;
  roi.x = x;
  roi.y = y;
  roi.w = width;
  roi.h = height;
  if (!PlusPlayerProxy::GetInstance().SetDisplayRoi(player_, roi)) {
    throw VideoPlayerError("SetDisplayRoi failed",
                           "PlusPlayer failed to set display roi");
  }
}

void PlusPlayer::Play() {
  LOG_INFO("PlusPlayer plays video.");
  if (player_ == nullptr) {
    throw VideoPlayerError("Invalid PlusPlayer", "PlusPlayer is not created");
  }

  PlusPlayerProxy &instance = PlusPlayerProxy::GetInstance();
  plusplayer::State state = instance.GetState(player_);
  if (state < plusplayer::State::kReady) {
    throw VideoPlayerError("Invalid State", "PlusPlayer is not ready");
  }

  if (state == plusplayer::State::kReady) {
    if (!instance.Start(player_)) {
      throw VideoPlayerError("Start failed", "PlusPlayer failed to start");
    }
  } else if (state == plusplayer::State::kPaused) {
    if (!instance.Resume(player_)) {
      throw VideoPlayerError("Resume failed",
                             "PlusPlayer failed to resume playing");
    }
  }
}

void PlusPlayer::Pause() {
  LOG_INFO("PlusPlayer pauses video.");
  if (player_ == nullptr) {
    throw VideoPlayerError("Invalid PlusPlayer", "PlusPlayer is not created");
  }

  PlusPlayerProxy &instance = PlusPlayerProxy::GetInstance();
  plusplayer::State state = instance.GetState(player_);
  if (state < plusplayer::State::kReady) {
    throw VideoPlayerError("Invalid State", "PlusPlayer is not ready");
  }

  if (state == plusplayer::State::kPlaying) {
    if (!instance.Pause(player_)) {
      throw VideoPlayerError("Pause failed",
                             "PlusPlayer failed to pause video");
    }
  }
}

void PlusPlayer::SetLooping(bool is_looping) {
  throw VideoPlayerError("Invalid Operation",
                         "PlusPlayer doesn't support to set looping");
}

void PlusPlayer::SetVolume(double volume) {
  throw VideoPlayerError("Invalid Operation",
                         "PlusPlayer doesn't support to set volume");
}

void PlusPlayer::SetPlaybackSpeed(double speed) {
  LOG_INFO("Media player sets playback speed(%f)", speed);
  if (player_ == nullptr) {
    throw VideoPlayerError("Invalid PlusPlayer", "PlusPlayer is not created");
  }

  PlusPlayerProxy &instance = PlusPlayerProxy::GetInstance();
  if (instance.GetState(player_) > plusplayer::State::kIdle) {
    if (!instance.SetPlaybackRate(player_, speed)) {
      throw VideoPlayerError("SetPlaybackRate failed",
                             "PlusPlayer failed to set playback rate");
    }
  } else {
    throw VideoPlayerError("Invalid State", "PlusPlayer is not prepared");
  }
}

void PlusPlayer::SeekTo(int32_t position, SeekCompletedCallback callback) {
  LOG_INFO("PlusPlayer seeks to position(%d)", position);
  if (player_ == nullptr) {
    throw VideoPlayerError("Invalid PlusPlayer", "PlusPlayer is not created");
  }

  if (on_seek_completed_) {
    throw VideoPlayerError("Invalid Operation",
                           "PlusPlayer is already seeking");
  }

  PlusPlayerProxy &instance = PlusPlayerProxy::GetInstance();
  if (instance.GetState(player_) >= plusplayer::State::kReady) {
    on_seek_completed_ = std::move(callback);
    if (!instance.Seek(player_, position)) {
      on_seek_completed_ = nullptr;
      throw VideoPlayerError("Seek failed", "PlusPlayer failed to seek");
    }
  } else {
    throw VideoPlayerError("Invalid State", "PlusPlayer is not ready");
  }
}

int32_t PlusPlayer::GetPosition() {
  if (player_ == nullptr) {
    throw VideoPlayerError("Invalid PlusPlayer", "PlusPlayer is not created");
  }

  PlusPlayerProxy &instance = PlusPlayerProxy::GetInstance();
  plusplayer::State state = instance.GetState(player_);
  if (state == plusplayer::State::kPlaying ||
      state == plusplayer::State::kPaused) {
    uint64_t position;
    if (!instance.GetPlayingTime(player_, &position)) {
      throw VideoPlayerError(
          "GetPlayingTime failed",
          "PlusPlayer failed to get the current playing time");
    }
    return position;
  } else {
    throw VideoPlayerError("Invalid State", "PlusPlayer is not playing video");
  }
}

int32_t PlusPlayer::GetDuration() {
  if (player_ == nullptr) {
    throw VideoPlayerError("Invalid PlusPlayer", "PlusPlayer is not created");
  }

  PlusPlayerProxy &instance = PlusPlayerProxy::GetInstance();
  if (instance.GetState(player_) >= plusplayer::State::kTrackSourceReady) {
    int64_t duration;
    if (!instance.GetDuration(player_, &duration)) {
      throw VideoPlayerError("GetDuration failed",
                             "PlusPlayer failed to get the duration");
    }
    LOG_INFO("Video duration: %llu", duration);
    return duration;
  } else {
    throw VideoPlayerError("Invalid State", "PlusPlayer is not prepared");
  }
}

void PlusPlayer::GetVideoSize(int32_t *width, int32_t *height) {
  if (player_ == nullptr) {
    throw VideoPlayerError("Invalid PlusPlayer", "PlusPlayer is not created");
  }

  PlusPlayerProxy &instance = PlusPlayerProxy::GetInstance();
  if (instance.GetState(player_) >= plusplayer::State::kTrackSourceReady) {
    int w = 0, h = 0;
    if (!instance.GetVideoSize(player_, &w, &h)) {
      throw VideoPlayerError("GetVideoSize failed",
                             "PlusPlayer failed to get the video size");
    }
    *width = w;
    *height = h;
    LOG_INFO("Video widht: %d, height: %d", w, h);
  } else {
    throw VideoPlayerError("Invalid State", "PlusPlayer is not prepared");
  }
}

bool PlusPlayer::isReady() {
  if (player_ == nullptr) {
    throw VideoPlayerError("Invalid PlusPlayer", "PlusPlayer is not created");
  }

  return plusplayer::State::kReady ==
         PlusPlayerProxy::GetInstance().GetState(player_);
}

void PlusPlayer::SetDisplay() {
  int width = 0;
  int height = 0;
  if (system_info_get_platform_int("http://tizen.org/feature/screen.width",
                                   &width) != SYSTEM_INFO_ERROR_NONE ||
      system_info_get_platform_int("http://tizen.org/feature/screen.height",
                                   &height) != SYSTEM_INFO_ERROR_NONE) {
    throw VideoPlayerError("PlusPlayer error",
                           "Could not obtain the screen size");
  }

  PlusPlayerProxy &instance = PlusPlayerProxy::GetInstance();
  bool ret = instance.SetDisplay(player_, plusplayer::DisplayType::kOverlay,
                                 instance.GetSurfaceId(player_, native_window_),
                                 0, 0, width, height);
  if (!ret) {
    throw VideoPlayerError("PlusPlayer error", "Failed to set display");
  }

  ret = instance.SetDisplayMode(player_, plusplayer::DisplayMode::kDstRoi);
  if (!ret) {
    throw VideoPlayerError("PlusPlayer error", "Failed to set display mode");
  }
}

void PlusPlayer::SetDrm(const std::string &uri, int drm_type,
                        const std::string &license_server_url) {
  drm_manager_ = std::make_unique<DrmManager>();
  if (!drm_manager_->CreateDrmSession(drm_type, true)) {
    throw VideoPlayerError("Drm error", "Failed to create drm session");
  }

  int drm_handle = 0;
  if (!drm_manager_->GetDrmHandle(&drm_handle)) {
    throw VideoPlayerError("Drm error", "Failed to get drm handle");
  }

  plusplayer::drm::Type type;
  switch (drm_type) {
    case DrmManager::DrmType::DRM_TYPE_PLAYREADAY:
      type = plusplayer::drm::Type::kPlayready;
      break;
    case DrmManager::DrmType::DRM_TYPE_WIDEVINECDM:
      type = plusplayer::drm::Type::kWidevineCdm;
      break;
    default:
      type = plusplayer::drm::Type::kNone;
      break;
  }

  plusplayer::drm::Property property;
  property.handle = drm_handle;
  property.type = type;
  property.license_acquired_cb =
      reinterpret_cast<plusplayer::drm::LicenseAcquiredCb>(OnLicenseAcquired);
  property.license_acquired_userdata =
      reinterpret_cast<plusplayer::drm::UserData>(this);
  property.external_decryption = false;
  PlusPlayerProxy::GetInstance().SetDrm(player_, property);

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

void PlusPlayer::OnPrepared(bool ret, void *user_data) {
  LOG_DEBUG("Prepare done, result: %d", ret);

  PlusPlayer *self = static_cast<PlusPlayer *>(user_data);
  if (!self->is_initialized_ && ret) {
    self->SendInitialized();
  }
}

void PlusPlayer::OnBuffering(int percent, void *user_data) {
  LOG_INFO("Buffering percent: %d", percent);

  PlusPlayer *self = static_cast<PlusPlayer *>(user_data);
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

void PlusPlayer::OnSeekCompleted(void *user_data) {
  LOG_INFO("Seek completed.");

  PlusPlayer *self = static_cast<PlusPlayer *>(user_data);
  if (self->on_seek_completed_) {
    self->on_seek_completed_();
    self->on_seek_completed_ = nullptr;
  }
}

void PlusPlayer::OnPlayCompleted(void *user_data) {
  LOG_INFO("Play completed");

  PlusPlayer *self = static_cast<PlusPlayer *>(user_data);
  self->SendPlayCompleted();
}

void PlusPlayer::OnError(const plusplayer::ErrorType &error_code,
                         void *user_data) {
  LOG_ERROR("Error code: %d", error_code);

  PlusPlayer *self = static_cast<PlusPlayer *>(user_data);
  self->SendError("PlusPlayer error", "");
}

void PlusPlayer::OnErrorMessage(const plusplayer::ErrorType &error_code,
                                const char *error_msg, void *user_data) {
  LOG_ERROR("Error code: %d, message: %s", error_code, error_msg);

  PlusPlayer *self = static_cast<PlusPlayer *>(user_data);
  self->SendError("PlusPlayer error", error_msg);
}

void PlusPlayer::OnSubtitleUpdated(char *data, const int size,
                                   const plusplayer::SubtitleType &type,
                                   const uint64_t duration, void *user_data) {
  LOG_INFO("Subtitle updated, duration: %llu, text: %s", duration, data);

  PlusPlayer *self = static_cast<PlusPlayer *>(user_data);
  self->SendSubtitleUpdate(duration, data);
}

void PlusPlayer::OnAdaptiveStreamingControl(
    const plusplayer::StreamingMessageType &type,
    const plusplayer::MessageParam &msg, void *user_data) {
  LOG_INFO("Message type: %d, is DrmInitData (%d)", type,
           type == plusplayer::StreamingMessageType::kDrmInitData);

  PlusPlayer *self = static_cast<PlusPlayer *>(user_data);
  if (type == plusplayer::StreamingMessageType::kDrmInitData) {
    if (msg.data.empty() || 0 == msg.size) {
      LOG_ERROR("Empty message");
      return;
    }

    if (self->drm_manager_) {
      self->drm_manager_->UpdatePsshData(msg.data.data(), msg.size);
    }
  }
}

void PlusPlayer::OnDrmInitData(int *drmhandle, unsigned int len,
                               unsigned char *psshdata,
                               plusplayer::TrackType type, void *user_data) {
  LOG_INFO("Drm init completed");

  PlusPlayer *self = static_cast<PlusPlayer *>(user_data);
  if (self->drm_manager_) {
    if (self->drm_manager_->SecurityInitCompleteCB(drmhandle, len, psshdata,
                                                   nullptr)) {
      PlusPlayerProxy::GetInstance().DrmLicenseAcquiredDone(self->player_,
                                                            type);
    }
  }
}

bool PlusPlayer::OnLicenseAcquired(int *drm_handle, unsigned int length,
                                   unsigned char *pssh_data, void *user_data) {
  LOG_INFO("License acquired.");

  PlusPlayer *self = static_cast<PlusPlayer *>(user_data);
  if (self->drm_manager_) {
    return self->drm_manager_->SecurityInitCompleteCB(drm_handle, length,
                                                      pssh_data, self->player_);
  }
  return false;
}
