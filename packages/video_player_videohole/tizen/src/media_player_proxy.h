// Copyright 2023 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_MEDIA_PLAYER_PROXY_H_
#define FLUTTER_PLUGIN_MEDIA_PLAYER_PROXY_H_

#include <player.h>

typedef enum {
  PLAYER_DRM_TYPE_NONE = 0,
  PLAYER_DRM_TYPE_PLAYREADY,
  PLAYER_DRM_TYPE_MARLIN,
  PLAYER_DRM_TYPE_VERIMATRIX,
  PLAYER_DRM_TYPE_WIDEVINE_CLASSIC,
  PLAYER_DRM_TYPE_SECUREMEDIA,
  PLAYER_DRM_TYPE_SDRM,
  PLAYER_DRM_TYPE_VUDU,
  PLAYER_DRM_TYPE_WIDEVINE_CDM,
  PLAYER_DRM_TYPE_AES128,
  PLAYER_DRM_TYPE_HDCP,
  PLAYER_DRM_TYPE_DTCP,
  PLAYER_DRM_TYPE_SCSA,
  PLAYER_DRM_TYPE_CLEARKEY,
  PLAYER_DRM_TYPE_EME,
  PLAYER_DRM_TYPE_MAX_COUNT,
} player_drm_type_e;

typedef enum {
  CENC = 0,
  KEYIDS = 1,
  WEBM = 2,
} drm_init_data_type;

typedef int (*FuncPlayerSetEcoreWlDisplay)(player_h player,
                                           player_display_type_e type,
                                           void* ecore_wl_window, int x, int y,
                                           int width, int height);

typedef bool (*security_init_complete_cb)(int* drmhandle, unsigned int length,
                                          unsigned char* psshdata,
                                          void* user_data);
typedef int (*set_drm_init_data_cb)(drm_init_data_type init_type, void* data,
                                    int data_length, void* user_data);

typedef int (*FuncPlayerSetDrmHandle)(player_h player,
                                      player_drm_type_e drm_type,
                                      int drm_handle);
typedef int (*FuncPlayerSetDrmInitCompleteCB)(
    player_h player, security_init_complete_cb callback, void* user_data);
typedef int (*FuncPlayerSetDrmInitDataCB)(player_h player,
                                          set_drm_init_data_cb callback,
                                          void* user_data);

void* OpenMediaPlayerProxy();
bool InitMediaPlayerProxy(void* handle);
void CloseMediaPlayerProxy(void* handle);

extern FuncPlayerSetEcoreWlDisplay player_set_ecore_wl_display;
extern FuncPlayerSetDrmHandle player_set_drm_handle;
extern FuncPlayerSetDrmInitCompleteCB player_set_drm_init_complete_cb;
extern FuncPlayerSetDrmInitDataCB player_set_drm_init_data_cb;

#endif  // FLUTTER_PLUGIN_MEDIA_PLAYER_PROXY_H_
