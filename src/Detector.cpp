#include "Detector.hpp"

//Constructor
Detector::Detector(std::string path, double td) {
    PathImg = path;
    treshhold = td;
}

//Detects hand gesture in the frame and returns a key to be pressed
WORD Detector::Detect(cv::Mat& frame) {
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
        return VK_SPACE;
    }
    return 0;
}
