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
#include <memory>

#include <OBSApi.h>
#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <sapi.h>

#include "recording_notifier.hpp"


#define CHECK_HR(expr) \
    hr = expr;  \
    if(FAILED(hr)) { \
        goto hr_failure;   \
    }


class Volume
{
private:
    ISimpleAudioVolume* volume;

public:
    Volume(ISimpleAudioVolume* volume) :
        volume(volume)
    {
    }

    ~Volume()
    {
        if(volume == nullptr) return;

        volume->Release();
    }

    bool is_muted()
    {
        if(volume == nullptr) false;

        BOOL _is_muted;
        volume->GetMute(&_is_muted);
        return _is_muted;
    }

    void mute()
    {
        if(volume == nullptr) return;

        volume->SetMute(TRUE, NULL);
    }

    void unmute()
    {
        if(volume == nullptr) return;

        volume->SetMute(FALSE, NULL);
    }

    void toggle_muted()
    {
        if(volume == nullptr) return;

        if(is_muted())
            unmute();
        else
            mute();
    }
};


void speak(const std::wstring&);
std::unique_ptr<Volume> get_scene_volume();
BOOL CALLBACK get_scene_audio_volume_EnumWindows_callback(HWND, LPARAM);


ISpVoice* voice;


/*void ConfigPlugin(HWND)
{
}*/

bool LoadPlugin()
{
    HRESULT hr;

    CHECK_HR(CoInitialize(NULL));
    CHECK_HR(CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (LPVOID*)&voice));
    
    return true;

hr_failure:
    return false;
}

void UnloadPlugin()
{
    SafeRelease(voice);
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

    auto message = OBSGetSceneName() + std::wstring(L" <rate speed=\"-5\"><emph>is now</emph></rate> recording.");
    speak(message);
}

void OnStopStream()
{
    if(!OBSGetRecording()) return;

    auto message = OBSGetSceneName() + std::wstring(L" <rate speed=\"-5\"><emph>has stopped</emph></rate> recording.");
    speak(message);
}

BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}


void speak(const std::wstring& message)
{
    auto scene_volume = get_scene_volume();
    auto desktop_audio = OBSGetDesktopAudioSource();

    scene_volume->mute();
    desktop_audio->StopCapture();

    voice->Speak(message.data(), SPF_ASYNC | SPF_IS_XML, NULL);
    voice->WaitUntilDone(INFINITE);

    desktop_audio->StartCapture();
    scene_volume->unmute();
}

std::unique_ptr<Volume> get_scene_volume()
{
    HWND hwnd = NULL;
    DWORD procid;
    EnumWindows(get_scene_audio_volume_EnumWindows_callback, reinterpret_cast<LPARAM>(&hwnd));
    GetWindowThreadProcessId(hwnd, &procid);

    HRESULT hr;
    IMMDeviceEnumerator *device_enumerator = NULL;
    IMMDevice *audio_device = NULL;
    IAudioSessionManager* audio_sessions_manager = NULL;
    IAudioSessionManager2* audio_sessions_manager2 = NULL;
    IAudioSessionEnumerator* audio_sessions = NULL;
    IAudioSessionControl* audio_session_control = NULL;
    IAudioSessionControl2* audio_session_control2 = NULL;
    ISimpleAudioVolume* audio_volume = NULL;

    CHECK_HR(CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&device_enumerator)));
    CHECK_HR(device_enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &audio_device));

    CHECK_HR(audio_device->Activate(__uuidof(IAudioSessionManager), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&audio_sessions_manager)));
    CHECK_HR(audio_device->Activate(__uuidof(IAudioSessionManager2), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&audio_sessions_manager2)));

    int audio_sessions_num;
    CHECK_HR(audio_sessions_manager2->GetSessionEnumerator(&audio_sessions));
    CHECK_HR(audio_sessions->GetCount(&audio_sessions_num));
    for(int i = 0; i < audio_sessions_num; ++i) {
        CHECK_HR(audio_sessions->GetSession(i, &audio_session_control));
        CHECK_HR(audio_session_control->QueryInterface(__uuidof(IAudioSessionControl2), reinterpret_cast<void**>(&audio_session_control2)));

        DWORD audio_session_procid;
        audio_session_control2->GetProcessId(&audio_session_procid);

        if(audio_session_procid == procid) {
            CHECK_HR(audio_session_control->QueryInterface(__uuidof(ISimpleAudioVolume), reinterpret_cast<void**>(&audio_volume)));
            break;
        }
    }

hr_failure:
    SafeRelease(device_enumerator);
    SafeRelease(audio_device);
    SafeRelease(audio_sessions_manager);
    SafeRelease(audio_sessions_manager2);
    SafeRelease(audio_sessions);
    SafeRelease(audio_session_control);
    SafeRelease(audio_session_control2);

    return std::unique_ptr<Volume>(new Volume(audio_volume));
}

BOOL CALLBACK get_scene_audio_volume_EnumWindows_callback(HWND hwnd, LPARAM param)
{
    auto title_length = GetWindowTextLength(hwnd) + 1;
    wchar_t* title_raw = new wchar_t[title_length];
    GetWindowText(hwnd, title_raw, title_length);
    std::wstring title(title_raw);
    delete title_raw;

    if(title.find(OBSGetSceneName()) != std::string::npos) {
        HWND* out_hwnd = reinterpret_cast<HWND*>(param);
        *out_hwnd = hwnd;
        return FALSE;
    }

    return TRUE;
}
