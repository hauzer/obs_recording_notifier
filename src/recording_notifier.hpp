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

#ifndef RECORDING_NOTIFIER_HPP
#define RECORDING_NOTIFIER_HPP

extern "C" __declspec(dllexport) void ConfigPlugin(HWND);

extern "C" __declspec(dllexport) bool LoadPlugin();
extern "C" __declspec(dllexport) void UnloadPlugin();
extern "C" __declspec(dllexport) CTSTR GetPluginName();
extern "C" __declspec(dllexport) CTSTR GetPluginDescription();

extern "C" __declspec(dllexport) void OnStartStream();
extern "C" __declspec(dllexport) void OnStopStream();

#endif // RECORDING_NOTIFIER_HPP
