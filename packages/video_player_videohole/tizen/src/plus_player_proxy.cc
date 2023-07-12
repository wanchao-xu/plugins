// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "plus_player_proxy.h"

#include <app_common.h>
#include <dlfcn.h>
#include <system_info.h>

#include "log.h"

typedef PlusPlayerHandle (*PlusPlayerCreatePlayer)();
typedef bool (*PlusPlayerOpen)(PlusPlayerHandle player, const std::string& uri);
typedef void (*PlusPlayerSetAppId)(PlusPlayerHandle player,
                                   const std::string& app_id);
typedef void (*PlusPlayerSetPrebufferMode)(PlusPlayerHandle player,
                                           bool is_prebuffer_mode);
typedef bool (*PlusPlayerStopSource)(PlusPlayerHandle player);
typedef bool (*PlusPlayerSetDisplay)(PlusPlayerHandle player,
                                     const plusplayer::DisplayType& type,
                                     const uint32_t serface_id, const int x,
                                     const int y, const int w, const int h);
typedef bool (*PlusPlayerSetDisplayMode)(PlusPlayerHandle player,
                                         const plusplayer::DisplayMode& mode);
typedef bool (*PlusPlayerSetDisplayRoi)(PlusPlayerHandle player,
                                        const plusplayer::Geometry& roi);
typedef bool (*PlusPlayerSetDisplayRotate)(
    PlusPlayerHandle player, const plusplayer::DisplayRotation& rotate);
typedef bool (*PlusPlayerGetDisplayRotate)(PlusPlayerHandle player,
                                           plusplayer::DisplayRotation* rotate);
typedef bool (*PlusPlayerSetDisplayVisible)(PlusPlayerHandle player,
                                            bool is_visible);
typedef bool (*PlusPlayerSetAudioMute)(PlusPlayerHandle player, bool is_mute);
typedef plusplayer::State (*PlusPlayerGetState)(PlusPlayerHandle player);
typedef bool (*PlusPlayerGetDuration)(PlusPlayerHandle player,
                                      int64_t* duration_in_milliseconds);
typedef bool (*PlusPlayerGetPlayingTime)(PlusPlayerHandle player,
                                         uint64_t* time_in_milliseconds);
typedef bool (*PlusPlayerSetPlaybackRate)(PlusPlayerHandle player,
                                          const double speed);
typedef bool (*PlusPlayerPrepare)(PlusPlayerHandle player);
typedef bool (*PlusPlayerPrepareAsync)(PlusPlayerHandle player);
typedef bool (*PlusPlayerStart)(PlusPlayerHandle player);
typedef bool (*PlusPlayerStop)(PlusPlayerHandle player);
typedef bool (*PlusPlayerPause)(PlusPlayerHandle player);
typedef bool (*PlusPlayerResume)(PlusPlayerHandle player);
typedef bool (*PlusPlayerSeek)(PlusPlayerHandle player,
                               const uint64_t time_millisecond);
typedef bool (*PlusPlayerSetStopPosition)(PlusPlayerHandle player,
                                          const uint64_t time_millisecond);
typedef bool (*PlusPlayerSuspend)(PlusPlayerHandle player);
typedef bool (*PlusPlayerRestore)(PlusPlayerHandle player,
                                  plusplayer::State state);
typedef bool (*PlusPlayerGetVideoSize)(PlusPlayerHandle player, int* width,
                                       int* height);
typedef int (*PlusPlayerGetSurfaceId)(PlusPlayerHandle player, void* window);
typedef bool (*PlusPlayerClose)(PlusPlayerHandle player);
typedef void (*PlusPlayerDestroyPlayer)(PlusPlayerHandle player);
typedef void (*PlusPlayerRegisterListener)(PlusPlayerHandle player,
                                           PlusPlayerListener* listener,
                                           void* user_data);
typedef void (*PlusPlayerUnregisterListener)(PlusPlayerHandle player);
typedef void (*PlusPlayerSetDrm)(PlusPlayerHandle player,
                                 const plusplayer::drm::Property& property);
