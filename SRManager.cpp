// Copyright (c) 2003 NPD Group. All rights reserved.
// $Id: $

//-----------------------------------------------------------------------
// includes

// includes - system

// includes - project
#include "stdafx.h"

#include "SRManager.h"

//-----------------------------------------------------------------------
// globals and externs

//-----------------------------------------------------------------------
// ASRManager class
ASRManager::ASRManager() :
   fActive(false)
{
}

ASRManager::~ASRManager()
{
   this->Cleanup();
}

bool ASRManager::Initialize(HWND hWnd)
{
   this->Cleanup();

   HRESULT hr = fEngine.CoCreateInstance(CLSID_SpSharedRecognizer);
   if (SUCCEEDED(hr))   // Create the recognition context
      hr = fEngine->CreateRecoContext(&fContext);
   if (SUCCEEDED(hr))   // Register for notifications
      hr = fContext->SetNotifyWindowMessage(hWnd, rCmdVoice, 0, 0);
   if (SUCCEEDED(hr))   // Tell SR what types of events interest us.  Here we only care about command recognition.
      hr = fContext->SetInterest(SPFEI(SPEI_RECOGNITION), SPFEI(SPEI_RECOGNITION));

   // use the language from the engine
   SPRECOGNIZERSTATUS stat;
   ::memset(&stat, 0, sizeof(stat));
   if (SUCCEEDED(hr = fEngine->GetStatus(&stat)))
   {  // Create the grammar for command mode
      LANGID language = stat.aLangID[0];
      hr = fContext->CreateGrammar(0, &fGrammar);
      if (SUCCEEDED(hr))
         hr = fGrammar->LoadCmdFromResource(::AfxGetResourceHandle(),
          (const WCHAR*)MAKEINTRESOURCE(rGrammar), L"SRGRAMMAR", language, SPLO_STATIC);

      if (FAILED(hr))
      {  // report failure to load grammar
         if ((SPERR_UNSUPPORTED_LANG == hr) || (ERROR_RESOURCE_LANG_NOT_FOUND == (0xffff & hr)))
            TRACE(_T("Unsupported language\n"));
         else
            TRACE(_T("Error loading the grammar\n"));
      }
   }
   
   return SUCCEEDED(hr);
}

void ASRManager::Cleanup()
{
   if (fGrammar != NULL)
      fGrammar.Release();
   if (fContext != NULL)
      fContext.Release();
   if (fEngine != NULL)
      fEngine.Release();
}

BOOL ASRManager::SupportsUserTraining() const
{
   if (fEngine != NULL)
   {
      BOOL supported = false;
      if (SUCCEEDED(fEngine->IsUISupported(SPDUI_UserTraining, NULL, 0, &supported)))
         return supported;
      TRACE(_T("IsUISupported(SPDUI_UserTraining) failed\n"));
   }
   return false;
}

BOOL ASRManager::SupportsMicTraining() const
{
   if (fEngine != NULL)
   {
      BOOL supported = false;
      if (SUCCEEDED(fEngine->IsUISupported(SPDUI_MicTraining, NULL, 0, &supported)))
         return supported;
      TRACE(_T("IsUISupported(SPDUI_MicTraining) failed\n"));
   }
   return false;
}

void ASRManager::SetIsActive(bool active)
{
   if (fGrammar != NULL)
   {
      fActive = active;
      fGrammar->SetRuleState(NULL, NULL, active ? SPRS_ACTIVE : SPRS_INACTIVE);
   }
}

