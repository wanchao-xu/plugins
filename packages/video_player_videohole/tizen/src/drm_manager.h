// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FLUTTER_PLUGIN_DRM_MANAGER_H_
#define FLUTTER_PLUGIN_DRM_MANAGER_H_

#include <functional>

class DrmManager {
 public:
  typedef enum {
    DRM_TYPE_NONE,
    DRM_TYPE_PLAYREADAY,
    DRM_TYPE_WIDEVINECDM,
  } DrmType;

  using ChallengeCallback =
      std::function<void(const void *challenge, unsigned long challenge_len,
                         void **response, unsigned long *response_len)>;

  explicit DrmManager();
  ~DrmManager();

  bool CreateDrmSession(int drm_type);
  bool SetChallenge(const std::string &media_url,
                    const std::string &license_server_url);
  bool SetChallenge(const std::string &media_url, ChallengeCallback callback);
  void ReleaseDrmSession();

  bool GetDrmHandle(int *handle);
  int UpdatePsshData(void *data, int length);
  bool SecurityInitCompleteCB(int *drm_handle, unsigned int len,
                              unsigned char *pssh_data, void *user_data);

 private:
  int SetChallenge(const std::string &media_url);
  static int OnChallengeData(void *session_id, int message_type, void *message,
                             int message_length, void *user_data);
  static void OnDrmManagerError(long error_code, char *error_message,
                                void *user_data);

  void *drm_session_ = nullptr;
  void *drm_manager_proxy_ = nullptr;

  int drm_type_;
  std::string license_server_url_;
  ChallengeCallback challenge_callback_;

  bool initialized_ = false;
};

#endif  // FLUTTER_PLUGIN_DRM_MANAGER_H_
