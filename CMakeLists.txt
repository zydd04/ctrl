cmake_minimum_required(VERSION 3.14)
project(ctrl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(X11 REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

set(CMAKE_AUTOMOC ON) # generates the Qt signal/slot code for Q_OBJECT classes

# MediaPipe hand models (ONNX exports from the OpenCV model zoo), fetched once
set(MODEL_DIR ${CMAKE_SOURCE_DIR}/model)
set(MODEL_URL https://github.com/opencv/opencv_zoo/raw/main/models)
foreach(MODEL palm_detection_mediapipe/palm_detection_mediapipe_2023feb.onnx
              handpose_estimation_mediapipe/handpose_estimation_mediapipe_2023feb.onnx)
    get_filename_component(MODEL_FILE ${MODEL} NAME)
    if(NOT EXISTS ${MODEL_DIR}/${MODEL_FILE})
        message(STATUS "Downloading ${MODEL_FILE}")
        file(DOWNLOAD ${MODEL_URL}/${MODEL} ${MODEL_DIR}/${MODEL_FILE} SHOW_PROGRESS STATUS DL_STATUS)
        list(GET DL_STATUS 0 DL_CODE)
        if(NOT DL_CODE EQUAL 0)
            file(REMOVE ${MODEL_DIR}/${MODEL_FILE})
            message(FATAL_ERROR "Could not download ${MODEL_FILE}: ${DL_STATUS}")
        endif()
    endif()
endforeach()

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/Input.cpp
    src/Capture.cpp
    src/FingerDetector.cpp
    src/CameraThread.cpp
    src/MouseController.cpp
    # header with Q_OBJECT, listed so AUTOMOC finds it in include/
    include/CameraThread.hpp
)

target_include_directories(${PROJECT_NAME} PRIVATE include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} X11::X11 X11::Xtst Qt6::Widgets)
target_compile_definitions(${PROJECT_NAME} PRIVATE CTRL_MODEL_DIR="${MODEL_DIR}")
