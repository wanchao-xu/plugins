// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// ignore_for_file: public_member_api_docs, avoid_print

/// An example of using the plugin, controlling lifecycle and playback of the
/// video.

import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:http/http.dart' as http;
import 'package:video_player_videohole/video_player.dart';

void main() {
  runApp(
    MaterialApp(
      home: _App(),
    ),
  );
}

class _App extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    return DefaultTabController(
      length: 5,
      child: Scaffold(
        key: const ValueKey<String>('home_page'),
        appBar: AppBar(
          title: const Text('Video player example'),
          bottom: const TabBar(
            isScrollable: true,
            tabs: <Widget>[
              Tab(icon: Icon(Icons.cloud), text: 'MP4'),
              Tab(icon: Icon(Icons.cloud), text: 'HLS 2.4'),
              Tab(icon: Icon(Icons.cloud), text: 'HLS cenc 2.4'),
              Tab(icon: Icon(Icons.cloud), text: 'HLS cbcs 2.4'),
              Tab(icon: Icon(Icons.cloud), text: 'HLS 2.5'),
              Tab(icon: Icon(Icons.cloud), text: 'HLS cenc 2.5'),
              Tab(icon: Icon(Icons.cloud), text: 'HLS cbcs 2.5'),
              Tab(icon: Icon(Icons.cloud), text: 'HLS 2.6'),
              Tab(icon: Icon(Icons.cloud), text: 'HLS cenc 2.6'),
              Tab(icon: Icon(Icons.cloud), text: 'HLS cbcs 2.6'),
            ],
          ),
        ),
        body: const TabBarView(
          children: <Widget>[
            _NetworkVideo(
              title: 'With remote mp4',
              url: 'https://media.w3.org/2010/05/bunny/trailer.mp4',
            ),
            _NetworkVideo(
              title: 'With bbb_none_2.4.1.m3u8',
              url:
                  'https://devel.uniqcast.com/wv/samples/bbb_none_2.4.1/bbb_none_2.4.1.m3u8',
            ),
            _NetworkVideo(
              title: 'With bbb_cenc_2.4.1.m3u8',
              drmType: DrmType.widevine,
              url:
                  'https://devel.uniqcast.com/wv/samples/bbb_cenc_2.4.1/bbb_cenc_2.4.1.m3u8',
              license: 'https://proxy.uat.widevine.com/proxy',
            ),
            _NetworkVideo(
              title: 'With bbb_cbcs_2.4.1.m3u8',
              drmType: DrmType.widevine,
              url:
                  'https://devel.uniqcast.com/wv/samples/bbb_cbcs_2.4.1/bbb_cbcs_2.4.1.m3u8',
              license: 'https://proxy.uat.widevine.com/proxy',
            ),
            _NetworkVideo(
              title: 'With bbb_none_2.5.1.m3u8',
              url:
                  'https://devel.uniqcast.com/wv/samples/bbb_none_2.5.1/bbb_none_2.5.1.m3u8',
            ),
            _NetworkVideo(
              title: 'With bbb_cenc_2.5.1.m3u8',
              drmType: DrmType.widevine,
              url:
                  'https://devel.uniqcast.com/wv/samples/bbb_cenc_2.5.1/bbb_cenc_2.5.1.m3u8',
              license: 'https://proxy.uat.widevine.com/proxy',
            ),
            _NetworkVideo(
              title: 'With bbb_cbcs_2.5.1.m3u8',
              drmType: DrmType.widevine,
              url:
                  'https://devel.uniqcast.com/wv/samples/bbb_cbcs_2.5.1/bbb_cbcs_2.5.1.m3u8',
              license: 'https://proxy.uat.widevine.com/proxy',
            ),
            _NetworkVideo(
              title: 'With bbb_none_2.6.1.m3u8',
              url:
                  'https://devel.uniqcast.com/wv/samples/bbb_none_2.6.1/bbb_none_2.6.1.m3u8',
            ),
            _NetworkVideo(
              title: 'With bbb_cenc_2.6.1.m3u8',
              drmType: DrmType.widevine,
              url:
                  'https://devel.uniqcast.com/wv/samples/bbb_cenc_2.6.1/bbb_cenc_2.6.1.m3u8',
              license: 'https://proxy.uat.widevine.com/proxy',
            ),
            _NetworkVideo(
              title: 'With bbb_cbcs_2.6.1.m3u8',
              drmType: DrmType.widevine,
              url:
                  'https://devel.uniqcast.com/wv/samples/bbb_cbcs_2.6.1/bbb_cbcs_2.6.1.m3u8',
              license: 'https://proxy.uat.widevine.com/proxy',
            ),
          ],
        ),
      ),
    );
  }
}

class _NetworkVideo extends StatefulWidget {
  const _NetworkVideo({
    Key? key,
    this.title = '',
    this.drmType = DrmType.none,
    required this.url,
    this.license = '',
    this.useLicenseCallback = false,
  }) : super(key: key);

  final String title;
  final DrmType drmType;
  final String url;
  final String license;
  final bool useLicenseCallback;

  @override
  State<_NetworkVideo> createState() => _NetworkVideoState();
}