typedef void (*PlusPlayerDrmLicenseAcquiredDone)(PlusPlayerHandle player,
                                                 plusplayer::TrackType type);
typedef bool (*PlusPlayerSetBufferConfig)(
    PlusPlayerHandle player, const std::pair<std::string, int>& config);
typedef std::vector<plusplayer::Track> (*PlusPlayerGetActiveTrackInfo)(
    PlusPlayerHandle player);
typedef std::vector<plusplayer::Track> (*PlusPlayerGetTrackInfo)(
    PlusPlayerHandle player);

std::string GetPlatformVersion() {
  char* version = nullptr;
  std::string value;
  const char* key = "http://tizen.org/feature/platform.version";
  int ret = system_info_get_platform_string(key, &version);
  if (ret == SYSTEM_INFO_ERROR_NONE) {
    value = std::string(version);
    free(version);
  }
  return value;
}

PlusPlayerProxy::PlusPlayerProxy() {
  std::string version = GetPlatformVersion();
  char* app_res_path = app_get_resource_path();
  if (app_res_path != nullptr) {
    std::string lib_path = app_res_path;
    if (version == "6.0") {
      lib_path += "/video_player_videohole/libplus_player_wrapper_60.so";
    } else if (version == "6.5") {
      lib_path += "/video_player_videohole/libplus_player_wrapper_65.so";
    } else if (version == "7.0") {
      lib_path += "/video_player_videohole/libplus_player_wrapper_70.so";
    }
    plus_player_lib_ = dlopen(lib_path.c_str(), RTLD_LAZY);
    free(app_res_path);
  }
  if (!plus_player_lib_) {
    LOG_ERROR("dlopen failed %s: ", dlerror());
  }
}

PlusPlayerProxy::~PlusPlayerProxy() {
  if (plus_player_lib_) {
    dlclose(plus_player_lib_);
    plus_player_lib_ = nullptr;
  }
}

void* PlusPlayerProxy::Dlsym(const char* name) {
  if (!plus_player_lib_) {
    LOG_ERROR("dlopen failed plus_player_lib_ is null");
    return nullptr;
  }
  return dlsym(plus_player_lib_, name);
}

PlusPlayerHandle PlusPlayerProxy::CreatePlayer() {
  PlusPlayerCreatePlayer method_create_player;
  *reinterpret_cast<void**>(&method_create_player) = Dlsym("CreatePlayer");
  if (method_create_player) {
    return method_create_player();
  }
  return nullptr;
}

bool PlusPlayerProxy::Open(PlusPlayerHandle player, const std::string& uri) {
  PlusPlayerOpen method_open;
  *reinterpret_cast<void**>(&method_open) = Dlsym("Open");
  if (method_open) {
    return method_open(player, uri);
  }
  return false;
}

bool PlusPlayerProxy::SetBufferConfig(
    PlusPlayerHandle player, const std::pair<std::string, int>& config) {
  PlusPlayerSetBufferConfig method_set_buffer_config;
  *reinterpret_cast<void**>(&method_set_buffer_config) =
      Dlsym("SetBufferConfig");
  if (method_set_buffer_config) {
    return method_set_buffer_config(player, config);
  }
  return false;
}

void PlusPlayerProxy::SetAppId(PlusPlayerHandle player,
                               const std::string& app_id) {
  PlusPlayerSetAppId method_set_app_id;
  *reinterpret_cast<void**>(&method_set_app_id) = Dlsym("SetAppId");
  if (method_set_app_id) {
    method_set_app_id(player, app_id);
  }
}

void PlusPlayerProxy::SetPrebufferMode(PlusPlayerHandle player,
                                       bool is_prebuffer_mode) {
  PlusPlayerSetPrebufferMode method_set_prebuffer_mode;
  *reinterpret_cast<void**>(&method_set_prebuffer_mode) =
      Dlsym("SetPrebufferMode");
  if (method_set_prebuffer_mode) {
    method_set_prebuffer_mode(player, is_prebuffer_mode);
  }
}

