// Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "video_player.h"

#include <flutter/event_stream_handler_functions.h>
#include <flutter/standard_method_codec.h>

#include "log.h"
#include "pending_call.h"
#include "video_player_error.h"

VideoPlayer::~VideoPlayer() {
  event_sink_ = nullptr;
  if (event_channel_) {
    event_channel_->SetStreamHandler(nullptr);
  }
}

void VideoPlayer::SetUpEventChannel(int32_t player_id,
                                    flutter::BinaryMessenger *messenger) {
  std::string channel_name =
      "tizen/video_player/video_events_" + std::to_string(player_id);
  auto channel =
      std::make_unique<flutter::EventChannel<flutter::EncodableValue>>(
          messenger, channel_name,
          &flutter::StandardMethodCodec::GetInstance());
  auto handler = std::make_unique<
      flutter::StreamHandlerFunctions<flutter::EncodableValue>>(
      [&](const flutter::EncodableValue *arguments,
          std::unique_ptr<flutter::EventSink<>> &&events)
          -> std::unique_ptr<flutter::StreamHandlerError<>> {
        event_sink_ = std::move(events);
        try {
          if (isReady()) {
            SendInitialized();
          } else {
            LOG_INFO("Video Player is not ready.");
          }
        } catch (const VideoPlayerError &error) {
          LOG_ERROR("Failed to get video player state, error(%s, %s)",
                    error.code().c_str(), error.message().c_str());
        }
        return nullptr;
      },
      [&](const flutter::EncodableValue *arguments)
          -> std::unique_ptr<flutter::StreamHandlerError<>> {
        event_sink_ = nullptr;
        return nullptr;
      });
  channel->SetStreamHandler(std::move(handler));

  event_channel_ = std::move(channel);
}

void VideoPlayer::SendInitialized() {
  if (!is_initialized_ && event_sink_) {
    int32_t duration = 0, width = 0, height = 0;
    try {
      duration = GetDuration();
      GetVideoSize(&width, &height);
    } catch (const VideoPlayerError &error) {
      LOG_ERROR("Failed to get video information");
      event_sink_->Error(error.code(), error.message());
      return;
    }

    is_initialized_ = true;
    flutter::EncodableMap result = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("initialized")},
        {flutter::EncodableValue("duration"),
         flutter::EncodableValue(duration)},
        {flutter::EncodableValue("width"), flutter::EncodableValue(width)},
        {flutter::EncodableValue("height"), flutter::EncodableValue(height)},
    };
    event_sink_->Success(flutter::EncodableValue(result));
  }
}

void VideoPlayer::SendBufferingStart() {
  if (event_sink_) {
    flutter::EncodableMap result = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("bufferingStart")},
    };
    event_sink_->Success(flutter::EncodableValue(result));
  }
}

void VideoPlayer::SendBufferingUpdate(int32_t value) {
  if (event_sink_) {
    flutter::EncodableMap result = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("bufferingUpdate")},
        {flutter::EncodableValue("value"), flutter::EncodableValue(value)},
    };
    event_sink_->Success(flutter::EncodableValue(result));
  }
}

void VideoPlayer::SendBufferingEnd() {
  if (event_sink_) {
    flutter::EncodableMap result = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("bufferingEnd")},
    };
    event_sink_->Success(flutter::EncodableValue(result));
  }
}

void VideoPlayer::SendSubtitleUpdate(int32_t duration,
                                     const std::string &text) {
  if (event_sink_) {
    flutter::EncodableMap result = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("subtitleUpdate")},
        {flutter::EncodableValue("duration"),
         flutter::EncodableValue(duration)},
        {flutter::EncodableValue("text"), flutter::EncodableValue(text)},
    };
    event_sink_->Success(flutter::EncodableValue(result));
  }
}

void VideoPlayer::SendPlayCompleted() {
  if (event_sink_) {
    flutter::EncodableMap result = {
        {flutter::EncodableValue("event"),
         flutter::EncodableValue("completed")},
    };
    event_sink_->Success(flutter::EncodableValue(result));
  }
}

void VideoPlayer::SendError(const std::string &error_code,
                            const std::string &error_message) {
  if (event_sink_) {
    event_sink_->Error(error_code, error_message);
  }
}

void VideoPlayer::OnLicenseChallenge(const void *challenge,
                                     unsigned long challenge_len,
                                     void **response,
                                     unsigned long *response_len) {
  const char *method_name = "onLicenseChallenge";
  size_t request_length = challenge_len;
  void *request_buffer = malloc(request_length);
  memcpy(request_buffer, challenge, challenge_len);

  size_t response_length = 0;
  PendingCall pending_call(response, &response_length);

  Dart_CObject c_send_port;
  c_send_port.type = Dart_CObject_kSendPort;
  c_send_port.value.as_send_port.id = pending_call.port();
  c_send_port.value.as_send_port.origin_id = ILLEGAL_PORT;

  Dart_CObject c_pending_call;
  c_pending_call.type = Dart_CObject_kInt64;
  c_pending_call.value.as_int64 = reinterpret_cast<int64_t>(&pending_call);

  Dart_CObject c_method_name;
  c_method_name.type = Dart_CObject_kString;
  c_method_name.value.as_string = const_cast<char *>(method_name);

  Dart_CObject c_request_data;
  c_request_data.type = Dart_CObject_kExternalTypedData;
  c_request_data.value.as_external_typed_data.type = Dart_TypedData_kUint8;
  c_request_data.value.as_external_typed_data.length = request_length;
  c_request_data.value.as_external_typed_data.data =
      static_cast<uint8_t *>(request_buffer);
  c_request_data.value.as_external_typed_data.peer = request_buffer;
  c_request_data.value.as_external_typed_data.callback =
      [](void *isolate_callback_data, void *peer) { free(peer); };

  Dart_CObject *c_request_arr[] = {&c_send_port, &c_pending_call,
                                   &c_method_name, &c_request_data};
  Dart_CObject c_request;
  c_request.type = Dart_CObject_kArray;
  c_request.value.as_array.values = c_request_arr;
  c_request.value.as_array.length =
      sizeof(c_request_arr) / sizeof(c_request_arr[0]);

  pending_call.PostAndWait(send_port_, &c_request);
  LOG_INFO("Received response of challenge (size: %d)", response_length);

  *response_len = response_length;
}
