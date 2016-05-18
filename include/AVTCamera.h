#pragma once

#include <string>
#include <memory>

#include <VimbaCPP/Include/VimbaCPP.h>

#include <opencv2/opencv.hpp>

#include "CameraObserver.h"
#include "FrameObserver.h"

using namespace AVT::VmbAPI;


class AVTCamera {
public:

  AVTCamera(VimbaSystem *_Vmb_Sys,
            const std::string &_camera_id);
  ~AVTCamera();
  
  VmbErrorType        Open();
  VmbErrorType        Close();
  VmbErrorType        LoadSettings(const std::string &_settings_file);
  VmbErrorType        ResetTimestamp();
  VmbErrorType        StartContinuousImageAcquisition();
  VmbErrorType        StopContinuousImageAcquisition();

  VmbErrorType        GetFeatureByName(const std::string &_feature_name,
                                       FeaturePtr &_p_feature);
  

  int                 GetWidth() const;
  int                 GetHeight() const;
  double              GetFPS() const;
  FramePtr            GetFrame() const;

  VmbErrorType        GetRawImage(VmbUchar_t *&_p_raw_image,
                                  double &_time_stamp);
  VmbErrorType        GetRawImage(VmbUchar_t *&_p_raw_image);
  
  VmbErrorType        QueueFrame(FramePtr _p_frame);
  void                ClearFrameQueue();

  FrameObserver*      GetFrameObserver();

  std::string         ErrorCodeToMessage(VmbErrorType _err) const;

  std::string         GetCameraID() const;

  CameraPtr                      p_camera_;
  
private:
  VimbaSystem                    *p_vimba_system_;
  IFrameObserverPtr              p_frame_observer_;
                                 
  std::string                    camera_id_;
                                 
  // VmbInt64_t                     nPixelFormat_;
  // The current width           
  VmbInt64_t                     width_;
  // The current height          
  VmbInt64_t                     height_;
  // The current FPS             
  double                         FPS_;

  // The Frequency of image timestamp
  VmbInt64_t                     timestamp_frq_;

  
  bool                           is_opened_;
  bool                           settings_loaded_;


  FeaturePtr                     p_timestamp_;
  
};

typedef std::shared_ptr<AVTCamera> AVTCameraPtr;
typedef std::vector<AVTCamera> AVTCameraVector;
typedef std::vector<std::shared_ptr<AVTCamera>> AVTCameraPtrVector;


