#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <windows.h>

#define INI_FILE "FloatingDamage.ini"

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
	std::uint32_t Size;
	std::uint32_t Alpha;
	std::uint32_t BlockSize;
	std::uint32_t SneakSize;
	std::uint32_t CriticalSize;
	std::uint32_t HostileSingleDamage;
	std::uint32_t HostileTimeDamage;
	std::uint32_t FollowerSingleDamage;
	std::uint32_t FollowerTimeDamage;
	std::uint32_t Block;
	std::uint32_t Sneak;
	std::uint32_t Critical;
	std::uint32_t HealthHeal;
	std::uint32_t MagickaDamage;
	std::uint32_t MagickaHeal;
	std::uint32_t StaminaDamage;
	std::uint32_t StaminaHeal;
	float PositionX;
	float PositionY;
	std::uint32_t DamageDisplayMode;
	std::uint32_t HealDisplayMode;
	double DisplayDispersion;
	std::uint32_t TimeDamageDifference;
	std::uint32_t TimeDamageInterval;
	std::uint32_t MinHeal;
	std::uint32_t EnableBlock;
	std::uint32_t EnableSneak;
	std::uint32_t EnableCritical;
	std::uint32_t EnableHealth;
	std::uint32_t EnableMagicka;
	std::uint32_t EnableStamina;
	double NearRadius;
	double FarRadius;
	std::uint32_t ListUpdateTime;

	INIFile();
	void Load();

private:
	std::string GetSkyrimPath();
	std::string GetSksePluginPath();
	bool IsFoundFile(const char* fileName);
	std::vector<std::string> GetSectionKeys(LPCTSTR section_name, LPCTSTR ini_file_path);
	void ToLower(std::string& str);
	std::vector<std::string> Split(const std::string& str, char sep);

	void SetSettings();
	std::string GetINIlFile();
	void SetINIData(std::vector<std::string>* list);
	void GetSettings();
	void ShowSettings();

	std::unordered_map<std::string, std::string> stringsMap;
	std::unordered_map<std::string, std::uint32_t> settingsMap;
	std::string iniFilePath;
};

extern INIFile ini;