bool PlusPlayerProxy::StopSource(PlusPlayerHandle player) {
  PlusPlayerStopSource method_stop_source;
  *reinterpret_cast<void**>(&method_stop_source) = Dlsym("StopSource");
  if (method_stop_source) {
    return method_stop_source(player);
  }
  return false;
}

bool PlusPlayerProxy::SetDisplay(PlusPlayerHandle player,
                                 const plusplayer::DisplayType& type,
                                 const uint32_t serface_id, const int x,
                                 const int y, const int w, const int h) {
  PlusPlayerSetDisplay method_set_display;
  *reinterpret_cast<void**>(&method_set_display) = Dlsym("SetDisplay");
  if (method_set_display) {
    return method_set_display(player, type, serface_id, x, y, w, h);
  }
  return false;
}

bool PlusPlayerProxy::SetDisplayMode(PlusPlayerHandle player,
                                     const plusplayer::DisplayMode& mode) {
  PlusPlayerSetDisplayMode method_set_display_mode;
  *reinterpret_cast<void**>(&method_set_display_mode) = Dlsym("SetDisplayMode");
  if (method_set_display_mode) {
    return method_set_display_mode(player, mode);
  }
  return false;
}

bool PlusPlayerProxy::SetDisplayRoi(PlusPlayerHandle player,
                                    const plusplayer::Geometry& roi) {
  PlusPlayerSetDisplayRoi method_set_display_roi;
  *reinterpret_cast<void**>(&method_set_display_roi) = Dlsym("SetDisplayRoi");
  if (method_set_display_roi) {
    return method_set_display_roi(player, roi);
  }
  return false;
}

bool PlusPlayerProxy::SetDisplayRotate(
    PlusPlayerHandle player, const plusplayer::DisplayRotation& rotate) {
  PlusPlayerSetDisplayRotate method_set_display_rotate;
  *reinterpret_cast<void**>(&method_set_display_rotate) =
      Dlsym("SetDisplayRotate");
  if (method_set_display_rotate) {
    return method_set_display_rotate(player, rotate);
  }
  return false;
}

bool PlusPlayerProxy::GetDisplayRotate(PlusPlayerHandle player,
                                       plusplayer::DisplayRotation* rotate) {
  PlusPlayerGetDisplayRotate method_get_display_rotate;
  *reinterpret_cast<void**>(&method_get_display_rotate) =
      Dlsym("GetDisplayRotate");
  if (method_get_display_rotate) {
    return method_get_display_rotate(player, rotate);
  }
  return false;
}

bool PlusPlayerProxy::SetDisplayVisible(PlusPlayerHandle player,
                                        bool is_visible) {
  PlusPlayerSetDisplayVisible method_set_display_visible;
  *reinterpret_cast<void**>(&method_set_display_visible) =
      Dlsym("SetDisplayVisible");
  if (method_set_display_visible) {
    return method_set_display_visible(player, is_visible);
  }
  return false;
}

bool PlusPlayerProxy::SetAudioMute(PlusPlayerHandle player, bool is_mute) {
  PlusPlayerSetAudioMute method_set_audio_mute;
  *reinterpret_cast<void**>(&method_set_audio_mute) = Dlsym("SetAudioMute");
  if (method_set_audio_mute) {
    return method_set_audio_mute(player, is_mute);
  }
  return false;
}

plusplayer::State PlusPlayerProxy::GetState(PlusPlayerHandle player) {
  PlusPlayerGetState method_get_state;
  *reinterpret_cast<void**>(&method_get_state) = Dlsym("GetState");
  if (method_get_state) {
    return method_get_state(player);
  }
  return plusplayer::State::kNone;
}

bool PlusPlayerProxy::GetDuration(PlusPlayerHandle player,
                                  int64_t* duration_in_milliseconds) {
  PlusPlayerGetDuration method_get_duration;
  *reinterpret_cast<void**>(&method_get_duration) = Dlsym("GetDuration");
  if (method_get_duration) {
    return method_get_duration(player, duration_in_milliseconds);
  }
  return false;
}

