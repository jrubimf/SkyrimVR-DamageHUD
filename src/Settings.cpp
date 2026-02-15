#include "Settings.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")

INIFile ini;

INIFile::INIFile()
	: Name("$EverywhereMediumFont")
	, SingleDamageFront("")
	, SingleDamageBack("")
	, TimeDamageFront("")
	, TimeDamageBack("")
	, BlockString("BLOCK")
	, SneakString("SNEAK")
	, CriticalString("CRITICAL")
	, HealthHealFront("")
	, HealthHealBack("")
	, MagickaDamageFront("")
	, MagickaDamageBack("")
	, MagickaHealFront("")
	, MagickaHealBack("")
	, StaminaDamageFront("")
	, StaminaDamageBack("")
	, StaminaHealFront("")
	, StaminaHealBack("")
	, EnablePlayer(true)
	, HideRegen(true)
	, HideNoLOS(false)
	, OnlyHostile(false)
	, Size(24)
	, Alpha(100)
	, BlockSize(24)
	, SneakSize(24)
	, CriticalSize(24)
	, HostileSingleDamage(0xFFFFFF)
	, HostileTimeDamage(0xFFD700)
	, FollowerSingleDamage(0xFF0000)
	, FollowerTimeDamage(0x800080)
	, Block(0xFF8C00)
	, Sneak(0xFF8C00)
	, Critical(0xFF8C00)
	, HealthHeal(0x00FF00)
	, MagickaDamage(0x0000FF)
	, MagickaHeal(0xFF00FF)
	, StaminaDamage(0x008000)
	, StaminaHeal(0x00FFFF)
	, PositionX(0.5f)
	, PositionY(0.75f)
	, DamageDisplayMode(0)
	, HealDisplayMode(0)
	, DisplayDispersion(0.02)
	, TimeDamageDifference(3)
	, TimeDamageInterval(60 + 1)
	, MinHeal(8)
	, EnableBlock(1)
	, EnableSneak(1)
	, EnableCritical(1)
	, EnableHealth(3)
	, EnableMagicka(0)
	, EnableStamina(0)
	, NearRadius(std::pow(7000, 2.0))
	, FarRadius(std::pow(8000, 2.0))
	, ListUpdateTime(100)
{
	stringsMap["name"] = "$EverywhereMediumFont";
	stringsMap["singledamagefront"] = "";
	stringsMap["singledamageback"] = "";
	stringsMap["timedamagefront"] = "";
	stringsMap["timedamageback"] = "";
	stringsMap["blockstring"] = "BLOCK";
	stringsMap["sneakstring"] = "SNEAK";
	stringsMap["criticalstring"] = "CRITICAL";
	stringsMap["healthhealfront"] = "";
	stringsMap["healthhealback"] = "";
	stringsMap["magickadamagefront"] = "";
	stringsMap["magickadamageback"] = "";
	stringsMap["magickahealfront"] = "";
	stringsMap["magickahealback"] = "";
	stringsMap["staminadamagefront"] = "";
	stringsMap["staminadamageback"] = "";
	stringsMap["staminahealfront"] = "";
	stringsMap["staminahealback"] = "";

	settingsMap["enableplayer"] = 1;
	settingsMap["hideregen"] = 1;
	settingsMap["hidenolos"] = 0;
	settingsMap["onlyhostile"] = 0;
	settingsMap["size"] = 24;
	settingsMap["alpha"] = 100;
	settingsMap["blocksize"] = 24;
	settingsMap["sneaksize"] = 24;
	settingsMap["criticalsize"] = 24;
	settingsMap["hostilesingledamage"] = 0xFFFFFF;
	settingsMap["hostiletimedamage"] = 0xFFD700;
	settingsMap["followersingledamage"] = 0xFF0000;
	settingsMap["followertimedamage"] = 0x800080;
	settingsMap["block"] = 0xFF8C00;
	settingsMap["sneak"] = 0xFF8C00;
	settingsMap["critical"] = 0xFF8C00;
	settingsMap["healthheal"] = 0x00FF00;
	settingsMap["magickadamage"] = 0x0000FF;
	settingsMap["magickaheal"] = 0xFF00FF;
	settingsMap["staminadamage"] = 0x008000;
	settingsMap["staminaheal"] = 0x00FFFF;
	settingsMap["positionx"] = 50;
	settingsMap["positiony"] = 75;
	settingsMap["damagedisplaymode"] = 0;
	settingsMap["healdisplaymode"] = 0;
	settingsMap["displaydispersion"] = 2;
	settingsMap["timedamagedifference"] = 3;
	settingsMap["timedamageinterval"] = 60;
	settingsMap["minheal"] = 8;
	settingsMap["enableblock"] = 1;
	settingsMap["enablesneak"] = 1;
	settingsMap["enablecritical"] = 1;
	settingsMap["enablehealth"] = 3;
	settingsMap["enablemagicka"] = 0;
	settingsMap["enablestamina"] = 0;
	settingsMap["nearradius"] = 7000;
	settingsMap["farradius"] = 8000;
	settingsMap["listupdatetime"] = 100;
}