class _NetworkVideoState extends State<_NetworkVideo> {
  late VideoPlayerController _controller;

  @override
  void initState() {
    super.initState();

    if (widget.drmType != DrmType.none) {
      late DrmConfigs configs;
      if (widget.useLicenseCallback) {
        configs = DrmConfigs(
          type: widget.drmType,
          licenseCallback: (Uint8List challenge) async {
            final http.Response response = await http.post(
              Uri.parse(widget.license),
              body: challenge,
            );
            return response.bodyBytes;
          },
        );
      } else {
        configs = DrmConfigs(
          type: widget.drmType,
          licenseServerUrl: widget.license,
        );
      }

      _controller = VideoPlayerController.network(
        widget.url,
        drmConfigs: configs,
      );
    } else {
      _controller = VideoPlayerController.network(widget.url);
    }

    _controller.addListener(() {
      if (_controller.value.hasError) {
        print(_controller.value.errorDescription);
      }
      setState(() {});
    });
    _controller.initialize().then((_) => setState(() {}));
    _controller.play();
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      child: Column(
        children: <Widget>[
          Container(padding: const EdgeInsets.only(top: 20.0)),
          if (widget.title.isNotEmpty) Text(widget.title),
          Container(
            padding: const EdgeInsets.all(20),
            child: AspectRatio(
              aspectRatio: _controller.value.aspectRatio,
              child: Stack(
                alignment: Alignment.bottomCenter,
                children: <Widget>[
                  VideoPlayer(_controller),
                  ClosedCaption(text: _controller.value.caption.text),
                  _ControlsOverlay(controller: _controller),
                  VideoProgressIndicator(_controller, allowScrubbing: true),
                ],
              ),
            ),
          ),
        ],
      ),
    );
  }
}

class _ControlsOverlay extends StatelessWidget {
  const _ControlsOverlay({required this.controller});

  static const List<Duration> _exampleCaptionOffsets = <Duration>[
    Duration(seconds: -10),
    Duration(seconds: -3),
    Duration(seconds: -1, milliseconds: -500),
    Duration(milliseconds: -250),
    Duration.zero,
    Duration(milliseconds: 250),
    Duration(seconds: 1, milliseconds: 500),
    Duration(seconds: 3),
    Duration(seconds: 10),
  ];
  static const List<double> _examplePlaybackRates = <double>[
    0.25,
    0.5,
    1.0,
    1.5,
    2.0,
    3.0,
    5.0,
    10.0,
  ];

  final VideoPlayerController controller;

  @override
  Widget build(BuildContext context) {
    return Stack(
      children: <Widget>[
        AnimatedSwitcher(
          duration: const Duration(milliseconds: 50),
          reverseDuration: const Duration(milliseconds: 200),
          child: controller.value.isPlaying
              ? const SizedBox.shrink()
              : Container(
                  color: Colors.black26,
                  child: const Center(
                    child: Icon(
                      Icons.play_arrow,
                      color: Colors.white,
                      size: 100.0,
                      semanticLabel: 'Play',
                    ),
                  ),
                ),
        ),
        GestureDetector(
          onTap: () {
            controller.value.isPlaying ? controller.pause() : controller.play();
          },
        ),
        Align(
          alignment: Alignment.topLeft,
          child: PopupMenuButton<Duration>(
            initialValue: controller.value.captionOffset,
            tooltip: 'Caption Offset',
            onSelected: (Duration delay) {
              controller.setCaptionOffset(delay);
            },
            itemBuilder: (BuildContext context) {
              return <PopupMenuItem<Duration>>[
                for (final Duration offsetDuration in _exampleCaptionOffsets)
                  PopupMenuItem<Duration>(
                    value: offsetDuration,
                    child: Text('${offsetDuration.inMilliseconds}ms'),
                  )
              ];
            },
            child: Padding(
              padding: const EdgeInsets.symmetric(
                // Using less vertical padding as the text is also longer
                // horizontally, so it feels like it would need more spacing
                // horizontally (matching the aspect ratio of the video).
                vertical: 12,
                horizontal: 16,
              ),
              child: Text('${controller.value.captionOffset.inMilliseconds}ms'),
            ),
          ),
        ),
        Align(
          alignment: Alignment.topRight,
          child: PopupMenuButton<double>(
            initialValue: controller.value.playbackSpeed,
            tooltip: 'Playback speed',
            onSelected: (double speed) {
              controller.setPlaybackSpeed(speed);
            },
            itemBuilder: (BuildContext context) {
              return <PopupMenuItem<double>>[
                for (final double speed in _examplePlaybackRates)
                  PopupMenuItem<double>(
                    value: speed,
                    child: Text('${speed}x'),
                  )
              ];
            },
            child: Padding(
              padding: const EdgeInsets.symmetric(
                // Using less vertical padding as the text is also longer
                // horizontally, so it feels like it would need more spacing
                // horizontally (matching the aspect ratio of the video).
                vertical: 12,
                horizontal: 16,
              ),
              child: Text('${controller.value.playbackSpeed}x'),
            ),
          ),
        ),
      ],
    );
  }
}
