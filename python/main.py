import socket
import cv2
import numpy as np
from ultralytics import YOLO
from collections import deque

# Connect to the C++ framing router port
SERVER_IP = "127.0.0.1"
PYTHON_PORT = 1936

print("[AI Engine] Loading YOLOv8-Pose tracking network...")
model = YOLO('yolov8n-pose.pt')
hip_history = deque(maxlen=10)

def detect_action(history, frame_shape):
    if len(history) < 5: return "Calibrating..."
    h, w, _ = frame_shape
    dx = (history[-1][0] - history[0][0]) * w
    dy = (history[-1][1] - history[0][1]) * h
    
    if dy < -30: return "jump"
    if dx > 25: return "right"
    if dx < -25: return "left"
    
    all_dx = np.diff([pt[0] * w for pt in history])
    all_dy = np.diff([pt[1] * h for pt in history])
    total_movement = np.mean(np.sqrt(all_dx**2 + all_dy**2))
    return "walk/run" if total_movement > 4.5 else "stionary"

def recv_all(sock, length):
    data = b""
    while len(data) < length:
        packet = sock.recv(length - len(data))
        if not packet: return None
        data += packet
    return data

def run_engine():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    try:
        sock.connect((SERVER_IP, PYTHON_PORT))
        print("[AI Engine] Successfully synched with C++ Router.")
    except Exception as e:
        print(f"[Error] Failed to connect: {e}")
        return

    try:
        while True:
            # 1. Read the 4-byte length header prefix
            header = recv_all(sock, 4)
            if not header: break
            frame_length = int.from_bytes(header, byteorder='little')

            # 2. Read the complete image byte payload
            frame_bytes = recv_all(sock, frame_length)
            if not frame_bytes: break

            # 3. Decode frame and pass into ML network
            frame_array = np.frombuffer(frame_bytes, dtype=np.uint8)
            frame = cv2.imdecode(frame_array, cv2.IMREAD_COLOR)

            if frame is not None:
                results = model.track(source=frame, persist=True, verbose=False)
                action_label = "Scanning environment..."
                
                if results and len(results) > 0 and results[0].keypoints is not None:
                    kpts = results[0].keypoints.xyn.cpu().numpy()
                    if len(kpts) > 0 and kpts.shape[1] > 12:
                        left_hip, right_hip = kpts[0][11], kpts[0][12]
                        if left_hip[0] > 0 and right_hip[0] > 0:
                            center_hip = ((left_hip[0] + right_hip[0]) / 2.0, (left_hip[1] + right_hip[1]) / 2.0)
                            hip_history.append(center_hip)
                            action_label = detect_action(hip_history, frame.shape)
                    frame = results[0].plot()

                cv2.putText(frame, f"Action: {action_label}", (30, 50), 
                            cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 255, 0), 2, cv2.LINE_AA)
                cv2.imshow("CTRLai Real-time Action Recognition Tracker", frame)

            if cv2.waitKey(1) & 0xFF == ord('q'): break
    finally:
        sock.close()
        cv2.destroyAllWindows()

if __name__ == "__main__":
    run_engine()
