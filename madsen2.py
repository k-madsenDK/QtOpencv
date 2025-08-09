import os
import sys
from pathlib import Path

import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib

import numpy as np
import cv2
import hailo

# ------------------------------------------------------------------------------
# Compatibility layer for Hailo Apps SDK (pre-4.25 and 4.25+)
# ------------------------------------------------------------------------------
# Try the newer/changed import paths first, then fall back to the older ones.
GET_CAPS = None
GET_NUMPY = None
APP_CB_BASE = None
DET_APP = None

_import_errors = []

try:
    # Newer-style (example: 4.25+ on some distros)
    from hailo_apps_infra.hailo_rpi_common import (
        get_caps_from_pad as _get_caps_from_pad_infra,
        get_numpy_from_buffer as _get_numpy_from_buffer_infra,
        app_callback_class as _app_callback_class_infra,
    )
    from hailo_apps_infra.detection_pipeline import GStreamerDetectionApp as _GStreamerDetectionApp_infra

    GET_CAPS = _get_caps_from_pad_infra
    GET_NUMPY = _get_numpy_from_buffer_infra
    APP_CB_BASE = _app_callback_class_infra
    DET_APP = _GStreamerDetectionApp_infra
except Exception as e:
    _import_errors.append(("hailo_apps_infra", e))

if GET_CAPS is None:
    try:
        # Older-style (pre-4.25)
        from hailo_apps.hailo_app_python.core.common.buffer_utils import (
            get_caps_from_pad as _get_caps_from_pad_legacy,
            get_numpy_from_buffer as _get_numpy_from_buffer_legacy,
        )
        from hailo_apps.hailo_app_python.core.gstreamer.gstreamer_app import app_callback_class as _app_callback_class_legacy
        from hailo_apps.hailo_app_python.apps.detection.detection_pipeline import (
            GStreamerDetectionApp as _GStreamerDetectionApp_legacy,
        )

        GET_CAPS = _get_caps_from_pad_legacy
        GET_NUMPY = _get_numpy_from_buffer_legacy
        APP_CB_BASE = _app_callback_class_legacy
        DET_APP = _GStreamerDetectionApp_legacy
    except Exception as e:
        _import_errors.append(("hailo_apps.hailo_app_python", e))

if any(x is None for x in (GET_CAPS, GET_NUMPY, APP_CB_BASE, DET_APP)):
    msg = ["Failed to import a compatible Hailo Apps SDK (tried new and legacy paths)."]
    for src, err in _import_errors:
        msg.append(f"- {src}: {repr(err)}")
    raise ImportError("\n".join(msg))

# ------------------------------------------------------------------------------
# Optional: auto-configure HAILO_ENV_FILE if a .env exists alongside this file
# ------------------------------------------------------------------------------
try:
    project_root = Path(__file__).resolve().parent
    env_file = project_root / ".env"
    if env_file.exists():
        os.environ.setdefault("HAILO_ENV_FILE", str(env_file))
except Exception:
    pass

# ------------------------------------------------------------------------------
# User-defined class to be used in the callback function
# ------------------------------------------------------------------------------
class user_app_callback_class(APP_CB_BASE):
    def __init__(self):
        super().__init__()
        self.new_variable = 42  # New variable example

    def new_function(self):  # New function example
        return "The meaning of life is: "

