# QtOpencv
simpel program that takes a mp4 file combined with a hailo generated text fil and if needed save frames from the video file as jpg with the originale frame

this is intended to bee used with hailo+ hat on a rpi system

spacebar start/stop showing the vidio file
left arrow key is 1 frame back (slow)
right arrow 1 frame forward
ctrl+s save current frame as filename+framenumber.jpg

You have to have use madsen.py as detection file instead of  detection.py
python3 basic_pipelines/madsen.py --hef-path resources/yolo11s.hef --input example.mp4 --labels-json resources/madsen.json >>example.txt
The filname on the text file hase to be equal to the mp4 file only the extension is different.

