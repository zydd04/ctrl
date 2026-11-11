# ctrl

Control your computer with your hand, using only a webcam.

| Gesture | Action |
| --- | --- |
| Point with the index finger (other fingers closed) | Move the mouse |
| Touch thumb and index together | Click (keep them together to drag) |
| Open hand (5 fingers) | Press space |

| Key (while the ctrl window is focused) | Action |
| --- | --- |
| `P` | Pause / resume the hand control |
| `Q` | Quit |

A C++ Qt desktop app. Hand tracking uses the MediaPipe hand models, run with OpenCV.

`Tech: C++, OpenCV, Qt, MediaPipe`

## Requirements

- Linux with an **X11** session (Wayland is not supported)
- A webcam at `/dev/video0`
- An internet connection for the first build (it downloads the hand models, about 8 MB)

## Installation

These steps are for Ubuntu / Debian.

1. Install the tools and libraries:

   ```bash
   sudo apt install build-essential cmake libopencv-dev qt6-base-dev libx11-dev libxtst-dev
   ```

2. Get the code:

   ```bash
   git clone https://github.com/zydd04/ctrl.git
   cd ctrl
   ```

3. Build it:

   ```bash
   cmake -S . -B build
   cmake --build build
   ```

   The first `cmake` command downloads the two hand models into the `model/` folder.

4. Run it:

   ```bash
   ./build/ctrl
   ```

## How to use

A window opens and shows the camera. Your hand is drawn on the picture when it is found.

- The box on the picture is the area that stands for your screen. Move your index
  fingertip inside it to move the mouse. The box turns yellow while you are pointing.
- Close the window to stop controlling the mouse.

## Troubleshooting

- **"Webcam not found"**: check that the webcam is plugged in and not used by another
  app. If it is a permission problem, add yourself to the `video` group with
  `sudo usermod -aG video $USER`, then log out and back in.
- **"Could not load the hand models"**: the download failed. Delete the `model/` folder
  and run `cmake -S . -B build` again.
- **"Cannot open the X display"**: you are probably on Wayland. Pick an "Xorg" / "X11"
  session on the login screen.

## Project layout

| File | What it does |
| --- | --- |
| `src/main.cpp` | Opens the window |
| `src/CameraThread.cpp` | The main loop: webcam, hand, mouse, space |
| `src/FingerDetector.cpp` | Finds the hand and counts the fingers |
| `src/MouseController.cpp` | Turns the hand into mouse moves and clicks |
| `src/Input.cpp` | Sends the mouse and keyboard events |
