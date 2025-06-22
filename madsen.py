import gi
gi.require_version('Gst', '1.0')
from gi.repository import Gst, GLib
import os
import numpy as np
import cv2
import hailo
import sys

from hailo_apps_infra.hailo_rpi_common import (
    get_caps_from_pad,
    get_numpy_from_buffer,
    app_callback_class,
)
from hailo_apps_infra.detection_pipeline import GStreamerDetectionApp

# -----------------------------------------------------------------------------------------------
# User-defined class to be used in the callback function
# -----------------------------------------------------------------------------------------------
# Inheritance from the app_callback_class
class user_app_callback_class(app_callback_class):
    def __init__(self):
        super().__init__()
        self.new_variable = 42  # New variable example

    def new_function(self):  # New function example
        return "The meaning of life is: "

# -----------------------------------------------------------------------------------------------
# User-defined callback function
# -----------------------------------------------------------------------------------------------

# This is the callback function that will be called when data is available from the pipeline
def app_callback(pad, info, user_data):
    # Get the GstBuffer from the probe info
    buffer = info.get_buffer()
    # Check if the buffer is valid
    if buffer is None:
        return Gst.PadProbeReturn.OK

    # Using the user_data to count the number of frames
    user_data.increment()
    #user_data.use_frame = True
    string_to_print = f"Frame count: {user_data.get_count()} "
    # Get the caps from the pad
    width = 1280
    height = 720
    format, width, height = get_caps_from_pad(pad)
    string_to_print += f"Width: {width} Heigth: {height}\n"
    # If the user_data.use_frame is set to True, we can get the video frame from the buffer
    frame = None
    label = None
    if user_data.use_frame and format is not None and width is not None and height is not None:
        # Get video frame
        frame = get_numpy_from_buffer(buffer, format, width, height)
        #string_to_print += " frame ok "
    # Get the detections from the buffer
    roi = hailo.get_roi_from_buffer(buffer)
    detections = roi.get_objects_typed(hailo.HAILO_DETECTION)

    # Parse the detections
    detection_count = 0
    for detection in detections:
        detection_count += 1
        label = detection.get_label()
        if label != None:
            bbox = detection.get_bbox()
            confidence = detection.get_confidence()
            # Get track ID
            track_id = 0
        
            track = detection.get_objects_typed(hailo.HAILO_UNIQUE_ID)
            if len(track) == 1:
                track_id = track[0].get_id()
            string_to_print += (f"Label: {label} ID: {track_id} Confidence: {confidence:.2f} Detection count: {detection_count} ")
            
            bbox = detection.get_bbox()
            
            # Call the coordinate methods
            x_min = bbox.xmin()
            y_min = bbox.ymin()
            box_width = bbox.width()
            box_height = bbox.height()
            
            # Calculate max coordinates
            x_max = x_min + box_width
            y_max = y_min + box_height
            
            # Calculate center point (these are normalized 0-1)
            center_x = x_min + (box_width / 2)
            center_y = (y_min + (box_height / 2) - 0.22) * 1.83
        
            # Debug print for coordinates
            string_to_print += (f"Position: center=({center_x:.4f}, {center_y:.4f}) "
                               f"Bounds: xmin={x_min:.4f}, ymin={y_min:.4f}, xmax={x_max:.4f}, ymax={y_max:.4f}\n")
            if user_data.use_frame:
                cv2.rectangle(frame, ((int)(x_min * width),(int) (y_min* height)), ((int)(x_max * width),(int) (y_max * height)), (255, 255, 255),1)
                cv2.putText(frame, label,((int)(x_min * width),(int) ((y_min* height) - 10)), cv2.FONT_HERSHEY_SIMPLEX, 1, (255, 255, 255), 1)
    
    if user_data.use_frame:
        cv2.putText(frame,f"Frame count: {user_data.get_count()}", (10, 30), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        
    if user_data.use_frame and label != None:
        # Note: using imshow will not work here, as the callback function is not running in the main thread
        # Let's print the detection count to the frame
        cv2.putText(frame, f"Label: {label} ID: {track_id} Confidence: {confidence:.2f} Detection count: {detection_count}", (10, 70), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        # Example of how to use the new_variable and new_function from the user_data
        # Let's print the new_variable and the result of the new_function to the frame
        #cv2.putText(frame, f"{user_data.new_function()} {user_data.new_variable}", (10, 60), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2)
        # Convert the frame to BGR
        frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
        user_data.set_frame(frame)
    elif user_data.use_frame :
         frame = cv2.cvtColor(frame, cv2.COLOR_RGB2BGR)
         user_data.set_frame(frame)
    print(string_to_print)
    sys.stdout.flush()
    return Gst.PadProbeReturn.OK

if __name__ == "__main__":
    # Create an instance of the user app callback class
    user_data = user_app_callback_class()
    app = GStreamerDetectionApp(app_callback, user_data)
    app.run()
