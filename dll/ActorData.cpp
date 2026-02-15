#include "ActorData.h"
#include "FloatingDamage.h"
#include "iniSettings.h"

#include <skse64/GameForms.h>
#include <skse64/GameRTTI.h>
#include <skse64/PluginAPI.h>
#include <skse64/GameCamera.h>
#include <skse64/GameData.h>

# include <iostream>
# include <random>
# include <string>


static double Random()
{
	if (ini.DisplayDispersion == 0.0)
		return 0.0;

	static std::random_device rd;
	static std::mt19937 mt(rd());
	static std::uniform_real_distribution<double> score(-ini.DisplayDispersion, ini.DisplayDispersion);

	return score(mt);
}


ActorData::ActorData(UInt32 formid) : formId(formid), combatCount(0), removeTime(0)
{
	for (int i = 0; i < ActorValues_NumValues; i++)
	{
		preValue[i] = 0.0f;
		for (int j = 0; j < 3; j++)
		{
			preDamage[i][j] = 0.0f;
		}
		longDamage[i] = 0.0f;
		longHeal[i] = 0.0f;
		longDamageTime[i] = 0;
		longHealTime[i] = 0;
		overDamage[i] = false;
		overHeal[i] = false;
	}
}


bool ActorData::Update()
{
	TESForm* form = LookupFormByID(formId);
	if (!form)
		return true;
	Actor* actor = DYNAMIC_CAST(form, TESForm, Actor);
	if (!actor)
		return true;

	float current = actor->actorValueOwner.GetCurrent(24);
	if (ini.EnableHealth != 0)
		CheckActorValue(actor, ActorValues_Health, current);
	if (ini.EnableMagicka != 0)
		CheckActorValue(actor, ActorValues_Magicka, actor->actorValueOwner.GetCurrent(25));
	if (ini.EnableStamina != 0)
		CheckActorValue(actor, ActorValues_Stamina, actor->actorValueOwner.GetCurrent(26));

	if (!hitEvents.empty())
	{
		auto it = hitEvents.begin();
		while (it != hitEvents.end())
		{
			if (it->first == 0)
			{
				SetDamage(actor, it->second);
				it = hitEvents.erase(it);
			}
			else
			{
				it->first--;
				it++;
			}
		}
	}

	if (actor->IsDead(1) || (current < 0.0f && (actor->flags2 & 0x40000) == 0))
	{
		if (removeTime == 0)
		{
			removeTime = 1;
		}
		else if (removeTime == 1)
		{
			removeTime = 2;
			for (int i = 0; i < ActorValues_NumValues; i++)
			{
				preDamage[i][0] = 0.0f;
				overDamage[i] = false;
				overHeal[i] = false;
			}
		}
		else
		{
			if (!hitEvents.empty())
			{
				auto it = hitEvents.begin();
				while (it != hitEvents.end())
				{
					SetDamage(actor, it->second);
					it = hitEvents.erase(it);
				}
			}

			return true;
		}
	}

	return false;
}


