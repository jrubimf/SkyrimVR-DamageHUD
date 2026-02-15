#include "Settings.h"

#include <cmath>
#include <fstream>

INIFile ini;

INIFile::INIFile()
	: EnableHealth(3)
	, Size(24)
	, Alpha(100)
	, DisplayDispersion(2)
	, EnableBlock(1)
	, EnableSneak(1)
	, EnableCritical(1)
	, HostileSingleDamage(0xFFFFFF)
	, HostileTimeDamage(0xFFD700)
	, FollowerSingleDamage(0xFF0000)
	, FollowerTimeDamage(0x800080)
	, HealthHeal(0x00FF00)
	, Block(0xFF8C00)
	, Sneak(0xFF8C00)
	, Critical(0xFF8C00)
	, HideRegen(true)
	, MinHeal(8)
	, Name("$EverywhereMediumFont")
	, BlockString("BLOCK")
	, SneakString("SNEAK")
	, CriticalString("CRITICAL")
	, SingleDamageFront("")
	, SingleDamageBack("")
	, TimeDamageFront("")
	, TimeDamageBack("")
	, HealthHealFront("")
	, HealthHealBack("")
	, BlockSize(24)
	, SneakSize(24)
	, CriticalSize(24)
	, TimeDamageDifference(3)
	, TimeDamageInterval(61)
	, ListUpdateTime(100)
	, NearRadius(std::pow(7000, 2.0))
	, FarRadius(std::pow(8000, 2.0))
{
}

std::string INIFile::GetSkyrimDataPath()
{
	char buf[MAX_PATH];
	GetModuleFileNameA(nullptr, buf, MAX_PATH);
	std::string path(buf);
	auto pos = path.find_last_of('\\');
	if (pos != std::string::npos)
		path = path.substr(0, pos);
	path += "\\Data\\";
	return path;
}

std::string INIFile::FindINIPath()
{
	std::string dataPath = GetSkyrimDataPath();

	// Primary: MCM Helper settings path (written when user changes settings)
	std::string mcmPath = dataPath + "MCM\\Settings\\FloatingDamage.ini";
	std::ifstream mcmFile(mcmPath);
	if (mcmFile.good()) {
		logger::info("Found MCM settings: {}", mcmPath);
		return mcmPath;
	}

	// Fallback: old SKSE plugins path (backward compat)
	std::string sksePath = dataPath + "SKSE\\Plugins\\FloatingDamage.ini";
	std::ifstream skseFile(sksePath);
	if (skseFile.good()) {
		logger::info("Found legacy INI: {}", sksePath);
		return sksePath;
	}

	logger::info("No INI found, using defaults");
	return "";
}

void INIFile::ReadSettings(const std::string& path)
{
	if (path.empty())
		return;

	LPCSTR file = path.c_str();

	// Display
	EnableHealth = GetPrivateProfileIntA("Display", "iEnableHealth", EnableHealth, file);
	Size = GetPrivateProfileIntA("Display", "iSize", Size, file);
	Alpha = GetPrivateProfileIntA("Display", "iAlpha", Alpha, file);
	DisplayDispersion = GetPrivateProfileIntA("Display", "iDispersion", DisplayDispersion, file);
	EnableBlock = GetPrivateProfileIntA("Display", "iEnableBlock", EnableBlock, file);
	EnableSneak = GetPrivateProfileIntA("Display", "iEnableSneak", EnableSneak, file);
	EnableCritical = GetPrivateProfileIntA("Display", "iEnableCritical", EnableCritical, file);

	// Colors
	HostileSingleDamage = GetPrivateProfileIntA("Colors", "iDamageColor", HostileSingleDamage, file);
	HostileTimeDamage = GetPrivateProfileIntA("Colors", "iDoTColor", HostileTimeDamage, file);
	FollowerSingleDamage = GetPrivateProfileIntA("Colors", "iReceivedColor", FollowerSingleDamage, file);
	FollowerTimeDamage = GetPrivateProfileIntA("Colors", "iReceivedDoTColor", FollowerTimeDamage, file);
	HealthHeal = GetPrivateProfileIntA("Colors", "iHealColor", HealthHeal, file);
	Block = GetPrivateProfileIntA("Colors", "iBlockColor", Block, file);
	Sneak = GetPrivateProfileIntA("Colors", "iSneakColor", Sneak, file);
	Critical = GetPrivateProfileIntA("Colors", "iCriticalColor", Critical, file);

	// Behavior
	HideRegen = GetPrivateProfileIntA("Behavior", "bHideRegen", HideRegen ? 1 : 0, file) != 0;
	MinHeal = GetPrivateProfileIntA("Behavior", "iMinHeal", MinHeal, file);

	// Clamp values
	if (Alpha > 100) Alpha = 100;
	if (EnableHealth > 3) EnableHealth = 3;
	if (EnableBlock > 2) EnableBlock = 2;
	if (EnableSneak > 2) EnableSneak = 2;
	if (EnableCritical > 2) EnableCritical = 2;
	if (MinHeal < 1) MinHeal = 1;

	logger::info("Settings loaded from: {}", path);
	logger::info("  EnableHealth={} Size={} Alpha={} Dispersion={}", EnableHealth, Size, Alpha, DisplayDispersion);
	logger::info("  DmgColor=0x{:06X} DoTColor=0x{:06X} RcvColor=0x{:06X} RcvDoT=0x{:06X}",
		HostileSingleDamage, HostileTimeDamage, FollowerSingleDamage, FollowerTimeDamage);
	logger::info("  HealColor=0x{:06X} BlockColor=0x{:06X} SneakColor=0x{:06X} CritColor=0x{:06X}",
		HealthHeal, Block, Sneak, Critical);
	logger::info("  HideRegen={} MinHeal={}", HideRegen, MinHeal);
}

void INIFile::Load()
{
	cachedIniPath = FindINIPath();
	ReadSettings(cachedIniPath);
}

void INIFile::Reload()
{
	// Re-find in case MCM Helper created the file since last Load
	cachedIniPath = FindINIPath();
	ReadSettings(cachedIniPath);
	logger::info("Settings reloaded");
}
