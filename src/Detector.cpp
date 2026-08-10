#include "Detector.hpp"

//Constructor
Detector::Detector(std::unordered_map<std::string, WORD> gTk, double td) {
    gestureToKey = gTk;
    treshhold = td;
}

//Detects hand gesture in the frame and returns a key to be pressed
WORD Detector::Detect(cv::Mat& frame) {
    std::string PathImg; // = gestureToKey[JUMP] example 
    cv::Mat obj = cv::imread(PathImg);
    cv::Mat grayObj;
    cv::cvtColor(obj, grayObj, cv::COLOR_BGR2GRAY);
    cv::Mat grayFrame;
    cv::cvtColor(frame, grayFrame, cv::COLOR_BGR2GRAY);
    cv::Mat result;
    cv::matchTemplate(grayFrame, grayObj, result, cv::TM_CCOEFF_NORMED);
    double minVal, maxVal;
    cv::Point minLoc, maxLoc;
    cv::minMaxLoc(result, &minVal, &maxVal, &minLoc, &maxLoc);
    if (maxVal > treshhold) {
        return gestureToKey[PathImg]; //NEEDS IMPLEMENTATION : ALLOW MULTIPLE GESTURES DETECTION
    }
    return 0;
}
