cmake_minimum_required(VERSION 3.14)
project(ctrl)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenCV REQUIRED)
find_package(X11 REQUIRED)

add_executable(${PROJECT_NAME}
    src/main.cpp
    src/Input.cpp
    src/Detector.cpp
    src/Capture.cpp
    src/FingerDetector.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS} X11::X11 X11::Xtst)