void ActorData::CheckActorValue(Actor* actor, ActorValues type, float current)
{
	float damage = preValue[type] - current;
	preValue[type] = current;

	if (overDamage[type])
	{
		SetDamage(actor, type, preDamage[type][1] + 0.5f);
		overDamage[type] = false;
	}
	else if (overHeal[type])
	{
		SetDamage(actor, type + 10, -(preDamage[type][1] - 0.5f));
		overHeal[type] = false;
	}
	else if (preDamage[type][1] != 0.0f)
	{
		if (preDamage[type][1] > 0.0f)
		{
			if (preDamage[type][2] <= 0.0f || std::fabs(preDamage[type][2] - preDamage[type][1]) > ini.TimeDamageDifference)
			{
				if (preDamage[type][0] <= 0.0f || std::fabs(preDamage[type][1] - preDamage[type][0]) > ini.TimeDamageDifference)
				{
					SetDamage(actor, type, preDamage[type][1] + 0.5f);
				}
				else if (damage <= 0.0f || std::fabs(preDamage[type][0] - damage) > ini.TimeDamageDifference)
				{
					SetDamage(actor, type, preDamage[type][1] + 0.5f);
					overDamage[type] = true;
				}
				else
				{
					longDamage[type] += preDamage[type][2] = preDamage[type][1];
				}
			}
			else
			{
				longDamage[type] += preDamage[type][2] = preDamage[type][1];
			}
		}
		else
		{
			if (preDamage[type][2] >= 0.0f || std::fabs(preDamage[type][2] - preDamage[type][1]) > ini.TimeDamageDifference)
			{
				if (preDamage[type][0] >= 0.0f || std::fabs(preDamage[type][1] - preDamage[type][0]) > ini.TimeDamageDifference)
				{
					SetDamage(actor, type + 10, -(preDamage[type][1] - 0.5f));
				}
				else if (damage >= 0.0f || std::fabs(preDamage[type][0] - damage) > ini.TimeDamageDifference)
				{
					SetDamage(actor, type + 10, -(preDamage[type][1] - 0.5f));
					overHeal[type] = true;
				}
				else
				{
					longHeal[type] += preDamage[type][2] = preDamage[type][1];
				}
			}
			else
			{
				longHeal[type] += preDamage[type][2] = preDamage[type][1];
			}
		}
	}
	else
	{
		preDamage[type][2] = 0.0f;
	}

	if (longDamage[type] > 0.0f && ++longDamageTime[type] >= ini.TimeDamageInterval)
	{
		if (type == ActorValues_Health)
			SetDamage(actor, -1, longDamage[type] + 0.5f);
		else
			SetDamage(actor, type, longDamage[type] + 0.5f);
		longDamage[type] = 0.0f;
		longDamageTime[type] = 0;
	}

	if (longHeal[type] < 0.0f)
	{
		if (type == ActorValues_Health && actor->IsInCombat())
			combatCount++;
		if (++longHealTime[type] >= ini.TimeDamageInterval)
		{
			if (ini.HideRegen)
			{
				FloatingDamage *FloatingDamage = FloatingDamage::GetSingleton();
				if (FloatingDamage && !FloatingDamage->t_list.empty())
				{
					float result = 0.0f;

					if (type == ActorValues_Health)
					{
						float regen = actor->actorValueOwner.GetMaximum(24) * actor->actorValueOwner.GetCurrent(27) * actor->actorValueOwner.GetCurrent(155) / 10000.0f;
						float combat_regen = regen * actor->actorValueOwner.GetCurrent(134);

						if ((actor->flags2 & 0x40000) != 0)
						{
							if (regen > 0.0f)
								regen += 5.0f;
							else
								regen = 5.0f;
						}

						float combat_ratio = (float)combatCount / (float)longHealTime[type];
						result = regen * (1.0f - combat_ratio) + combat_regen * combat_ratio;
					}
					else if (type == ActorValues_Magicka)
					{
						result = actor->actorValueOwner.GetMaximum(25) * actor->actorValueOwner.GetCurrent(28) * actor->actorValueOwner.GetCurrent(156) / 10000.0f;
					}
					else if (type == ActorValues_Stamina)
					{
						result = actor->actorValueOwner.GetMaximum(26) * actor->actorValueOwner.GetCurrent(29) * actor->actorValueOwner.GetCurrent(157) / 10000.0f;
					}

					result *= std::chrono::duration_cast<std::chrono::milliseconds>(FloatingDamage->t_list.back() - FloatingDamage->t_list.front()).count() / 1000.0;

					if (result > 0.0f)
						longHeal[type] += result;

				}
			}
			if (longHeal[type] <= -0.5f)
				SetDamage(actor, type + 10, -(longHeal[type] - 0.5f));

			longHeal[type] = 0.0f;
			longHealTime[type] = 0;
			if (type == ActorValues_Health)
				combatCount = 0;
		}
	}

	preDamage[type][1] = preDamage[type][0];
	preDamage[type][0] = damage;
}