bool PlusPlayerProxy::GetPlayingTime(PlusPlayerHandle player,
                                     uint64_t* time_in_milliseconds) {
  PlusPlayerGetPlayingTime method_get_playing_time;
  *reinterpret_cast<void**>(&method_get_playing_time) = Dlsym("GetPlayingTime");
  if (method_get_playing_time) {
    return method_get_playing_time(player, time_in_milliseconds);
  }
  return false;
}

bool PlusPlayerProxy::SetPlaybackRate(PlusPlayerHandle player,
                                      const double speed) {
  PlusPlayerSetPlaybackRate method_set_playback_rate;
  *reinterpret_cast<void**>(&method_set_playback_rate) =
      Dlsym("SetPlaybackRate");
  if (method_set_playback_rate) {
    return method_set_playback_rate(player, speed);
  }
  return false;
}

bool PlusPlayerProxy::Prepare(PlusPlayerHandle player) {
  PlusPlayerPrepare method_prepare;
  *reinterpret_cast<void**>(&method_prepare) = Dlsym("Prepare");
  if (method_prepare) {
    return method_prepare(player);
  }
  return false;
}

bool PlusPlayerProxy::PrepareAsync(PlusPlayerHandle player) {
  PlusPlayerPrepareAsync method_prepare_async;
  *reinterpret_cast<void**>(&method_prepare_async) = Dlsym("PrepareAsync");
  if (method_prepare_async) {
    return method_prepare_async(player);
  }
  return false;
}

bool PlusPlayerProxy::Start(PlusPlayerHandle player) {
  PlusPlayerStart method_start;
  *reinterpret_cast<void**>(&method_start) = Dlsym("Start");
  if (method_start) {
    return method_start(player);
  }
  return false;
}

bool PlusPlayerProxy::Stop(PlusPlayerHandle player) {
  PlusPlayerStop method_stop;
  *reinterpret_cast<void**>(&method_stop) = Dlsym("Stop");
  if (method_stop) {
    return method_stop(player);
  }
  return false;
}

bool PlusPlayerProxy::Pause(PlusPlayerHandle player) {
  PlusPlayerPause method_pause;
  *reinterpret_cast<void**>(&method_pause) = Dlsym("Pause");
  if (method_pause) {
    return method_pause(player);
  }
  return false;
}

bool PlusPlayerProxy::Resume(PlusPlayerHandle player) {
  PlusPlayerResume method_resume;
  *reinterpret_cast<void**>(&method_resume) = Dlsym("Resume");
  if (method_resume) {
    return method_resume(player);
  }
  return false;
}

bool PlusPlayerProxy::Seek(PlusPlayerHandle player,
                           const uint64_t time_millisecond) {
  PlusPlayerSeek method_seek;
  *reinterpret_cast<void**>(&method_seek) = Dlsym("Seek");
  if (method_seek) {
    return method_seek(player, time_millisecond);
  }
  return false;
}

void PlusPlayerProxy::SetStopPosition(PlusPlayerHandle player,
                                      const uint64_t time_millisecond) {
  PlusPlayerSetStopPosition method_set_stop_position;
  *reinterpret_cast<void**>(&method_set_stop_position) =
      Dlsym("SetStopPosition");
  if (method_set_stop_position) {
    method_set_stop_position(player, time_millisecond);
  }
}

bool PlusPlayerProxy::Suspend(PlusPlayerHandle player) {
  PlusPlayerSuspend method_suspend;
  *reinterpret_cast<void**>(&method_suspend) = Dlsym("Suspend");
  if (method_suspend) {
    return method_suspend(player);
  }
  return false;
}

bool PlusPlayerProxy::Restore(PlusPlayerHandle player,
                              plusplayer::State state) {
  PlusPlayerRestore method_restore;
  *reinterpret_cast<void**>(&method_restore) = Dlsym("Restore");
  if (method_restore) {
    return method_restore(player, state);
  }
  return false;
}