# ------------------------------------------------------------------------------
# User-defined callback function
# ------------------------------------------------------------------------------
def app_callback(pad, info, user_data):
    buffer = info.get_buffer()
    if buffer is None:
        return Gst.PadProbeReturn.OK

    # Frame counter
    user_data.increment()
    string_to_print = f"Frame count: {user_data.get_count()} "

    # Caps
    fmt = None
    width = None
    height = None
    try:
        fmt, width, height = GET_CAPS(pad)
    except Exception:
        # Some SDKs can behave differently; keep robust defaults
        pass

    if width is not None and height is not None:
        string_to_print += f"Width: {width} Heigth: {height}\n"
    else:
        string_to_print += "(no caps)\n"

    # Optional frame extraction
    frame = None
    if getattr(user_data, "use_frame", False) and all(v is not None for v in (fmt, width, height)):
        try:
            frame = GET_NUMPY(buffer, fmt, width, height)
        except Exception:
            frame = None

    # Detections
    try:
        roi = hailo.get_roi_from_buffer(buffer)
        detections = roi.get_objects_typed(hailo.HAILO_DETECTION)
    except Exception:
        detections = []

    detection_count = 0

    # Draw per-detection info to avoid relying on "last detection" variables
    if frame is not None:
        vis_bgr = None  # Will allocate upon first draw

    for detection in detections:
        label = detection.get_label()
        if label is None:
            continue

        bbox = detection.get_bbox()
        confidence = detection.get_confidence()
        track_id = 0

        # Unique ID (tracker)
        try:
            track = detection.get_objects_typed(hailo.HAILO_UNIQUE_ID)
            if len(track) == 1:
                track_id = track[0].get_id()
        except Exception:
            track_id = 0

        detection_count += 1
        string_to_print += f"Label: {label} ID: {track_id} Confidence: {confidence:.2f} Detection count: {detection_count} "

        # Coordinates (normalized 0..1)
        try:
            x_min = bbox.xmin()
            y_min = bbox.ymin()
            box_width = bbox.width()
            box_height = bbox.height()
            x_max = x_min + box_width
            y_max = y_min + box_height
            center_x = x_min + (box_width / 2.0)
            center_y = (y_min + (box_height / 2.0) - 0.22) * 1.83  # your custom mapping
            string_to_print += (
                f"Position: center=({center_x:.4f}, {center_y:.4f}) "
                f"Bounds: xmin={x_min:.4f}, ymin={y_min:.4f}, xmax={x_max:.4f}, ymax={y_max:.4f}\n"
            )
        except Exception:
            x_min = y_min = x_max = y_max = None
            string_to_print += "(no bbox)\n"

        # Visualization
        if frame is not None and all(v is not None for v in (x_min, y_min, x_max, y_max)):
            try:
                # Convert once when we first need to draw
                if vis_bgr is None:
                    vis_bgr = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)

                p1 = (int(x_min * width), int(y_min * height))
                p2 = (int(x_max * width), int(y_max * height))
                cv2.rectangle(vis_bgr, p1, p2, (255, 255, 255), 1)
                cv2.putText(
                    vis_bgr,
                    f"{label} {confidence:.2f} id:{track_id}",
                    (p1[0], max(0, p1[1] - 10)),
                    cv2.FONT_HERSHEY_SIMPLEX,
                    0.7,
                    (255, 255, 255),
                    1,
                    cv2.LINE_AA,
                )
            except Exception:
                pass

    # Framecount overlay on the frame
    if frame is not None:
        try:
            if 'vis_bgr' in locals() and vis_bgr is not None:
                draw_target = vis_bgr
            else:
                draw_target = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)

            cv2.putText(
                draw_target,
                f"Frame count: {user_data.get_count()}",
                (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX,
                1,
                (0, 255, 0),
                2,
                cv2.LINE_AA,
            )
            user_data.set_frame(draw_target)
        except Exception:
            pass

    # Print out summary per frame
    print(string_to_print)
    sys.stdout.flush()
    return Gst.PadProbeReturn.OK

def update_framecount_overlay(pipeline, user_data):
    try:
        overlay = pipeline.get_by_name("framecount_overlay")
        if overlay is not None:
            overlay.set_property("text", f"Frame: {user_data.get_count()}")
    except Exception:
        pass
    return True  # keep timer running

def _maybe_init_gst():
    try:
        Gst.init(None)
    except Exception:
        pass

if __name__ == "__main__":
    _maybe_init_gst()
    user_data = user_app_callback_class()
    app = DET_APP(app_callback, user_data)
    # Update textoverlay element (if present) every 100 ms
    GLib.timeout_add(100, update_framecount_overlay, app.pipeline, user_data)
    app.run()
