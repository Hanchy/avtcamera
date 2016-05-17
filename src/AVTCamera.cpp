#include "AVTCamera.h"
#include "Common/StreamSystemInfo.h"
#include "Common/ErrorCodeToMessage.h"

AVTCamera::AVTCamera(VimbaSystem *_Vmb_Sys,
                     const std::string &_camera_id): p_vimba_system_(NULL){
  if (_Vmb_Sys != nullptr) {
    p_vimba_system_ = _Vmb_Sys;
    camera_id_ = _camera_id;
  }
  
  is_opened_ = false;
  settings_loaded_ = false;
  p_camera_ = nullptr;

}


AVTCamera::~AVTCamera() {
  
}


VmbErrorType        AVTCamera::Open() {
  if (SP_ISNULL(p_vimba_system_))
    return VmbErrorApiNotStarted;

  VmbErrorType res;
  res = p_vimba_system_->OpenCameraByID(camera_id_.c_str(),
                                        VmbAccessModeFull,
                                        p_camera_);
  if (VmbErrorSuccess == res) {
    FeaturePtr pCommandFeature;
    res = p_camera_->GetFeatureByName("GVSPAdjustPacketSize", pCommandFeature);
    if (VmbErrorSuccess == res) {
      res = SP_ACCESS(pCommandFeature)->RunCommand();
      if (VmbErrorSuccess == res) {
        bool bIsCommandDone = false;
        do {
          SP_ACCESS(pCommandFeature)->IsCommandDone(bIsCommandDone);
        } while (false == bIsCommandDone);
      }
    }
  }
  
  is_opened_ = (VmbErrorSuccess == res)? true : false;

  return res;
}


VmbErrorType AVTCamera::Close() {
  return is_opened_? p_camera_->Close() : VmbErrorSuccess;
}


VmbErrorType AVTCamera::LoadSettings(const std::string &_settings_file) {
  if (!is_opened_)
    return VmbErrorDeviceNotOpen;

  VmbFeaturePersistSettings_t settingsStruct;
  settingsStruct.loggingLevel = 4;
  settingsStruct.maxIterations = 5;
  settingsStruct.persistType = VmbFeaturePersistNoLUT;

  VmbErrorType res;
  res = p_camera_->LoadCameraSettings(_settings_file, &settingsStruct);

  if (VmbErrorSuccess != res)
    return res;

  FeaturePtr pFeature;
  res = p_camera_->GetFeatureByName("Width", pFeature);
  if (VmbErrorSuccess == res)
    SP_ACCESS(pFeature)->GetValue(width_);
  res = p_camera_->GetFeatureByName("Height", pFeature);
  if (VmbErrorSuccess == res)
    pFeature->GetValue(height_);

  if (VmbErrorSuccess == res)
    settings_loaded_ = true;

  FeaturePtr pFeatureFPS ;
  res = p_camera_->GetFeatureByName("AcquisitionFrameRateAbs", pFeatureFPS);
  if( VmbErrorSuccess != res) {
    // lets try other
    res = p_camera_->GetFeatureByName("AcquisitionFrameRate", pFeatureFPS);
  }
  if( VmbErrorSuccess == res ) {
    res = SP_ACCESS(pFeatureFPS)->GetValue( FPS_ );
  }
}


VmbErrorType AVTCamera::ResetTimestamp() {
  if (!is_opened_)
    return VmbErrorDeviceNotOpen;
  
  FeaturePtr pFeature;
  VmbErrorType err = p_camera_->GetFeatureByName("GevTimestampControlReset",
                                                 pFeature);
  if (VmbErrorSuccess == err) {
    pFeature->RunCommand();
    bool bIsCommandDone = false;
    do {
      SP_ACCESS(pFeature)->IsCommandDone(bIsCommandDone);
    } while(false == bIsCommandDone);
  }
  return err;
}


VmbErrorType        AVTCamera::StartContinuousImageAcquisition() {
  if (!is_opened_)
    return VmbErrorDeviceNotOpen;
  
  SP_SET(p_frame_observer_, new FrameObserver(p_camera_));
  ResetTimestamp();
  p_camera_->StartContinuousImageAcquisition(3, p_frame_observer_);
}



VmbErrorType        AVTCamera::StopContinuousImageAcquisition() {
  // Stop streaming
  p_camera_->StopContinuousImageAcquisition();

  // Close camera
  return  p_camera_->Close();
}


int                 AVTCamera::GetWidth() const {
  return is_opened_? width_ : -1;
}

int                 AVTCamera::GetHeight() const {
  return is_opened_? height_ : -1;
}

double              AVTCamera::GetFPS() const {
  return is_opened_? FPS_ : -1;
}

FramePtr            AVTCamera::GetFrame() const {
  return SP_DYN_CAST( p_frame_observer_, FrameObserver )->GetFrame();
}

VmbErrorType        AVTCamera::QueueFrame(FramePtr _p_frame) {
  return p_camera_->QueueFrame( _p_frame );
}

void        AVTCamera::ClearQueueFrame() {
  SP_DYN_CAST( p_frame_observer_,FrameObserver )->ClearFrameQueue();
}


FrameObserver*      AVTCamera::GetFrameObserver() {
  return SP_DYN_CAST( p_frame_observer_, FrameObserver ).get();
}

std::string         AVTCamera::ErrorCodeToMessage(VmbErrorType _err) const {
  return AVT::VmbAPI::Examples::ErrorCodeToMessage( _err );
}

std::string         AVTCamera::GetCameraID() const {
  return camera_id_;
}