bool PlusPlayerProxy::GetVideoSize(PlusPlayerHandle player, int* width,
                                   int* height) {
  PlusPlayerGetVideoSize method_get_video_size;
  *reinterpret_cast<void**>(&method_get_video_size) = Dlsym("GetVideoSize");
  if (method_get_video_size) {
    return method_get_video_size(player, width, height);
  }
  return false;
}

int PlusPlayerProxy::GetSurfaceId(PlusPlayerHandle player, void* window) {
  PlusPlayerGetSurfaceId method_get_surface_id;
  *reinterpret_cast<void**>(&method_get_surface_id) = Dlsym("GetSurfaceId");
  if (method_get_surface_id) {
    return method_get_surface_id(player, window);
  }
  return -1;
}

bool PlusPlayerProxy::Close(PlusPlayerHandle player) {
  PlusPlayerClose method_close;
  *reinterpret_cast<void**>(&method_close) = Dlsym("Close");
  if (method_close) {
    return method_close(player);
  }
  return false;
}

void PlusPlayerProxy::DestroyPlayer(PlusPlayerHandle player) {
  PlusPlayerDestroyPlayer method_destroy_player;
  *reinterpret_cast<void**>(&method_destroy_player) = Dlsym("DestroyPlayer");
  if (method_destroy_player) {
    method_destroy_player(player);
  }
}

void PlusPlayerProxy::SetDrm(PlusPlayerHandle player,
                             const plusplayer::drm::Property& property) {
  PlusPlayerSetDrm method_set_drm;
  *reinterpret_cast<void**>(&method_set_drm) = Dlsym("SetDrm");
  if (method_set_drm) {
    method_set_drm(player, property);
  }
}

void PlusPlayerProxy::DrmLicenseAcquiredDone(PlusPlayerHandle player,
                                             plusplayer::TrackType type) {
  PlusPlayerDrmLicenseAcquiredDone method_drm_licenseAcquire_done;
  *reinterpret_cast<void**>(&method_drm_licenseAcquire_done) =
      Dlsym("DrmLicenseAcquiredDone");
  if (method_drm_licenseAcquire_done) {
    method_drm_licenseAcquire_done(player, type);
  }
}

void PlusPlayerProxy::RegisterListener(PlusPlayerHandle player,
                                       PlusPlayerListener* listener,
                                       void* user_data) {
  PlusPlayerRegisterListener method_register_listener;
  *reinterpret_cast<void**>(&method_register_listener) =
      Dlsym("RegisterListener");
  if (method_register_listener) {
    method_register_listener(player, listener, user_data);
  }
}

void PlusPlayerProxy::UnregisterListener(PlusPlayerHandle player) {
  PlusPlayerUnregisterListener method_unregister_listener;
  *reinterpret_cast<void**>(&method_unregister_listener) =
      Dlsym("UnregisterListener");
  if (method_unregister_listener) {
    method_unregister_listener(player);
  }
}

std::vector<plusplayer::Track> PlusPlayerProxy::GetTrackInfo(
    PlusPlayerHandle player) {
  PlusPlayerGetTrackInfo method_get_track_info;
  *reinterpret_cast<void**>(&method_get_track_info) = Dlsym("GetTrackInfo");
  if (method_get_track_info) {
    return method_get_track_info(player);
  }
  return std::vector<plusplayer::Track>{};
}

std::vector<plusplayer::Track> PlusPlayerProxy::GetActiveTrackInfo(
    PlusPlayerHandle player) {
  PlusPlayerGetActiveTrackInfo method_get_active_track_info;
  *reinterpret_cast<void**>(&method_get_active_track_info) =
      Dlsym("GetActiveTrackInfo");
  if (method_get_active_track_info) {
    return method_get_active_track_info(player);
  }
  return std::vector<plusplayer::Track>{};
}