void INIFile::Load()
{
	SetSettings();
	GetSettings();
	ShowSettings();
	stringsMap.clear();
	settingsMap.clear();
}

std::string INIFile::GetSkyrimPath()
{
	std::string result;
	char buf[MAX_PATH];
	GetModuleFileName(nullptr, buf, MAX_PATH);
	PathRemoveFileSpec(buf);
	result = buf;
	result += "\\";
	return result;
}

std::string INIFile::GetSksePluginPath()
{
	std::string result = GetSkyrimPath();
	result += "data\\SKSE\\Plugins\\";
	return result;
}

bool INIFile::IsFoundFile(const char* fileName)
{
	std::ifstream ifs(fileName);
	return !ifs.fail();
}

std::vector<std::string> INIFile::GetSectionKeys(LPCTSTR section_name, LPCTSTR ini_file_path)
{
	std::vector<std::string> result;
	std::string file_path(ini_file_path);
	if (IsFoundFile(ini_file_path)) {
		TCHAR buf[32768] = {};
		GetPrivateProfileSection(section_name, buf, sizeof(buf), ini_file_path);
		for (LPTSTR seek = buf; *seek != '\0'; seek++) {
			std::string str(seek);
			result.push_back(str);
			while (*seek != '\0')
				seek++;
		}
	}
	return result;
}

void INIFile::ToLower(std::string& str)
{
	for (auto& c : str)
		c = static_cast<char>(tolower(c));
}

std::vector<std::string> INIFile::Split(const std::string& str, char sep)
{
	std::vector<std::string> result;
	auto first = str.begin();
	while (first != str.end()) {
		auto last = first;
		while (last != str.end() && *last != sep)
			++last;
		result.push_back(std::string(first, last));
		if (last != str.end())
			++last;
		first = last;
	}
	return result;
}

void INIFile::SetSettings()
{
	static std::string sections[] = { "Font", "Color", "String", "FirstPerson", "Behavior", "NearActors" };
	for (int i = 0; i < sizeof(sections) / sizeof(std::string); i++) {
		std::vector<std::string> list = GetSectionKeys(sections[i].c_str(), GetINIlFile().c_str());
		std::sort(list.begin(), list.end());
		SetINIData(&list);
	}
}

std::string INIFile::GetINIlFile()
{
	if (iniFilePath.empty()) {
		iniFilePath = GetSksePluginPath();
		iniFilePath += INI_FILE;
	}
	return iniFilePath;
}

