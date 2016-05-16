#include <iostream>
#include <vector>
#include <string>
#include <thread>
#include <chrono>

#include <csignal>


#include <opencv2/opencv.hpp>
#include "ApiController.h"
#include "Common/ErrorCodeToMessage.h"

#include "FindCorners.h"

static bool stop = false;
void sigIntHandler(int signal) {
  stop = true;
}



int main(int argc, char **argv) {

  std::signal(SIGINT, sigIntHandler);

  AVT::VmbAPI::ApiController avt_cam;

  VmbErrorType err;

  if (VmbErrorSuccess != avt_cam.StartUp()) {
    std::cerr << "avt_cam.StartUp() failed!\n";
    return -1;
  }

  AVT::VmbAPI::CameraPtrVector cameras_ptr = avt_cam.GetCameraList();

  std::vector<std::string> cams;
  for (auto cam_ptr : cameras_ptr) {
    std::string str_cam_id = "";
    if (VmbErrorSuccess == cam_ptr->GetID(str_cam_id))
      cams.push_back(str_cam_id);
  }
  

  
  std::cout << "cams[0]: " << cams[0] << "\n";
  std::chrono::steady_clock::time_point start =
      std::chrono::steady_clock::now();
  err = avt_cam.OpenCamera(cams[0]);
  err = avt_cam.LoadSettings("/home/fans/Vimba_2_0/bgr8.xml");
  err = avt_cam.StartContinuousImageAcquisition(cams[0]);
  std::chrono::steady_clock::time_point end =
      std::chrono::steady_clock::now();
  std::cout << "start took "
            << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms.\n";
  if (VmbErrorSuccess != err ) {
    std::cerr << avt_cam.ErrorCodeToMessage(err) << std::endl;
    return -1;
  }


  AVT::VmbAPI::FeaturePtr p_feature;
  cameras_ptr[0]->GetFeatureByName("GevTimestampTickFrequency", p_feature);
  VmbInt64_t timestamp_freq = 0;
  err = p_feature->GetValue(timestamp_freq);
  if (VmbErrorSuccess == err) {
    std::cerr << "timestamp_freq: " << timestamp_freq;
  } else {
    std::cerr << avt_cam.ErrorCodeToMessage(err) << std::endl;
  }

  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  int image_width = avt_cam.GetWidth();
  int image_height = avt_cam.GetHeight();

  cv::Scalar RED = cv::Scalar(0, 0, 255);
  
  while (!stop) {
    AVT::VmbAPI::FramePtr p_frame = avt_cam.GetFrame();
    if (SP_ISNULL(p_frame)) {
      continue;
    } 

    VmbUchar_t *pImageBuffer;
    p_frame->GetImage(pImageBuffer);

    cv::Mat bgr(image_height, image_width, CV_8UC3, (void *)pImageBuffer);

    cv::Mat gray(image_height, image_width, CV_8UC1);
    cv::cvtColor(bgr, gray, CV_BGR2GRAY);

    std::vector<cv::Point2f> corners;
    find_corners(gray, corners);
    std::cout << "corners size: " << corners.size() << std::endl;

    std::for_each(corners.begin(), corners.end(),
                  [&](cv::Point2f &pt){cv::circle(bgr, pt, 2, RED, -1);});
    
    
    cv::imshow("mat", bgr);
    cv::waitKey(10);
  }

  avt_cam.StopContinuousImageAcquisition();
  avt_cam.ClearFrameQueue();
  avt_cam.ShutDown();
  
  return 0;
}