void ActorData::SetDamage(Actor* actor, int flag, UInt32 damage)
{
	switch (flag)
	{
	case -4:
		if (ini.EnableCritical == 2)
			return;
		break;
	case -3:
		if (ini.EnableSneak == 2)
			return;
		break;
	case -2:
		if (ini.EnableBlock == 2)
			return;
		break;
	case -1:
	case 0:
		if (ini.EnableHealth == 2 || damage == 0)
			return;
		break;
	case 1:
		if (ini.EnableMagicka == 2 || damage == 0)
			return;
		break;
	case 2:
		if (ini.EnableStamina == 2 || damage == 0)
			return;
		break;
	case 10:
		if (ini.EnableHealth == 1 || damage < ini.MinHeal)
			return;
		break;
	case 11:
		if (ini.EnableMagicka == 1 || damage < ini.MinHeal)
			return;
		break;
	case 12:
		if (ini.EnableStamina == 1 || damage < ini.MinHeal)
			return;
		break;
	default:
		return;
	}

	if (!actor || actor->GetNiNode() == nullptr)
		return;

	FloatingDamage *FloatingDamage = FloatingDamage::GetSingleton();
	if (!FloatingDamage || !FloatingDamage->view)
		return;

	bool isPlayer = actor == *g_thePlayer;
	bool isFirstPerson = false;
	if (isPlayer)
	{
		PlayerCamera* camera = PlayerCamera::GetSingleton();
		isFirstPerson = camera && camera->cameraState == camera->cameraStates[PlayerCamera::kCameraState_FirstPerson];
	}
	else if (ini.HideNoLOS)
	{
		UInt8 unk1 = 0;
		if (!HasLOS((*g_thePlayer), actor, &unk1))
			return;
	}

	NiPoint3 screenPos;
	if (isFirstPerson)
	{
		if (!ini.EnablePlayer)
			return;

		screenPos.x = ini.PositionX;
		screenPos.y = ini.PositionY;
		screenPos.z = 0.5f;
	}
	else
	{
		NiPoint3 worldPos;
		actor->GetMarkerPosition(&worldPos);
		static NiRect<float> viewPort = { 0.0f, 1.0f, 0.0f, 1.0f };
		WorldPtToScreenPt3_Internal(g_worldToCamMatrix, &viewPort, &worldPos, &screenPos.x, &screenPos.y, &screenPos.z, 1e-5f);
		if (screenPos.z <= 0.0f || screenPos.x < 0.0f || screenPos.x > 1.0f || screenPos.y < 0.0f || screenPos.y > 1.0f)
			return;
	}

	double distance;
	if (screenPos.z >= 1.0f)
		distance = 999999999.0;
	else
		distance = 1.0 / (1.0f - screenPos.z);
	double scale = (40.0 / distance) * 100.0;
	if (scale > 150.0)
		scale = 150.0;
	else if (scale < 75.0)
		scale = 75.0;

	std::vector<UInt32> colors;
	std::vector<std::string> texts;
	std::vector<int> flags;
	flags.push_back(flag);
	bool flag_Sneak = false;
	bool flag_Critical = false;
	switch (flag)
	{
	case -4:
		screenPos.x += Random() * scale / 100;
		screenPos.y += Random() * scale / 100;
		colors.push_back(ini.Critical);
		texts.push_back(ini.CriticalString);
		break;
	case -3:
		screenPos.x += Random() * scale / 100;
		screenPos.y += Random() * scale / 100;
		colors.push_back(ini.Sneak);
		texts.push_back(ini.SneakString);
		break;
	case -2:
		screenPos.x += Random() * scale / 100;
		screenPos.y += Random() * scale / 100;
		colors.push_back(ini.Block);
		texts.push_back(ini.BlockString);
		break;
	case -1:
		if (isPlayer || (actor->flags1 & Actor::kFlags_IsPlayerTeammate) != 0)
			colors.push_back(ini.FollowerTimeDamage);
		else
			colors.push_back(ini.HostileTimeDamage);
		texts.push_back(ini.TimeDamageFront + std::to_string(damage) + ini.TimeDamageBack);
		break;
	case 0:
		screenPos.x += Random() * scale / 100;
		screenPos.y += Random() * scale / 100;
		if (isPlayer || (actor->flags1 & Actor::kFlags_IsPlayerTeammate) != 0)
			colors.push_back(ini.FollowerSingleDamage);
		else
			colors.push_back(ini.HostileSingleDamage);
		texts.push_back(ini.SingleDamageFront + std::to_string(damage) + ini.SingleDamageBack);

		if (!hitEvents.empty())
		{
			bool block = false;
			bool sneak = false;
			bool critical = false;

			auto it = hitEvents.begin();
			while (it != hitEvents.end())
			{
				bool done = false;
				switch (it->second)
				{
				case HitTypes_Block:
					if (!block)
					{
						block = true;
						if (ini.EnableBlock != 2)
						{
							colors.push_back(ini.Block);
							texts.push_back(ini.BlockString);
							flags.push_back(-5);
						}

						if (ini.EnableBlock != 1 && (!critical || ini.EnableCritical == 1) && (!sneak || ini.EnableSneak == 1))
						{
							colors[0] = ini.Critical;
							flags[0] = HitTypes_Block - 10;
						}

						it = hitEvents.erase(it);
						done = true;
					}
					break;
				case HitTypes_Sneak:
					if (!sneak)
					{
						sneak = true;
						if (ini.EnableSneak != 2)
						{
							colors.push_back(ini.Sneak);
							texts.push_back(ini.SneakString);
							flags.push_back(-6);
							flag_Sneak = true;
						}

						if (ini.EnableSneak != 1 && (!critical || ini.EnableCritical == 1))
						{
							colors[0] = ini.Sneak;
							flags[0] = HitTypes_Sneak - 10;
						}

						it = hitEvents.erase(it);
						done = true;
					}
					break;
				case HitTypes_Critical:
					if (!critical)
					{
						critical = true;
						if (ini.EnableCritical != 2)
						{
							colors.push_back(ini.Critical);
							texts.push_back(ini.CriticalString);
							flags.push_back(-7);
							flag_Critical = true;
						}

						if (ini.EnableCritical != 1)
						{
							colors[0] = ini.Critical;
							flags[0] = HitTypes_Critical - 10;
						}

						it = hitEvents.erase(it);
						done = true;
					}
					break;
				}

				if (!done)
					it++;
			}
		}
		break;
	case 1:
		colors.push_back(ini.MagickaDamage);
		texts.push_back(ini.MagickaDamageFront + std::to_string(damage) + ini.MagickaDamageBack);
		break;
	case 2:
		colors.push_back(ini.StaminaDamage);
		texts.push_back(ini.StaminaDamageFront + std::to_string(damage) + ini.StaminaDamageBack);
		break;
	case 10:
		colors.push_back(ini.HealthHeal);
		texts.push_back(ini.HealthHealFront + std::to_string(damage) + ini.HealthHealBack);
		break;
	case 11:
		colors.push_back(ini.MagickaHeal);
		texts.push_back(ini.MagickaHealFront + std::to_string(damage) + ini.MagickaHealBack);
		break;
	case 12:
		colors.push_back(ini.StaminaHeal);
		texts.push_back(ini.StaminaHealFront + std::to_string(damage) + ini.StaminaHealBack);
		break;
	default:
		return;
	}

	if (flags.size() >= 3)
	{
		for (auto& f : flags)
		{
			if (f == -5)
			{
				if (flag_Critical)
				{
					if (flag_Sneak)
						f = -10;
					else
						f = -8;
				}
				else if (flag_Sneak)
				{
					f = -9;
				}
			}
			else if (f == -6)
			{
				if (flag_Critical)
					f = -11;
			}
		}
	}

	for (int i = 0; i < flags.size(); i++)
	{
		GFxValue args[6];
		args[0].SetString(texts[i].c_str());
		args[1].SetNumber((double)screenPos.x);
		args[2].SetNumber((double)screenPos.y);
		args[3].SetNumber(scale);
		args[4].SetNumber((double)colors[i]);
		args[5].SetNumber((double)flags[i]);

		FloatingDamage->view->Invoke("_root.widget.SetDamageText", nullptr, args, 6);
	}
}