#pragma once

#include <string>
#include <windows.h>

class INIFile
{
public:
	// Display
	std::uint32_t EnableHealth;
	std::uint32_t Size;
	std::uint32_t Alpha;
	std::uint32_t DisplayDispersion;
	std::uint32_t EnableBlock;
	std::uint32_t EnableSneak;
	std::uint32_t EnableCritical;

	// Colors
	std::uint32_t HostileSingleDamage;   // Damage dealt
	std::uint32_t HostileTimeDamage;     // Damage over time dealt
	std::uint32_t FollowerSingleDamage;  // Damage received
	std::uint32_t FollowerTimeDamage;    // Damage over time received
	std::uint32_t HealthHeal;
	std::uint32_t Block;
	std::uint32_t Sneak;
	std::uint32_t Critical;

	// Behavior
	bool HideRegen;
	std::uint32_t MinHeal;

	// Hardcoded / internal
	std::string Name;
	std::string BlockString;
	std::string SneakString;
	std::string CriticalString;
	std::string SingleDamageFront;
	std::string SingleDamageBack;
	std::string TimeDamageFront;
	std::string TimeDamageBack;
	std::string HealthHealFront;
	std::string HealthHealBack;
	std::uint32_t BlockSize;
	std::uint32_t SneakSize;
	std::uint32_t CriticalSize;
	std::uint32_t TimeDamageDifference;
	std::uint32_t TimeDamageInterval;
	std::uint32_t ListUpdateTime;
	double NearRadius;
	double FarRadius;

	INIFile();
	void Load();
	void Reload();

private:
	std::string GetSkyrimDataPath();
	std::string FindINIPath();
	void ReadSettings(const std::string& path);

	std::string cachedIniPath;
};

extern INIFile ini;
