import os
import sys
import argparse
import glob
import time

import cv2
import numpy as np
import onnxruntime as ort
import yaml

def load_labels(data_yaml):
    with open(data_yaml, 'r') as f:
        data = yaml.safe_load(f)
    names = data['names']
    if isinstance(names, dict):
        labels = [v for k, v in sorted(names.items())]
    else:
        labels = names
    return labels

def is_ultralytics_output(outputs):
    return (
        len(outputs) == 1
        and len(outputs[0].shape) == 3
        and outputs[0].shape[2] >= 6
    )

def is_standard_onnx_output(outputs):
    return (
        len(outputs) == 3
        and outputs[0].shape[-1] == 4
    )

def postprocess(prediction, orig_shape, conf_thres=0.5, iou_thres=0.45):
    # For ONNX output shape (1, 6, N), fx (1, 6, 8400)
    import torch
    from torchvision.ops import nms

    output = prediction[0]  # numpy array (1, 6, N)
    if output.shape[1] == 6:
        # Transpose to (N, 6)
        preds = torch.from_numpy(output[0].T)  # (N, 6)
        boxes = preds[:, 0:4]
        scores = preds[:, 4]
        class_ids = preds[:, 5].long()

        # Convert xywh to xyxy
        boxes_xyxy = torch.zeros_like(boxes)
        boxes_xyxy[:, 0] = boxes[:, 0] - boxes[:, 2] / 2  # xmin
        boxes_xyxy[:, 1] = boxes[:, 1] - boxes[:, 3] / 2  # ymin
        boxes_xyxy[:, 2] = boxes[:, 0] + boxes[:, 2] / 2  # xmax
        boxes_xyxy[:, 3] = boxes[:, 1] + boxes[:, 3] / 2  # ymax

        mask = scores > conf_thres
        boxes_xyxy = boxes_xyxy[mask]
        scores = scores[mask]
        class_ids = class_ids[mask]

        if boxes_xyxy.size(0) == 0:
            return []

        keep = nms(boxes_xyxy, scores, iou_thres)
        results = []
        h0, w0 = orig_shape[:2]
        for idx in keep:
            xmin, ymin, xmax, ymax = boxes_xyxy[idx]
            score = float(scores[idx])
            class_id = int(class_ids[idx])
            # scale to original image size (typisk input 640)
            xmin = max(0, int(xmin / 640 * w0))
            xmax = min(w0, int(xmax / 640 * w0))
            ymin = max(0, int(ymin / 640 * h0))
            ymax = min(h0, int(ymax / 640 * h0))
            results.append([xmin, ymin, xmax, ymax, score, class_id])
        return results

    # fallback til andet output-format hvis nødvendigt
    print("Unrecognized ONNX output shape in postprocess.")
    return []

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument('--model', help='Path to YOLO ONNX model', required=True)
    parser.add_argument('--data', help='Path to data.yaml for class names', required=True)
    parser.add_argument('--source', help='Input source: image/video/cam/folder', required=True)
    parser.add_argument('--thresh', help='Minimum confidence threshold', default=0.5, type=float)
    parser.add_argument('--resolution', help='WxH display size', default=None)
    parser.add_argument('--record', help='Record video output', action='store_true')
    args = parser.parse_args()

    model_path = args.model
    data_yaml = args.data
    img_source = args.source
    min_thresh = args.thresh
    user_res = args.resolution
    record = args.record

    if not os.path.exists(model_path):
        print('ERROR: Model file not found')
        sys.exit(1)
    if not os.path.exists(data_yaml):
        print('ERROR: data.yaml file not found')
        sys.exit(1)

    session = ort.InferenceSession(model_path, providers=['CPUExecutionProvider']) #Cpu only
    #session = ort.InferenceSession(model_path, providers=['CUDAExecutionProvider', 'CPUExecutionProvider']) #cuda cpu fallback
    #session = ort.InferenceSession(model_path, providers=['DmlExecutionProvider']) #intel amd gpu
    labels = load_labels(data_yaml)
    input_shape = session.get_inputs()[0].shape
    img_size = input_shape[2]

    img_ext_list = ['.jpg','.JPG','.jpeg','.JPEG','.png','.PNG','.bmp','.BMP']
    vid_ext_list = ['.avi','.mov','.mp4','.mkv','.wmv']
    if os.path.isdir(img_source):
        source_type = 'folder'
    elif os.path.isfile(img_source):
        _, ext = os.path.splitext(img_source)
        if ext in img_ext_list:
            source_type = 'image'
        elif ext in vid_ext_list:
            source_type = 'video'
        else:
            print(f'File extension {ext} not supported')
            sys.exit(1)
    elif img_source.isdigit():
        source_type = 'camera'
        cam_idx = int(img_source)
    else:
        print(f'Input {img_source} is invalid')
        sys.exit(1)

    resize = False
    if user_res:
        resW, resH = [int(x) for x in user_res.split('x')]
        resize = True

    if record:
        if source_type not in ['video', 'camera']:
            print('Recording only works for video/camera')
            sys.exit(1)
        if not user_res:
            print('Please specify --resolution for recording')
            sys.exit(1)
        record_name = 'demo1.avi'
        record_fps = 30
        recorder = cv2.VideoWriter(
            record_name, cv2.VideoWriter_fourcc(*'MJPG'), record_fps, (resW, resH)
        )

    if source_type == 'image':
        imgs_list = [img_source]
    elif source_type == 'folder':
        imgs_list = []
        for file in glob.glob(os.path.join(img_source, '*')):
            _, ext = os.path.splitext(file)
            if ext in img_ext_list:
                imgs_list.append(file)
    elif source_type == 'video':
        cap = cv2.VideoCapture(img_source)
    elif source_type == 'camera':
        cap = cv2.VideoCapture(cam_idx)
    else:
        imgs_list = []

    bbox_colors = [(164,120,87), (68,148,228), (93,97,209), (178,182,133), (88,159,106), 
                  (96,202,231), (159,124,168), (169,162,241), (98,118,150), (172,176,184)]
    avg_frame_rate = 0
    frame_rate_buffer = []
    fps_avg_len = 200
    img_count = 0
    frame_count = 0

    while True:
        t_start = time.perf_counter()
        if source_type in ['image', 'folder']:
            if img_count >= len(imgs_list):
                print('All images processed. Exiting.')
                break
            img_filename = imgs_list[img_count]
            frame = cv2.imread(img_filename)
            img_count += 1
        elif source_type in ['video', 'camera']:
            ret, frame = cap.read()
            if not ret or frame is None:
                print('End of video/camera. Exiting.')
                break
        else:
            break

        frame_count += 1
        h0, w0 = frame.shape[:2]
        frame_w = w0
        frame_h = h0

        print(f"Frame count: {frame_count} Width: {frame_w} Heigth: {frame_h}\n")

        disp_frame = frame
        if resize:
            disp_frame = cv2.resize(frame, (resW, resH))
        else:
            resW, resH = w0, h0

        img = cv2.cvtColor(frame, cv2.COLOR_BGR2RGB)
        img = cv2.resize(img, (img_size, img_size))
        img = img.transpose(2, 0, 1)
        img = img.astype(np.float32) / 255.0
        img = np.expand_dims(img, axis=0)

        outputs = session.run(None, {session.get_inputs()[0].name: img})
        boxes = postprocess(outputs, frame.shape, conf_thres=min_thresh)

        object_count = 0

        for i, det in enumerate(boxes):
            xmin, ymin, xmax, ymax, conf, classid = det
            classname = labels[classid] if classid < len(labels) else f'id{classid}'
            object_count += 1

            # Normalized
            x_min_norm = xmin / frame_w
            y_min_norm = ymin / frame_h
            x_max_norm = xmax / frame_w
            y_max_norm = ymax / frame_h
            center_x = (xmin + xmax) / 2 / frame_w
            center_y = (ymin + ymax) / 2 / frame_h

            print(
                f"Label: {classname} ID: {classid} Confidence: {conf:.2f} Detection count: {object_count} "
                f"Position: center=({center_x:.4f}, {center_y:.4f}) "
                f"Bounds: xmin={x_min_norm:.4f}, ymin={y_min_norm:.4f}, xmax={x_max_norm:.4f}, ymax={y_max_norm:.4f}"
            )

            # Draw box for visual feedback
            color = bbox_colors[classid % 10]
            scale_x = resW / w0
            scale_y = resH / h0
            xmin_disp = int(xmin * scale_x)
            xmax_disp = int(xmax * scale_x)
            ymin_disp = int(ymin * scale_y)
            ymax_disp = int(ymax * scale_y)
            cv2.rectangle(disp_frame, (xmin_disp, ymin_disp), (xmax_disp, ymax_disp), color, 2)
            label = f'{classname}: {int(conf*100)}%'
            labelSize, baseLine = cv2.getTextSize(label, cv2.FONT_HERSHEY_SIMPLEX, 0.5, 1)
            label_ymin = max(ymin_disp, labelSize[1] + 10)
            cv2.rectangle(disp_frame, (xmin_disp, label_ymin-labelSize[1]-10), (xmin_disp+labelSize[0], label_ymin+baseLine-10), color, cv2.FILLED)
            cv2.putText(disp_frame, label, (xmin_disp, label_ymin-7), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 0, 0), 1)

        t_stop = time.perf_counter()
        frame_rate_calc = float(1/(t_stop - t_start))
        frame_rate_buffer.append(frame_rate_calc)
        if len(frame_rate_buffer) > fps_avg_len:
            frame_rate_buffer.pop(0)
        avg_frame_rate = np.mean(frame_rate_buffer)

        if source_type in ['video', 'camera']:
            cv2.putText(disp_frame, f'FPS: {avg_frame_rate:0.2f}', (10,20), cv2.FONT_HERSHEY_SIMPLEX, .7, (0,255,255), 2)
        cv2.putText(disp_frame, f'Number of objects: {object_count}', (10,40), cv2.FONT_HERSHEY_SIMPLEX, .7, (0,255,255), 2)

        cv2.imshow('YOLO ONNX detection results', disp_frame)
        if record:
            recorder.write(disp_frame)

        sys.stdout.flush()
        if source_type in ['image', 'folder']:
            key = cv2.waitKey()
        else:
            key = cv2.waitKey(5)
        if key == ord('q') or key == ord('Q'):
            break
        elif key == ord('s') or key == ord('S'):
            cv2.waitKey()
        elif key == ord('p') or key == ord('P'):
            cv2.imwrite('capture.png',disp_frame)

    print(f'Average pipeline FPS: {avg_frame_rate:.2f}')
    if source_type in ['video', 'camera']:
        cap.release()
    if record: recorder.release()
    cv2.destroyAllWindows()
