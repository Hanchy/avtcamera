#include "AVTCamera.h"
#include "Common/StreamSystemInfo.h"
#include "Common/ErrorCodeToMessage.h"


enum    { NUM_FRAMES=3, };


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
  this->Close();
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
  VmbErrorType err = VmbErrorSuccess;
  if (is_opened_) {
    is_opened_ = false;
    settings_loaded_ = false;
    camera_id_ = "";
    width_ = 0;
    height_ = 0;
    FPS_ = 0;
    err = p_camera_->Close();
    p_camera_ = nullptr;
  }
  return err;
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

  res = p_camera_->GetFeatureByName("GevTimestamptickfrequency", pFeature);
  if (VmbErrorSuccess == res)
    pFeature->GetValue(timestamp_frq_);

  res = p_camera_->GetFeatureByName("GevTimestampValue", p_timestamp_);

  return res;
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

VmbErrorType AVTCamera::GetFeatureByName(const std::string &_feature_name,
                                         FeaturePtr &_p_feature) {
  return is_opened_?
      p_camera_->GetFeatureByName(_feature_name.c_str(), _p_feature) :
      VmbErrorDeviceNotOpen;
}



VmbErrorType        AVTCamera::StartContinuousImageAcquisition() {
  if (!is_opened_)
    return VmbErrorDeviceNotOpen;
  
  SP_SET(p_frame_observer_, new FrameObserver(p_camera_));
  ResetTimestamp();
  p_camera_->StartContinuousImageAcquisition(NUM_FRAMES, p_frame_observer_);
}



VmbErrorType        AVTCamera::StopContinuousImageAcquisition() {
  // Stop streaming
  return p_camera_->StopContinuousImageAcquisition();

  // Close camera
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

void        AVTCamera::ClearFrameQueue() {
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


VmbErrorType AVTCamera::GetRawImage(VmbUchar_t *&_p_raw_image,
                                    double &_time_stamp) {

  AVT::VmbAPI::FramePtr p_frame = GetFrame();
  while (SP_ISNULL(p_frame)) {
    p_frame = GetFrame();
  }

  VmbUint64_t timestamp;
  p_frame->GetTimestamp(timestamp);
  _time_stamp =
      static_cast<double>(timestamp) / static_cast<double>(timestamp_frq_);
  
  VmbErrorType res;
  res = p_frame->GetImage(_p_raw_image);
  return res;
}


VmbErrorType        AVTCamera::GetRawImage(VmbUchar_t *&_p_raw_image) {
  AVT::VmbAPI::FramePtr p_frame = GetFrame();
  
  while (SP_ISNULL(p_frame)) {
    p_frame = GetFrame();
  }

  VmbErrorType res;
  res = p_frame->GetImage(_p_raw_image);

  return res;
}