void INIFile::SetINIData(std::vector<std::string>* list)
{
	for (std::string str : *list) {
		if (str.empty())
			continue;
		auto vec = Split(str, '=');
		if (vec.size() != 2)
			continue;
		std::string key = vec.at(0);
		ToLower(key);

		if (!key.empty()) {
			if (key == "name") {
				stringsMap["name"] = vec.at(1);
			} else if (key == "singledamagefront") {
				stringsMap["singledamagefront"] = vec.at(1);
			} else if (key == "singledamageback") {
				stringsMap["singledamageback"] = vec.at(1);
			} else if (key == "timedamagefront") {
				stringsMap["timedamagefront"] = vec.at(1);
			} else if (key == "timedamageback") {
				stringsMap["timedamageback"] = vec.at(1);
			} else if (key == "blockstring") {
				stringsMap["blockstring"] = vec.at(1);
			} else if (key == "sneakstring") {
				stringsMap["sneakstring"] = vec.at(1);
			} else if (key == "criticalstring") {
				stringsMap["criticalstring"] = vec.at(1);
			} else if (key == "healthhealfront") {
				stringsMap["healthhealfront"] = vec.at(1);
			} else if (key == "healthhealback") {
				stringsMap["healthhealback"] = vec.at(1);
			} else if (key == "magickadamagefront") {
				stringsMap["magickadamagefront"] = vec.at(1);
			} else if (key == "magickadamageback") {
				stringsMap["magickadamageback"] = vec.at(1);
			} else if (key == "magickahealfront") {
				stringsMap["magickahealfront"] = vec.at(1);
			} else if (key == "magickahealback") {
				stringsMap["magickahealback"] = vec.at(1);
			} else if (key == "staminadamagefront") {
				stringsMap["staminadamagefront"] = vec.at(1);
			} else if (key == "staminadamageback") {
				stringsMap["staminadamageback"] = vec.at(1);
			} else if (key == "staminahealfront") {
				stringsMap["staminahealfront"] = vec.at(1);
			} else if (key == "staminahealback") {
				stringsMap["staminahealback"] = vec.at(1);
			} else if (key == "enableplayer") {
				std::string value = vec.at(1);
				ToLower(value);
				if (value != "true")
					settingsMap["enableplayer"] = 0;
			} else if (key == "hideregen") {
				std::string value = vec.at(1);
				ToLower(value);
				if (value != "true")
					settingsMap["hideregen"] = 0;
			} else if (key == "hidenolos") {
				std::string value = vec.at(1);
				ToLower(value);
				if (value == "true")
					settingsMap["hidenolos"] = 1;
			} else if (key == "onlyhostile") {
				std::string value = vec.at(1);
				ToLower(value);
				if (value == "true")
					settingsMap["onlyhostile"] = 1;
			} else {
				std::uint32_t value = 0;
				try {
					value = std::stoi(vec.at(1), nullptr, 0);
				} catch (const std::invalid_argument& e) {
					logger::warn("{} : invalid argument - {}", vec.at(0), e.what());
				} catch (const std::out_of_range& e) {
					logger::warn("{} : out of range - {}", vec.at(0), e.what());
				}

				if (key == "size") {
					settingsMap["size"] = value;
				} else if (key == "alpha") {
					if (value <= 100)
						settingsMap["alpha"] = value;
				} else if (key == "blocksize") {
					settingsMap["blocksize"] = value;
				} else if (key == "sneaksize") {
					settingsMap["sneaksize"] = value;
				} else if (key == "criticalsize") {
					settingsMap["criticalsize"] = value;
				} else if (key == "hostilesingledamage") {
					if (value <= 0xFFFFFF)
						settingsMap["hostilesingledamage"] = value;
				} else if (key == "hostiletimedamage") {
					if (value <= 0xFFFFFF)
						settingsMap["hostiletimedamage"] = value;
				} else if (key == "followersingledamage") {
					if (value <= 0xFFFFFF)
						settingsMap["followersingledamage"] = value;
				} else if (key == "followertimedamage") {
					if (value <= 0xFFFFFF)
						settingsMap["followertimedamage"] = value;
				} else if (key == "block") {
					if (value <= 0xFFFFFF)
						settingsMap["block"] = value;
				} else if (key == "sneak") {
					if (value <= 0xFFFFFF)
						settingsMap["sneak"] = value;
				} else if (key == "critical") {
					if (value <= 0xFFFFFF)
						settingsMap["critical"] = value;
				} else if (key == "healthheal") {
					if (value <= 0xFFFFFF)
						settingsMap["healthheal"] = value;
				} else if (key == "magickadamage") {
					if (value <= 0xFFFFFF)
						settingsMap["magickadamage"] = value;
				} else if (key == "magickaheal") {
					if (value <= 0xFFFFFF)
						settingsMap["magickaheal"] = value;
				} else if (key == "staminadamage") {
					if (value <= 0xFFFFFF)
						settingsMap["staminadamage"] = value;
				} else if (key == "staminaheal") {
					if (value <= 0xFFFFFF)
						settingsMap["staminaheal"] = value;
				} else if (key == "positionx") {
					if (value <= 100)
						settingsMap["positionx"] = value;
				} else if (key == "positiony") {
					if (value <= 100)
						settingsMap["positiony"] = value;
				} else if (key == "damagedisplaymode") {
					if (value <= 4)
						settingsMap["damagedisplaymode"] = value;
				} else if (key == "healdisplaymode") {
					if (value <= 4)
						settingsMap["healdisplaymode"] = value;
				} else if (key == "displaydispersion") {
					settingsMap["displaydispersion"] = value;
				} else if (key == "timedamagedifference") {
					if (value >= 1)
						settingsMap["timedamagedifference"] = value;
				} else if (key == "timedamageinterval") {
					if (value >= 10)
						settingsMap["timedamageinterval"] = value;
				} else if (key == "minheal") {
					if (value >= 1)
						settingsMap["minheal"] = value;
				} else if (key == "enableblock") {
					if (value <= 3)
						settingsMap["enableblock"] = value;
				} else if (key == "enablesneak") {
					if (value <= 3)
						settingsMap["enablesneak"] = value;
				} else if (key == "enablecritical") {
					if (value <= 3)
						settingsMap["enablecritical"] = value;
				} else if (key == "enablehealth") {
					if (value <= 3)
						settingsMap["enablehealth"] = value;
				} else if (key == "enablemagicka") {
					if (value <= 3)
						settingsMap["enablemagicka"] = value;
				} else if (key == "enablestamina") {
					if (value <= 3)
						settingsMap["enablestamina"] = value;
				} else if (key == "nearradius") {
					settingsMap["nearradius"] = value;
				} else if (key == "farradius") {
					settingsMap["farradius"] = value;
				} else if (key == "listupdatetime") {
					if (value >= 10)
						settingsMap["listupdatetime"] = value;
				}
			}
		}
	}
}

