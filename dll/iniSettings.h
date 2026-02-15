#pragma once

#define INI_FILE "FloatingDamage.ini"

#include <windows.h>
#include <unordered_map>

class INIFile
{
public:
	std::string Name;
	std::string SingleDamageFront;
	std::string SingleDamageBack;
	std::string TimeDamageFront;
	std::string TimeDamageBack;
	std::string BlockString;
	std::string SneakString;
	std::string CriticalString;
	std::string HealthHealFront;
	std::string HealthHealBack;
	std::string MagickaDamageFront;
	std::string MagickaDamageBack;
	std::string MagickaHealFront;
	std::string MagickaHealBack;
	std::string StaminaDamageFront;
	std::string StaminaDamageBack;
	std::string StaminaHealFront;
	std::string StaminaHealBack;

	bool EnablePlayer;
	bool HideRegen;
	bool HideNoLOS;
	bool OnlyHostile;
	UInt32 Size;
	UInt32 Alpha;
	UInt32 BlockSize;
	UInt32 SneakSize;
	UInt32 CriticalSize;
	UInt32 HostileSingleDamage;
	UInt32 HostileTimeDamage;
	UInt32 FollowerSingleDamage;
	UInt32 FollowerTimeDamage;
	UInt32 Block;
	UInt32 Sneak;
	UInt32 Critical;
	UInt32 HealthHeal;
	UInt32 MagickaDamage;
	UInt32 MagickaHeal;
	UInt32 StaminaDamage;
	UInt32 StaminaHeal;
	float PositionX;
	float PositionY;
	UInt32 DamageDisplayMode;
	UInt32 HealDisplayMode;
	double DisplayDispersion;
	UInt32 TimeDamageDifference;
	UInt32 TimeDamageInterval;
	UInt32 MinHeal;
	UInt32 EnableBlock;
	UInt32 EnableSneak;
	UInt32 EnableCritical;
	UInt32 EnableHealth;
	UInt32 EnableMagicka;
	UInt32 EnableStamina;
	double NearRadius;
	double FarRadius;
	UInt32 ListUpdateTime;

	INIFile();
	void Load();

private:
	std::string GetSkyrimPath();
	std::string GetSksePluginPath();
	bool IsFoundFile(const char* fileName);
	std::vector<std::string> GetSectionKeys(LPCTSTR section_name, LPCTSTR ini_file_path);
	void ToLower(std::string &str);
	std::vector<std::string> Split(const std::string &str, char sep);

	void SetSettings();
	std::string GetINIlFile();
	void SetINIData(std::vector<std::string> *list);
	void GetSettings();
	void ShowSettings();

	std::unordered_map<std::string, std::string> stringsMap;
	std::unordered_map<std::string, UInt32> settingsMap;
	std::string iniFilePath;
};

extern INIFile ini;