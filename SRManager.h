// Copyright (c) 2003 NPD Group, Inc. All rights reserved.
// $Id: $

#ifndef h_SRManager
#define h_SRManager

//-----------------------------------------------------------------------
// includes

// includes - system

// includes - project
#include "msw.h"

//-----------------------------------------------------------------------
// classes
/** <doc>
  */
class ASRManager
{
public:
   // enums ----------------------
   enum {rCmdVoice = WM_USER + 1};

   enum {
      Command = 1,
         rCmdScroll  = 100,
         rCmdPause   = 101,
         rCmdStop    = 102,
         rCmdEdit    = 103,
         rCmdFast    = 104,
         rCmdFaster  = 105,
         rCmdSlow    = 106,
         rCmdSlower  = 107,
   };


   // methods --------------------
   ASRManager();
   virtual ~ASRManager();

   bool Initialize(HWND hWnd);
   void Cleanup();

   bool IsAvailable() const;
   bool GetIsActive() const;
   void SetIsActive(bool active);

   BOOL SupportsUserTraining() const;
   BOOL SupportsMicTraining() const;

   void TrainUser(HWND hWnd);
   void TrainMic(HWND hWnd);

   bool GetEvent(CSpEvent& event);

private: // methods
   /// don't allow copying
   ASRManager(const ASRManager& ref);
   const ASRManager& operator = (const ASRManager& ref);

private: // data
   bool fActive;
   CComPtr<ISpRecognizer> fEngine;
   CComPtr<ISpRecoContext> fContext;
   CComPtr<ISpRecoGrammar> fGrammar;
}; // ASRManager

//-----------------------------------------------------------------------
// inline functions
inline bool ASRManager::IsAvailable() const
{
   return (fEngine != NULL);
}

inline bool ASRManager::GetIsActive() const
{
   return fActive;
}

inline void ASRManager::TrainUser(HWND hWnd)
{
   ASSERT(fEngine != NULL);
   fEngine->DisplayUI(hWnd, NULL, SPDUI_UserTraining, NULL, 0);
}

inline void ASRManager::TrainMic(HWND hWnd)
{
   ASSERT(fEngine != NULL);
   fEngine->DisplayUI(hWnd, NULL, SPDUI_MicTraining, NULL, 0);
}

inline bool ASRManager::GetEvent(CSpEvent& event)
{
   return (S_OK == event.GetFrom(fContext));
}


#endif // h_SRManager
