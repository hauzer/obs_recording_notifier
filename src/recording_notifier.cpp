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

#include <memory>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>

#include <OBSApi.h>
#include <Mmdeviceapi.h>
#include <Audioclient.h>
#include <audiopolicy.h>
#include <sapi.h>

#include "recording_notifier.hpp"


#define _CHECK_HR(hr)   \
    if(FAILED(hr)) {    \
        goto check_failure;   \
    }

#define CHECK_HR(expr)  \
    _CHECK_HR(expr);

#define CHECK_ERROR(expr) \
    SetLastError(ERROR_SUCCESS);    \
    expr;   \
    _CHECK_HR(HRESULT_FROM_WIN32(GetLastError()));

#define SAFE_RELEASE(obj)   \
    if(obj != NULL) {   \
        obj->Release(); \
        obj = NULL; \
    }


class Voice
{
private:
    ISpVoice* voice;

public:
    Voice()
    {
        CHECK_HR(CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (LPVOID*)&voice));
        this->voice = voice;

    check_failure:
        return;
    }

    ~Voice()
    {
        voice->Release();
    }

    void speak(const std::wstring& message)
    {
        CHECK_HR(voice->SetVolume(100));
        CHECK_HR(voice->SetRate(-2));
        CHECK_HR(voice->Speak(message.data(), SPF_IS_XML, NULL));

    check_failure:
        return;
    }
};


class Volume
{
private:
    ISimpleAudioVolume& volume;

public:
    Volume(ISimpleAudioVolume* volume) :
        volume(*volume)
    {
    }

    ~Volume()
    {
        volume.Release();
    }

    bool is_muted()
    {
        BOOL _is_muted;
        volume.GetMute(&_is_muted);
        return _is_muted;
    }

    void mute()
    {
        volume.SetMute(TRUE, NULL);
    }

    void unmute()
    {
        volume.SetMute(FALSE, NULL);
    }

    void toggle_muted()
    {
        if(is_muted())
            unmute();
        else
            mute();
    }
};


void speak(const std::wstring&);
void speak(const boost::wformat&);
std::unique_ptr<Volume> get_obs_scene_volume();
HRESULT get_default_audio_device(IMMDevice*&);


/*void ConfigPlugin(HWND)
{
}*/

bool LoadPlugin()
{
    CHECK_HR(CoInitialize(NULL));
    
    return true;

check_failure:
    return false;
}

void UnloadPlugin()
{
    CoUninitialize();
}

CTSTR GetPluginName()
{
    return L"Recording Notifier";
}

CTSTR GetPluginDescription()
{
    return L"Let's you know when you start or stop recording.";
}

void OnStartStream()
{
    if(!OBSGetRecording()) return;

    speak((boost::wformat(L"%1% is now recording.") % OBSGetSceneName()));
}

void OnStopStream()
{
    if(!OBSGetRecording()) return;

    speak((boost::wformat(L"%1% has stopped recording.") % OBSGetSceneName()));
}

BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}


void speak(const std::wstring& message)
{
    /*
    IMMDevice* audio_device = NULL;
    IAudioClient* audio_session = NULL;
    WAVEFORMATEX format;
    */
    Voice voice;
    std::unique_ptr<Volume> scene_volume;
    AudioSource* desktop_audio;
    
    /*
    CHECK_HR(get_default_audio_device(audio_device));
    CHECK_HR(audio_device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, NULL, reinterpret_cast<void**>(&audio_session)));

    format.wFormatTag = WAVE_FORMAT_PCM;
    format.nChannels = 2;
    format.nSamplesPerSec = 44.1;
    format.wBitsPerSample = 16;
    format.nBlockAlign = format.nChannels * format.wBitsPerSample;
    format.nAvgBytesPerSec = format.nSamplesPerSec * format.nBlockAlign;
    format.cbSize = sizeof(format);
    CHECK_HR(audio_session->Initialize(AUDCLNT_SHAREMODE_SHARED, AUDCLNT_STREAMFLAGS_NOPERSIST | AUDCLNT_SESSIONFLAGS_DISPLAY_HIDE, 0, 0, &format, NULL));
    CHECK_HR(audio_session->Start());
    */

    scene_volume = get_obs_scene_volume();
    desktop_audio = OBSGetDesktopAudioSource();

    if(scene_volume != nullptr) scene_volume->mute();
    desktop_audio->StopCapture();

    /* CHECK_HR(voice->SetOutput(NULL, TRUE)) */
    voice.speak(message);

    desktop_audio->StartCapture();
    if(scene_volume != nullptr) scene_volume->unmute();

check_failure: ;
    /*
    SAFE_RELEASE(audio_device);
    SAFE_RELEASE(audio_session)
    */
}

void speak(const boost::wformat& message)
{
    speak(message.str());
}

std::unique_ptr<Volume> get_obs_scene_volume()
{
    struct EnumWindows_callback {
        static BOOL CALLBACK f(HWND hwnd, LPARAM param)
        {
            int title_length;
            wchar_t* title_raw = nullptr;
            std::wstring title;
            std::wstring scene_name;

            CHECK_ERROR(title_length = GetWindowTextLength(hwnd) + 1);

            title_raw = new wchar_t[title_length];
            CHECK_ERROR(GetWindowText(hwnd, title_raw, title_length));

            title = title_raw; delete title_raw; title_raw = nullptr;
            scene_name = OBSGetSceneName();

            boost::algorithm::to_lower(title);
            boost::algorithm::to_lower(scene_name);
            if(title.find(scene_name) != std::string::npos) {
                HWND* out_hwnd = reinterpret_cast<HWND*>(param);
                *out_hwnd = hwnd;
                return FALSE;
            }

        check_failure:
            delete title_raw;
            return TRUE;
        }
    };

    IMMDevice *audio_device = NULL;
    IAudioSessionManager* audio_sessions_manager = NULL;
    IAudioSessionManager2* audio_sessions_manager2 = NULL;
    IAudioSessionEnumerator* audio_sessions = NULL;
    IAudioSessionControl* audio_session_control = NULL;
    IAudioSessionControl2* audio_session_control2 = NULL;
    ISimpleAudioVolume* audio_volume = NULL;

    HWND hwnd = NULL;
    CHECK_ERROR(EnumWindows(EnumWindows_callback::f, reinterpret_cast<LPARAM>(&hwnd)));

    DWORD procid;
    CHECK_ERROR(GetWindowThreadProcessId(hwnd, &procid));

    CHECK_HR(get_default_audio_device(audio_device));
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

check_failure:
    SAFE_RELEASE(audio_device);
    SAFE_RELEASE(audio_sessions_manager);
    SAFE_RELEASE(audio_sessions_manager2);
    SAFE_RELEASE(audio_sessions);
    SAFE_RELEASE(audio_session_control);
    SAFE_RELEASE(audio_session_control2);

    if(audio_volume == NULL)
        return nullptr;
    else
        return std::unique_ptr<Volume>(new Volume(audio_volume));
}

HRESULT get_default_audio_device(IMMDevice*& audio_device)
{
    HRESULT hr;
    IMMDeviceEnumerator *device_enumerator = NULL;

    CHECK_HR(hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&device_enumerator)));
    CHECK_HR(hr = device_enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &audio_device));

check_failure:
    SAFE_RELEASE(device_enumerator);

    return hr;
}
