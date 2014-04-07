// An OBS (Open Broadcasting Software) plugin that let's you know when you start or stop recording.
// Copyright (C) 2014  Никола "hauzer" Вукосављевић

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <sstream>

#include <OBSApi.h>
#include <sapi.h>

#include "recording_notifier.hpp"

ISpVoice* voice;

void ConfigPlugin(HWND)
{
}

bool LoadPlugin()
{
    if(FAILED(CoInitialize(NULL)))
        return false;

    HRESULT hr = CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (void**)&voice);
    if(FAILED(hr))
        return false;
    
    return true;
}

void UnloadPlugin()
{
    voice->Release();
    voice = NULL;
    CoUninitialize();
}

CTSTR GetPluginName()
{
    return TEXT("Recording Notifier");
}

CTSTR GetPluginDescription()
{
    return TEXT("Let's you know when you start or stop recording.");
}

void OnStartStream()
{
    if(!OBSGetRecording()) return;

    std::wostringstream ss;
    ss << OBSGetSceneName() << L" is now recording.";
    voice->Speak(ss.str().c_str(), SPF_ASYNC | SPF_IS_NOT_XML, NULL);
    
    auto desktop_audio = OBSGetDesktopAudioSource();
    desktop_audio->StopCapture();
    voice->WaitUntilDone(INFINITE);
    desktop_audio->StartCapture();
}

void OnStopStream()
{
    if(!OBSGetRecording()) return;
    
    std::wostringstream ss;
    ss << OBSGetSceneName() << L" has stopped recording.";
    voice->Speak(ss.str().c_str(), SPF_ASYNC | SPF_IS_NOT_XML, NULL);
}

BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}
