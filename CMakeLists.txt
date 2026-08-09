cmake_minimum_required(VERSION 3.10)
project(ctrl)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(OpenCV_DIR "C:/Users/ziyad/Downloads/opencv/build/x64/vc16/lib")
find_package(OpenCV REQUIRED)
add_executable(${PROJECT_NAME} src/main.cpp
		src/MainWindow.cpp
		src/Detector.cpp
		src/Capture.cpp
		src/InputController.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE include ${OpenCV_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} PRIVATE ${OpenCV_LIBS})
