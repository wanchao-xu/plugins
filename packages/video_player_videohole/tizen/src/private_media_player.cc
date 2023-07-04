// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "private_media_player.h"

#include <dlfcn.h>

FuncPlayerSetEcoreWlDisplay player_set_ecore_wl_display = nullptr;
FuncPlayerSetDrmHandle player_set_drm_handle = nullptr;
FuncPlayerSetDrmInitCompleteCB player_set_drm_init_complete_cb = nullptr;
FuncPlayerSetDrmInitDataCB player_set_drm_init_data_cb = nullptr;

void* OpenMediaPlayer() {
  return dlopen("libcapi-media-player.so.0", RTLD_LAZY);
}

bool InitMediaPlayer(void* handle) {
  if (!handle) {
    return false;
  }

  player_set_ecore_wl_display = reinterpret_cast<FuncPlayerSetEcoreWlDisplay>(
      dlsym(handle, "player_set_ecore_wl_display"));
  if (!player_set_ecore_wl_display) {
    return false;
  }

  player_set_drm_handle = reinterpret_cast<FuncPlayerSetDrmHandle>(
      dlsym(handle, "player_set_drm_handle"));
  if (!player_set_drm_handle) {
    return false;
  }

  player_set_drm_init_complete_cb =
      reinterpret_cast<FuncPlayerSetDrmInitCompleteCB>(
          dlsym(handle, "player_set_drm_init_complete_cb"));
  if (!player_set_drm_init_complete_cb) {
    return false;
  }

  player_set_drm_init_data_cb = reinterpret_cast<FuncPlayerSetDrmInitDataCB>(
      dlsym(handle, "player_set_drm_init_data_cb"));
  if (!player_set_drm_init_data_cb) {
    return false;
  }

  return true;
}

void CloseMediaPlayer(void* handle) {
  if (handle) {
    dlclose(handle);
  }
}
