# QtOpencv

Simple program that takes an MP4 file combined with a Hailo-generated text file and, if needed, saves frames from the video file as JPGs with the original frame.

This is intended to be used with a Hailo+ hat on an RPi system.

- Spacebar: Start/stop showing the video file
- Left arrow key: Go back 1 frame (slow)
- Right arrow key: Move forward 1 frame
- Ctrl+S: Save the current frame as filename+framenumber.jpg

You need to use `madsen.py` as the detection file instead of `detection.py`.  
This script creates the text file:

python3 basic_pipelines/madsen.py --hef-path resources/yolo11s.hef --input example.mp4 --labels-json resources/madsen.json >> example.txt

The filename of the text file must be the same as the MP4 file, with only the extension different.

Start QtOpencv in the directory where you want to save the pictures.

I made this program to update my model.
I go through an MP4 file of the object(s) in my model. If the score is low or not present, I save some pictures and put them in the pictures folder of the model.

In some cases, pictures taken with my cellphone are upside down.
In these cases, the bounding box will not be in the right spot, but saving the picture is still in the correct orientation.

to compile it

cmake .

make

You have to have qt5 on your rpi and cmake working.
ask copilot how to install it if needed.
