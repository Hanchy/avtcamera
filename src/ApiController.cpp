/*=============================================================================
  Copyright (C) 2012 - 2016 Allied Vision Technologies.  All Rights Reserved.

  Redistribution of this file, in original or modified form, without
  prior written consent of Allied Vision Technologies is prohibited.

  -------------------------------------------------------------------------------

  File:        ApiController.cpp

  Description: Implementation file for the ApiController helper class that
  demonstrates how to implement an asynchronous, continuous image
  acquisition with VimbaCPP.

  -------------------------------------------------------------------------------

  THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR IMPLIED
  WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF TITLE,
  NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR  PURPOSE ARE
  DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT,
  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
  AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
  TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  =============================================================================*/

#include <sstream>
#include <iostream>
#include "ApiController.h"
#include "Common/StreamSystemInfo.h"
#include "Common/ErrorCodeToMessage.h"

namespace AVT {
namespace VmbAPI {


ApiController::ApiController()
    // Get a reference to the Vimba singleton
    : m_system( VimbaSystem::GetInstance() )
{
  m_pCameras.clear();
}

ApiController::~ApiController()
{
}

//
// Translates Vimba error codes to readable error messages
//
// Parameters:
//  [in]    eErr        The error code to be converted to string
//
// Returns:
//  A descriptive string representation of the error code
//
std::string ApiController::ErrorCodeToMessage( VmbErrorType eErr ) const
{
  return AVT::VmbAPI::Examples::ErrorCodeToMessage( eErr );
}

//
// Starts the Vimba API and loads all transport layers
//
// Returns:
//  An API status code
//
VmbErrorType ApiController::StartUp()
{
  VmbErrorType res;

  // Start Vimba
  res = m_system.Startup();
  if( VmbErrorSuccess == res )
  {
    // This will be wrapped in a shared_ptr so we don't delete it
    SP_SET( m_pCameraObserver , new CameraObserver() );
    // Register an observer whose callback routine gets triggered whenever a camera is plugged in or out
    res = m_system.RegisterCameraListObserver( m_pCameraObserver );
  }
  return res;
}

//
// Shuts down the API
//
void ApiController::ShutDown()
{
  VmbErrorType res;
  // Release Vimba
  res = m_system.UnregisterCameraListObserver(m_pCameraObserver);
  if (VmbErrorSuccess != res)
    std::cerr << "UnregisterCameraListObserver failed\n";
  m_system.Shutdown();
}



//
// Gets all cameras known to Vimba
//
// Returns:
//  A vector of camera shared pointers
//
CameraPtrVector ApiController::GetCameraList()
{
  CameraPtrVector cameras;
  // Get all known cameras
  if( VmbErrorSuccess == m_system.GetCameras( cameras ) ) {
    // And return them
    return cameras;
  }
  return CameraPtrVector();
}


VmbErrorType ApiController::InitializeCameras() {
  
  AVT::VmbAPI::CameraPtrVector cameras_ptr = GetCameraList();

  for (auto cam_ptr : cameras_ptr) {
    std::string str_cam_id = "";
    if (VmbErrorSuccess == cam_ptr->GetID(str_cam_id)) {
      std::cout << "str_cam_id: " << str_cam_id;
      AVTCamera avt_cam(&m_system, str_cam_id);
      m_pCameras.push_back(avt_cam);
    }
  }
}



//
// Returns the camera observer as QObject pointer to connect their signals to the view's slots
//
CameraObserver* ApiController::GetCameraObserver()
{
  return SP_DYN_CAST( m_pCameraObserver, CameraObserver ).get();
}


//
// Gets the version of the Vimba API
//
// Returns:
//  The version as string
//
std::string ApiController::GetVersion() const
{
  std::ostringstream os;
  os << m_system;
  return os.str();
}

}} // namespace AVT::VmbAPI