void INIFile::GetSettings()
{
	Name = stringsMap["name"];
	SingleDamageFront = stringsMap["singledamagefront"];
	SingleDamageBack = stringsMap["singledamageback"];
	TimeDamageFront = stringsMap["timedamagefront"];
	TimeDamageBack = stringsMap["timedamageback"];
	BlockString = stringsMap["blockstring"];
	SneakString = stringsMap["sneakstring"];
	CriticalString = stringsMap["criticalstring"];
	HealthHealFront = stringsMap["healthhealfront"];
	HealthHealBack = stringsMap["healthhealback"];
	MagickaDamageFront = stringsMap["magickadamagefront"];
	MagickaDamageBack = stringsMap["magickadamageback"];
	MagickaHealFront = stringsMap["magickahealfront"];
	MagickaHealBack = stringsMap["magickahealback"];
	StaminaDamageFront = stringsMap["staminadamagefront"];
	StaminaDamageBack = stringsMap["staminadamageback"];
	StaminaHealFront = stringsMap["staminahealfront"];
	StaminaHealBack = stringsMap["staminahealback"];
	if (settingsMap["enableplayer"] == 0)
		EnablePlayer = false;
	if (settingsMap["hideregen"] == 0)
		HideRegen = false;
	if (settingsMap["hidenolos"] == 1)
		HideNoLOS = true;
	if (settingsMap["onlyhostile"] == 1)
		OnlyHostile = true;
	Size = settingsMap["size"];
	Alpha = settingsMap["alpha"];
	BlockSize = settingsMap["blocksize"];
	SneakSize = settingsMap["sneaksize"];
	CriticalSize = settingsMap["criticalsize"];
	HostileSingleDamage = settingsMap["hostilesingledamage"];
	HostileTimeDamage = settingsMap["hostiletimedamage"];
	FollowerSingleDamage = settingsMap["followersingledamage"];
	FollowerTimeDamage = settingsMap["followertimedamage"];
	Block = settingsMap["block"];
	Sneak = settingsMap["sneak"];
	Critical = settingsMap["critical"];
	HealthHeal = settingsMap["healthheal"];
	MagickaDamage = settingsMap["magickadamage"];
	MagickaHeal = settingsMap["magickaheal"];
	StaminaDamage = settingsMap["staminadamage"];
	StaminaHeal = settingsMap["staminaheal"];
	PositionX = settingsMap["positionx"] / 100.0f;
	PositionY = settingsMap["positiony"] / 100.0f;
	DamageDisplayMode = settingsMap["damagedisplaymode"];
	HealDisplayMode = settingsMap["healdisplaymode"];
	if (settingsMap["displaydispersion"] == 0)
		DisplayDispersion = 0.0;
	else
		DisplayDispersion = settingsMap["displaydispersion"] / 100.0;
	TimeDamageDifference = settingsMap["timedamagedifference"];
	TimeDamageInterval = settingsMap["timedamageinterval"] + 1;
	MinHeal = settingsMap["minheal"];
	EnableBlock = settingsMap["enableblock"];
	EnableSneak = settingsMap["enablesneak"];
	EnableCritical = settingsMap["enablecritical"];
	EnableHealth = settingsMap["enablehealth"];
	EnableMagicka = settingsMap["enablemagicka"];
	EnableStamina = settingsMap["enablestamina"];
	NearRadius = std::pow(settingsMap["nearradius"], 2.0);
	if (std::pow(settingsMap["farradius"], 2.0) > NearRadius) {
		FarRadius = std::pow(settingsMap["farradius"], 2.0);
	} else {
		FarRadius = NearRadius;
		settingsMap["farradius"] = settingsMap["nearradius"];
	}
	ListUpdateTime = settingsMap["listupdatetime"];
}

void INIFile::ShowSettings()
{
	logger::info("Settings");

	for (auto& map : stringsMap) {
		logger::info("   {}  =  {}", map.first, map.second);
	}

	for (auto& map : settingsMap) {
		logger::info("   {}  =  {}", map.first, map.second);
	}
}
