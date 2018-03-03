// Copyright © 2007 Magic Teleprompting, Inc.  All Rights Reserved.
// $Id: $

#include "stdafx.h"

#include "HandController.h"
#include "Msw.h"
#include <setupapi.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static LPCTSTR kId      = _T("vid_04d8&pid_fe9f");
static GUID    kUsbGuid = {0xA5DCBF10L, 0x6530, 0x11D2, {0x90, 0x1F, 0x00, 0xC0, 0x4F, 0xB9, 0x51, 0xED}};


void AHandController::Connect()
{
   this->Disconnect();

   HDEVINFO info = ::SetupDiGetClassDevs(&kUsbGuid, NULL, NULL, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
   if (INVALID_HANDLE_VALUE != info)
   {  // Retrieve a context structure for a device interface of a device information set.
      SP_DEVICE_INTERFACE_DATA iData;
      ZeroMemory(&iData, sizeof(SP_DEVICE_INTERFACE_DATA));
      iData.cbSize = sizeof(iData);
      for (int i = 0; !this->IsConnected(); ++i)
      {
         if (!::SetupDiEnumDeviceInterfaces(info, NULL, &kUsbGuid, i, &iData))
            break;

         // Get size of symbolic link name
         DWORD pathLen = 0;
         ::SetupDiGetDeviceInterfaceDetail(info, &iData, NULL, 0, &pathLen, NULL);
         PSP_INTERFACE_DEVICE_DETAIL_DATA ifDetail = (PSP_INTERFACE_DEVICE_DETAIL_DATA)(new char[pathLen]);
         ifDetail->cbSize = sizeof(SP_INTERFACE_DEVICE_DETAIL_DATA);
         if (::SetupDiGetDeviceInterfaceDetail(info, &iData, ifDetail, pathLen, NULL, NULL))
         {
            CString path(ifDetail->DevicePath);
            path.MakeLower();
            if (path.Find(kId))
            {
               HANDLE h = ::CreateFile(path, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
               if (INVALID_HANDLE_VALUE != h)
               {  // now try to initialize the controller
                  // give the controller a little time to start generating events
                  ::Sleep(250);
                  COMSTAT status;
                  DWORD errors = 0;
                  if (::ClearCommError(h, &errors, &status) && (status.cbInQue > 0))
                  {
                     DCB dcb;
                     if (::GetCommState(h, &dcb))
                     {
                        dcb.BaudRate = kBaudRate;
                        if (::SetCommState(h, &dcb) && ::SetupComm(h, kInQueueSize, kOutQueueSize))
                        {
                           fPortId = h;
                           TRACE("HC: connected!\n");
                        }
                     }
                  }

                  if (!this->IsConnected())
                     ::CloseHandle(h);
               }
            }
         }
         delete [] ifDetail;
      }
      ::SetupDiDestroyDeviceInfoList(info);
   }
   else
      TRACE("HC: SetupDiClassDevs() failed. GetLastError() returns: 0x%x\n", GetLastError());
}

void AHandController::Disconnect()
{
   if (INVALID_HANDLE_VALUE != fPortId)
   {
      ::FlushFileBuffers(fPortId);
      VERIFY(::CloseHandle(fPortId));
      fPortId = INVALID_HANDLE_VALUE;
      TRACE("HC: disconnected!\n");
   }

   this->Init();
}
   
bool AHandController::Poll(int& speed, bool& lClick, bool& rClick)
{  // return if preference is set to 'no controller'
   if (!this->IsConnected())
      return false;

   COMSTAT status;
   DWORD errors = 0;
   if (!::ClearCommError(fPortId, &errors, &status) || (status.cbInQue < 3))
      return false;

   DWORD bytesRead = 0;
   std::auto_ptr<BYTE> data = std::auto_ptr<BYTE>(new BYTE[status.cbInQue]);
   if (!::ReadFile(fPortId, data.get(), status.cbInQue, &bytesRead, NULL) || (bytesRead < 3))
      return false;

   size_t i = bytesRead - 3;
   while ((i > 0) && (data.get()[i] != kSyncChar))
      --i;
   if (data.get()[i] != kSyncChar)
      return false;

   // don't support press-and-hold. Require the button to come up
   // before we report another press
   const BYTE c = data.get()[i + 1];
   static bool lastL, lastR;
   const bool l = (c == '1') || (c == '0');
   lClick = (l && !lastL);
   lastL = l;

   const bool r = (c == '2') || (c == '0');
   rClick = (r && !lastR);
   lastR = r;

   // update the speed
#ifdef _DEBUG
static int x = 0;
if (0 == (x %20)) TRACE("raw h/c speed: %d\n", data.get()[i + 2]);
#endif // _DEBUG

   speed = (data.get()[i + 2] & 0xFE) - 128;

#ifdef _DEBUG
if (0 == (x++ %20)) TRACE("h/c speed: %d\n", speed);
#endif // _DEBUG

   return true;
}
