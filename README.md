# gst-mlx90640: MLX90640 camera gstreamer plugins

An example gstreamer pipeline:
```
gst-launch-1.0 v4l2src device=/dev/video1 ! \
video/x-raw,width=32,height=26,format=GRAY16_BE ! \
mlx90640 name=thermal thermal.src ! \
video/x-raw,width=32,height=26,format=RGBA ! videoscale ! \
video/x-raw,width=640,height=520 ! mlx90640_bar name=bar ! \
textoverlay name=overlay ! videoconvert ! autovideosink \
thermal.thermal-src ! overlay.text_sink
```
