/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <https://www.gnu.org/licenses/old-licenses/gpl-2.0>.
 */

/** @file settings_func.h Functions related to setting/changing the settings. */

#ifndef SETTINGS_FUNC_H
#define SETTINGS_FUNC_H

#include "company_type.h"
#include "string_type.h"
#include "newgrf_config.h"

struct IniFile;
struct WindowDesc;

void IConsoleSetSetting(std::string_view name, std::string_view value, bool force_newgame = false);
void IConsoleSetSetting(std::string_view name, int32_t value);
void IConsoleGetSetting(std::string_view name, bool force_newgame = false);
void IConsoleListSettings(std::string_view prefilter);

/**
 * Reusable core for console-style setting get (used by console and RPC surfaces).
 * @param name Setting name as used by the in-game console.
 * @param out_value Current value formatted as a string.
 * @param out_error Human-readable error if operation fails.
 * @param force_newgame When true, operate on _settings_newgame regardless of game mode.
 * @return true on success, false on failure (out_error set).
 */
bool TryGetSettingValue(std::string_view name, std::string &out_value, std::string &out_error, bool force_newgame = false);

/**
 * Reusable core for console-style setting set (used by console and RPC surfaces).
 * @param name Setting name as used by the in-game console.
 * @param value Value string as accepted by the console.
 * @param out_value Resulting value formatted as a string (after applying).
 * @param out_error Human-readable error if operation fails.
 * @param force_newgame When true, operate on _settings_newgame regardless of game mode.
 * @return true on success, false on failure (out_error set).
 */
bool TrySetSettingValue(std::string_view name, std::string_view value, std::string &out_value, std::string &out_error, bool force_newgame = false);

void LoadFromConfig(bool minimal = false);
void SaveToConfig();

void IniLoadWindowSettings(IniFile &ini, std::string_view grpname, WindowDesc *desc);
void IniSaveWindowSettings(IniFile &ini, std::string_view grpname, WindowDesc *desc);

StringList GetGRFPresetList();
GRFConfigList LoadGRFPresetFromConfig(std::string_view config_name);
void SaveGRFPresetToConfig(std::string_view config_name, GRFConfigList &config);
void DeleteGRFPresetFromConfig(std::string_view config_name);

void SetDefaultCompanySettings(CompanyID cid);

void SyncCompanySettings();

#endif /* SETTINGS_FUNC_H */
