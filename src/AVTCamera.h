#pragma once

#include <string>
#include <memory>

#include <VimbaCPP/Include/VimbaCPP.h>

#include "CameraObserver.h"
#include "FrameObserver.h"

using namespace AVT::VmbAPI;

// class ApiController;
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


  int                 GetWidth() const;
  int                 GetHeight() const;
  double              GetFPS() const;
  FramePtr            GetFrame() const;

  VmbErrorType        QueueFrame(FramePtr _p_frame);
  void                ClearQueueFrame();

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

  bool                           is_opened_;
  bool                           settings_loaded_;
  
};
