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

#include <exception>
#include <memory>
#include <boost/algorithm/string.hpp>
#include <boost/exception/all.hpp>
#include <boost/format.hpp>

#include <Audioclient.h>
#include <audiopolicy.h>
#include <Mmdeviceapi.h>
#include <sapi.h>
// See http://stackoverflow.com/q/20031597/2006222
#pragma warning(disable: 4996)
#include <sphelper.h>

// See https://obsproject.com/forum/threads/obsapi-and-windows-headers.13327/
#ifdef WINVER
#define WINVER_REAL WINVER
#undef WINVER
#endif
#ifdef _WIN32_WINNT
#define WIN32_WINNT_REAL _WIN32_WINNT
#undef _WIN32_WINNT
#endif
#ifdef NTDDI_VERSION
#define NTDDI_VERSION_REAL NTDDI_VERSION
#undef NTDDI_VERSION
#endif
#ifndef _INC_COMMCTRL
#define _INC_COMMCTRL
#define INC_COMMCTRL_DO_UNDEF
#endif
#include <OBSApi.h>
#ifdef INC_COMMCTRL_DO_UNDEF
#undef _INC_COMMCTRL
#undef INC_COMMCTRL_DO_UNDEF
#endif
#ifdef NTDDI_VERSION_REAL
#undef NTDDI_VERSION
#define NTDDI_VERSION NTDDI_VERSION_REAL
#undef NTDDI_VERSION_REAL
#endif
#ifdef WIN32_WINNT_REAL
#undef _WIN32_WINNT
#define _WIN32_WINNT WIN32_WINNT_REAL
#undef WIN32_WINNT_REAL
#endif
#ifdef WINVER_REAL
#undef WINVER
#define WINVER WINVER_REAL
#undef WINVER_REAL
#endif

#include "recording_notifier.hpp"


#define INIT_CHECKS()   \
    bool _check_failed = false;

#define _CHECK_HR(hr)   \
    if(FAILED(hr)) {    \
        _check_failed = true;   \
        goto check_failure;   \
    }

#define CHECK_HR(expr)  \
    _CHECK_HR(expr);

#define CHECK_ERROR(expr) \
    SetLastError(ERROR_SUCCESS);    \
    expr;   \
    _CHECK_HR(HRESULT_FROM_WIN32(GetLastError()));

#define CHECK_FAILURE(exprs)    \
check_failure:  \
    exprs   \
    if(_check_failed)   \
        BOOST_THROW_EXCEPTION(check_error());
    
#define SAFE_RELEASE(obj)   \
    if(obj != NULL) {   \
        obj->Release(); \
        obj = NULL; \
    }


struct base_error : virtual std::exception, virtual boost::exception { };
struct nullptr_error : virtual base_error { };
struct check_error : virtual base_error { };


class Voice
{
private:
    ISpVoice* voice;

public:
    Voice()
    {
        INIT_CHECKS();

        CSpStreamFormat format;
        format.AssignFormat(SPSF_48kHz16BitStereo);

        ISpAudio* audio;
        CHECK_HR(SpCreateDefaultObjectFromCategoryId(SPCAT_AUDIOOUT, &audio));
        audio->SetFormat(format.FormatId(), format.WaveFormatExPtr());

        CHECK_HR(CoCreateInstance(CLSID_SpVoice, NULL, CLSCTX_ALL, IID_ISpVoice, (LPVOID*)&voice));
        CHECK_HR(voice->SetVolume(100));
        CHECK_HR(voice->SetRate(-2));
        CHECK_HR(voice->SetOutput(audio, FALSE));
        return;

    CHECK_FAILURE(
            SAFE_RELEASE(audio);
        );
    }

    ~Voice()
    {
        voice->Release();
    }

    void speak(const std::wstring& message)
    {
        INIT_CHECKS();

        CHECK_HR(voice->Speak(message.data(), SPF_IS_XML, NULL));
        return;

    CHECK_FAILURE();
    }
};


class Audio
{
private:
    ISimpleAudioVolume* audio;

public:
    Audio(ISimpleAudioVolume* audio)
    {
        if(audio != nullptr)
            this->audio = audio;
        else
            BOOST_THROW_EXCEPTION(nullptr_error());
    }

    ~Audio()
    {
        audio->Release();
    }

    bool is_muted()
    {
        INIT_CHECKS();

        BOOL _is_muted;
        CHECK_HR(audio->GetMute(&_is_muted));
        return _is_muted;

    CHECK_FAILURE();
    }

    void mute()
    {
        INIT_CHECKS();

        CHECK_HR(audio->SetMute(TRUE, NULL));
        return;

    CHECK_FAILURE();
    }

    void unmute()
    {
        INIT_CHECKS();

        CHECK_HR(audio->SetMute(FALSE, NULL));
        return;

    CHECK_FAILURE();
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
std::unique_ptr<Audio> get_obs_scene_audio();


/*void ConfigPlugin(HWND)
{
}*/

bool LoadPlugin()
{
    INIT_CHECKS();

    CHECK_HR(CoInitialize(NULL));
    return true;

CHECK_FAILURE(
    return false;
    );
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

    speak((boost::wformat(L"%1% is now recording.") % OBSGetSceneName()).str());
}

void OnStopStream()
{
    if(!OBSGetRecording()) return;

    speak((boost::wformat(L"%1% has stopped recording.") % OBSGetSceneName()).str());
}

BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}


void speak(const std::wstring& message)
{
    std::unique_ptr<Audio> scene_audio;
    try {
        scene_audio = get_obs_scene_audio();
    } catch(base_error) {
        scene_audio = nullptr;
    }

    auto desktop_audio = OBSGetDesktopAudioSource();

    if(scene_audio != nullptr) scene_audio->mute();
    desktop_audio->StopCapture();

    Voice voice;
    voice.speak(message);

    desktop_audio->StartCapture();
    if(scene_audio != nullptr) scene_audio->unmute();
}

std::unique_ptr<Audio> get_obs_scene_audio()
{
    struct EnumWindows_callback {
        static BOOL CALLBACK f(HWND hwnd, LPARAM param)
        {
            INIT_CHECKS();

            int title_length;
            std::unique_ptr<wchar_t> title_raw;
            std::wstring title;
            std::wstring scene_name;

            CHECK_ERROR(title_length = GetWindowTextLength(hwnd) + 1);

            title_raw = std::unique_ptr<wchar_t>(new wchar_t[title_length]);
            CHECK_ERROR(GetWindowText(hwnd, title_raw.get(), title_length));

            title = title_raw.get();
            scene_name = OBSGetSceneName();

            boost::algorithm::to_lower(title);
            boost::algorithm::to_lower(scene_name);
            if((title.find(L"open broadcaster software") == std::string::npos) &&
                (title.find(scene_name) != std::string::npos)) {
                HWND* out_hwnd = reinterpret_cast<HWND*>(param);
                *out_hwnd = hwnd;
                return FALSE;
            }

        CHECK_FAILURE(
            return TRUE;
            );
        }
    };

    INIT_CHECKS();

    IMMDeviceEnumerator *device_enumerator = NULL;
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

CHECK_FAILURE(
    SAFE_RELEASE(device_enumerator);
    SAFE_RELEASE(audio_device);
    SAFE_RELEASE(audio_sessions_manager);
    SAFE_RELEASE(audio_sessions_manager2);
    SAFE_RELEASE(audio_sessions);
    SAFE_RELEASE(audio_session_control);
    SAFE_RELEASE(audio_session_control2);
    );

    return std::unique_ptr<Audio>(new Audio(audio_volume));
}
