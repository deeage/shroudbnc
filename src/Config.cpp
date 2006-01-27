/*******************************************************************************
 * shroudBNC - an object-oriented framework for IRC                            *
 * Copyright (C) 2005 Gunnar Beutner                                           *
 *                                                                             *
 * This program is free software; you can redistribute it and/or               *
 * modify it under the terms of the GNU General Public License                 *
 * as published by the Free Software Foundation; either version 2              *
 * of the License, or (at your option) any later version.                      *
 *                                                                             *
 * This program is distributed in the hope that it will be useful,             *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the               *
 * GNU General Public License for more details.                                *
 *                                                                             *
 * You should have received a copy of the GNU General Public License           *
 * along with this program; if not, write to the Free Software                 *
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA. *
 *******************************************************************************/

#include "StdAfx.h"

/**
 * CConfig
 *
 * Constructs a new configuration object and loads the given filename
 * if specified. If you specify NULL as the filename, a volatile
 * configuration object is constructed. Changes made to such an object
 * are not stored to disk.
 *
 * @param Filename the filename of the configuration file, can be NULL
 */
CConfig::CConfig(const char *Filename) {
	m_WriteLock = false;
	m_Settings = NULL;

	if (Filename != NULL) {
		m_Filename = strdup(Filename);
	} else {
		m_Filename = NULL;
	}

	Reload();
}

/**
 * ParseConfig
 *
 * Parses a configuration file. Valid lines of the configuration file
 * have this syntax:
 *
 * setting=value
 */
bool CConfig::ParseConfig(void) {
	char Line[4096];
	char *dupEq;

	if (m_Filename == NULL) {
		return false;
	}

	FILE *ConfigFile = fopen(m_Filename, "r");

	if (ConfigFile == NULL) {
		return false;
	}

	m_WriteLock = true;

	while (!feof(ConfigFile)) {
		fgets(Line, sizeof(Line), ConfigFile);

		if (Line[strlen(Line) - 1] == '\n')
			Line[strlen(Line) - 1] = '\0';

		if (Line[strlen(Line) - 1] == '\r')
			Line[strlen(Line) - 1] = '\0';

		char* Eq = strstr(Line, "=");

		if (Eq) {
			*Eq = '\0';

			dupEq = strdup(++Eq);

			if (dupEq == NULL) {
				if (g_Bouncer != NULL) {
					LOGERROR("strdup() failed. Config option lost (%s=%s).",
						Line, Eq);

					g_Bouncer->Fatal();
				} else {
					printf("CConfig::ParseConfig: strdup() failed."
						" Config could not be parsed.");

					exit(0);
				}

				continue;
			}

			if (m_Settings->Add(Line, dupEq) == false) {
				LOGERROR("CHashtable::Add failed. Config could not be parsed"
					" (%s, %s).", Line, Eq);

				g_Bouncer->Fatal();
			}
		}
	}

	fclose(ConfigFile);

	m_WriteLock = false;

	return true;
}

/**
 * ~CConfig
 *
 * Destructs the configuration object.
 */
CConfig::~CConfig() {
	free(m_Filename);

	if (m_Settings != NULL) {
		delete m_Settings;
	}
}

/**
 * ReadString
 *
 * Reads a configuration setting as a string. If the specified setting does
 * not exist, NULL is returned.
 *
 * @param Setting the configuration setting
 */
const char *CConfig::ReadString(const char *Setting) {
	char *Value = m_Settings->Get(Setting);

	if (Value != NULL && *Value != '\0') {
		return Value;
	} else {
		return NULL;
	}
}

/**
 * ReadInteger
 *
 * Reads a configuration setting as an integer. If the specified setting does
 * not exist, 0 is returned.
 *
 * @param Setting the configuration setting
 */
int CConfig::ReadInteger(const char *Setting) {
	const char *Value = m_Settings->Get(Setting);

	if (Value != NULL) {
		return atoi(Value);
	} else {
		return 0;
	}
}

/**
 * WriteString
 *
 * Set a configuration setting.
 *
 * @param Setting the configuration setting
 * @param Value the new value for the setting, can be NULL to indicate that
 *              the configuration setting is to be removed
 */
bool CConfig::WriteString(const char *Setting, const char *Value) {
	bool ReturnValue;

	if (Value != NULL) {
		ReturnValue = m_Settings->Add(Setting, strdup(Value));
	} else {
		ReturnValue = m_Settings->Remove(Setting);
	}

	if (ReturnValue == false) {
		return false;
	}

	if (m_WriteLock == false && Persist() == false) {
		return false;
	} else {
		return true;
	}
}

/**
 * WriteInteger
 *
 * Sets a configuration setting.
 *
 * @param Setting the configuration setting
 * @param Value the new value for the setting
 */
bool CConfig::WriteInteger(const char *Setting, const int Value) {
	char ValueStr[50];

	snprintf(ValueStr, sizeof(ValueStr), "%d", Value);

	return WriteString(Setting, ValueStr);
}

/**
 * Persist
 *
 * Saves changes which have been made to the configuration object to disk
 * unless the configuration object is volatile.
 */
bool CConfig::Persist(void) {
	if (m_Filename == NULL) {
		return false;
	}

	FILE *ConfigFile = fopen(m_Filename, "w");

	if (ConfigFile != NULL) {
		int i = 0;
		while (hash_t<char*>* SettingHash = m_Settings->Iterate(i++)) {
			if (SettingHash->Name != NULL && SettingHash->Value != NULL) {
				fprintf(ConfigFile, "%s=%s\n", SettingHash->Name, SettingHash->Value);
			}
		}

		fclose(ConfigFile);

		return true;
	} else {
		if (g_Bouncer != NULL) {
			LOGERROR("Config file %s could not be opened.", m_Filename);
		} else {
			printf("Config file %s could not be opened.", m_Filename);
		}

		return false;
	}
}

/**
 * GetFilename
 *
 * Returns the filename of the configuration object. The return value will
 * be NULL if the configuration object is volatile.
 */
const char *CConfig::GetFilename(void) {
	return m_Filename;
}

/**
 * Iterate
 *
 * Iterates through the configuration object's settings.
 *
 * @param Index specifies the index of the setting which is to be returned
 */
hash_t<char *> *CConfig::Iterate(int Index) {
	return m_Settings->Iterate(Index);
}

/**
 * Reload
 *
 * Reloads all settings from disk.
 */
void CConfig::Reload(void) {
	if (m_Settings != NULL) {
		delete m_Settings;
	}

	m_Settings = new CHashtable<char *, false, 8>();

	if (m_Settings == NULL && g_Bouncer != NULL) {
		LOGERROR("new operator failed.");

		g_Bouncer->Fatal();
	}

	m_Settings->RegisterValueDestructor(string_free);

	if (m_Filename != NULL) {
		ParseConfig();
	}
}

/**
 * Count
 *
 * Returns the number of items in the config.
 */
int CConfig::Count(void) {
	if (m_Settings == NULL) {
		return 0;
	} else {
		return m_Settings->Count();
	}
}