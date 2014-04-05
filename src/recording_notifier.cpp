// A simple OBS (Open Broadcasting Software) plugin that plays sounds upon starting and stopping of a recording.
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

#include <OBSApi.h>
#include <Mmsystem.h>

#include "recording_notifier.hpp"

void ConfigPlugin(HWND)
{
}

bool LoadPlugin()
{
    return true;
}

void UnloadPlugin()
{
}

CTSTR GetPluginName()
{
    return TEXT("Recording notifier");
}

CTSTR GetPluginDescription()
{
    return TEXT("Recording notifier");
}

void OnStartStream()
{
    PlaySound("SystemQuestion", NULL, SND_ALIAS);
}

void OnStopStream()
{
    PlaySound("SystemHand", NULL, SND_ALIAS);
}

BOOL CALLBACK DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
    return TRUE;
}
