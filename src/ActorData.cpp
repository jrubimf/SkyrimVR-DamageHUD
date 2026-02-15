#include "ActorData.h"
#include "FloatingDamage.h"
#include "Settings.h"

#include <random>
#include <string>

static double Random()
{
	if (ini.DisplayDispersion == 0.0)
		return 0.0;

	static std::random_device rd;
	static std::mt19937 mt(rd());
	static std::uniform_real_distribution<double> score(-ini.DisplayDispersion, ini.DisplayDispersion);

	return score(mt);
}

ActorData::ActorData(RE::FormID formid)
	: formId(formid)
	, combatCount(0)
	, removeTime(0)
{
	for (int i = 0; i < ActorValues_NumValues; i++) {
		preValue[i] = 0.0f;
		for (int j = 0; j < 3; j++) {
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
	auto* form = RE::TESForm::LookupByID(formId);
	if (!form)
		return true;
	auto* actor = form->As<RE::Actor>();
	if (!actor)
		return true;

	float current = actor->GetActorValue(RE::ActorValue::kHealth);
	if (ini.EnableHealth != 0)
		CheckActorValue(actor, ActorValues_Health, current);
	if (ini.EnableMagicka != 0)
		CheckActorValue(actor, ActorValues_Magicka, actor->GetActorValue(RE::ActorValue::kMagicka));
	if (ini.EnableStamina != 0)
		CheckActorValue(actor, ActorValues_Stamina, actor->GetActorValue(RE::ActorValue::kStamina));

	if (!hitEvents.empty()) {
		auto it = hitEvents.begin();
		while (it != hitEvents.end()) {
			if (it->first == 0) {
				SetDamage(actor, it->second);
				it = hitEvents.erase(it);
			} else {
				it->first--;
				it++;
			}
		}
	}

	if (actor->IsDead() || (current < 0.0f && !actor->IsEssential())) {
		if (removeTime == 0) {
			removeTime = 1;
		} else if (removeTime == 1) {
			removeTime = 2;
			for (int i = 0; i < ActorValues_NumValues; i++) {
				preDamage[i][0] = 0.0f;
				overDamage[i] = false;
				overHeal[i] = false;
			}
		} else {
			if (!hitEvents.empty()) {
				auto it = hitEvents.begin();
				while (it != hitEvents.end()) {
					SetDamage(actor, it->second);
					it = hitEvents.erase(it);
				}
			}
			return true;
		}
	}

	return false;
}

void ActorData::CheckActorValue(RE::Actor* actor, ActorValues type, float current)
{
	float damage = preValue[type] - current;
	preValue[type] = current;

	if (overDamage[type]) {
		SetDamage(actor, type, static_cast<std::uint32_t>(preDamage[type][1] + 0.5f));
		overDamage[type] = false;
	} else if (overHeal[type]) {
		SetDamage(actor, type + 10, static_cast<std::uint32_t>(-(preDamage[type][1] - 0.5f)));
		overHeal[type] = false;
	} else if (preDamage[type][1] != 0.0f) {
		if (preDamage[type][1] > 0.0f) {
			if (preDamage[type][2] <= 0.0f || std::fabs(preDamage[type][2] - preDamage[type][1]) > ini.TimeDamageDifference) {
				if (preDamage[type][0] <= 0.0f || std::fabs(preDamage[type][1] - preDamage[type][0]) > ini.TimeDamageDifference) {
					SetDamage(actor, type, static_cast<std::uint32_t>(preDamage[type][1] + 0.5f));
				} else if (damage <= 0.0f || std::fabs(preDamage[type][0] - damage) > ini.TimeDamageDifference) {
					SetDamage(actor, type, static_cast<std::uint32_t>(preDamage[type][1] + 0.5f));
					overDamage[type] = true;
				} else {
					longDamage[type] += preDamage[type][2] = preDamage[type][1];
				}
			} else {
				longDamage[type] += preDamage[type][2] = preDamage[type][1];
			}
		} else {
			if (preDamage[type][2] >= 0.0f || std::fabs(preDamage[type][2] - preDamage[type][1]) > ini.TimeDamageDifference) {
				if (preDamage[type][0] >= 0.0f || std::fabs(preDamage[type][1] - preDamage[type][0]) > ini.TimeDamageDifference) {
					SetDamage(actor, type + 10, static_cast<std::uint32_t>(-(preDamage[type][1] - 0.5f)));
				} else if (damage >= 0.0f || std::fabs(preDamage[type][0] - damage) > ini.TimeDamageDifference) {
					SetDamage(actor, type + 10, static_cast<std::uint32_t>(-(preDamage[type][1] - 0.5f)));
					overHeal[type] = true;
				} else {
					longHeal[type] += preDamage[type][2] = preDamage[type][1];
				}
			} else {
				longHeal[type] += preDamage[type][2] = preDamage[type][1];
			}
		}
	} else {
		preDamage[type][2] = 0.0f;
	}

	if (longDamage[type] > 0.0f && ++longDamageTime[type] >= ini.TimeDamageInterval) {
		if (type == ActorValues_Health)
			SetDamage(actor, -1, static_cast<std::uint32_t>(longDamage[type] + 0.5f));
		else
			SetDamage(actor, type, static_cast<std::uint32_t>(longDamage[type] + 0.5f));
		longDamage[type] = 0.0f;
		longDamageTime[type] = 0;
	}

	if (longHeal[type] < 0.0f) {
		if (type == ActorValues_Health && actor->IsInCombat())
			combatCount++;
		if (++longHealTime[type] >= ini.TimeDamageInterval) {
			if (ini.HideRegen) {
				auto* floatingDmg = FloatingDamage::GetSingleton();
				if (floatingDmg && !floatingDmg->t_list.empty()) {
					float result = 0.0f;

					if (type == ActorValues_Health) {
						float regen = actor->GetPermanentActorValue(RE::ActorValue::kHealth) *
						              actor->GetActorValue(RE::ActorValue::kHealRate) *
						              actor->GetActorValue(RE::ActorValue::kHealRateMult) / 10000.0f;
						float combat_regen = regen * actor->GetActorValue(RE::ActorValue::kCombatHealthRegenMultiply);

						if (actor->IsEssential()) {
							if (regen > 0.0f)
								regen += 5.0f;
							else
								regen = 5.0f;
						}

						float combat_ratio = static_cast<float>(combatCount) / static_cast<float>(longHealTime[type]);
						result = regen * (1.0f - combat_ratio) + combat_regen * combat_ratio;
					} else if (type == ActorValues_Magicka) {
						result = actor->GetPermanentActorValue(RE::ActorValue::kMagicka) *
						         actor->GetActorValue(RE::ActorValue::kMagickaRate) *
						         actor->GetActorValue(RE::ActorValue::kMagickaRateMult) / 10000.0f;
					} else if (type == ActorValues_Stamina) {
						result = actor->GetPermanentActorValue(RE::ActorValue::kStamina) *
						         actor->GetActorValue(RE::ActorValue::kStaminaRate) *
						         actor->GetActorValue(RE::ActorValue::kStaminaRateMult) / 10000.0f;
					}

					result *= std::chrono::duration_cast<std::chrono::milliseconds>(
					              floatingDmg->t_list.back() - floatingDmg->t_list.front())
					              .count() /
					          1000.0f;

					if (result > 0.0f)
						longHeal[type] += result;
				}
			}
			if (longHeal[type] <= -0.5f)
				SetDamage(actor, type + 10, static_cast<std::uint32_t>(-(longHeal[type] - 0.5f)));

			longHeal[type] = 0.0f;
			longHealTime[type] = 0;
			if (type == ActorValues_Health)
				combatCount = 0;
		}
	}

	preDamage[type][1] = preDamage[type][0];
	preDamage[type][0] = damage;
}

void ActorData::SetDamage(RE::Actor* actor, int flag, std::uint32_t damage)
{
	switch (flag) {
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

	if (!actor || actor->Get3D() == nullptr) {
		logger::info("SetDamage: actor null or no 3D, flag={}", flag);
		return;
	}

	auto* floatingDmg = FloatingDamage::GetSingleton();
	if (!floatingDmg || !floatingDmg->hudMovie || !floatingDmg->hudInjectionReady) {
		logger::info("SetDamage: no singleton or HUD injection not ready");
		return;
	}

	auto* player = RE::PlayerCharacter::GetSingleton();
	bool isPlayer = (actor == player);

	// Only show damage for the player or the player's current target
	if (!isPlayer && floatingDmg->lastPlayerHitTarget != actor->GetFormID())
		return;

	// Compute where the player is currently looking on the HUD panel.
	// The HUD panel's ROTATION lags behind head movement; this compensates
	// by projecting the camera's look direction onto the panel's local axes.
	float viewCenterX = 0.5f;
	float viewCenterY = 0.5f;

	auto* camera = RE::Main::WorldRootCamera();
	if (camera) {
		auto* ui = RE::UI::GetSingleton();
		auto hudMenu = ui ? ui->GetMenu("HUD Menu") : nullptr;
		if (hudMenu) {
			auto* wsMenu = static_cast<RE::WorldSpaceMenu*>(hudMenu.get());
			auto* menuNode = wsMenu->menuNode.get();
			if (menuNode) {
				// Camera forward: col2 (DirectX convention)
				auto& cRot = camera->world.rotate;
				RE::NiPoint3 camFwd = { cRot.entry[0][2], cRot.entry[1][2], cRot.entry[2][2] };

				// Panel's local axes from its world rotation
				auto& pRot = menuNode->world.rotate;
				RE::NiPoint3 panelRight = { pRot.entry[0][0], pRot.entry[1][0], pRot.entry[2][0] };
				RE::NiPoint3 panelUp    = { pRot.entry[0][1], pRot.entry[1][1], pRot.entry[2][1] };
				RE::NiPoint3 panelFwd   = { pRot.entry[0][2], pRot.entry[1][2], pRot.entry[2][2] };

				// Project camera look direction onto panel axes.
				// This tells us where on the panel the camera is aimed.
				float rightProj = camFwd.x * panelRight.x + camFwd.y * panelRight.y + camFwd.z * panelRight.z;
				float upProj    = camFwd.x * panelUp.x    + camFwd.y * panelUp.y    + camFwd.z * panelUp.z;
				float fwdProj   = camFwd.x * panelFwd.x   + camFwd.y * panelFwd.y   + camFwd.z * panelFwd.z;

				float absFwd = std::fabs(fwdProj);
				if (absFwd > 0.1f) {
					float pth = floatingDmg->panelTanHalf;
					if (pth <= 0.0f) pth = 1.4f;

					viewCenterX = 0.5f + rightProj / (absFwd * 2.0f * pth);
					viewCenterY = 0.5f - upProj / (absFwd * 2.0f * pth);
				}
			}
		}
	}

	// Position relative to where we're looking: right=dealt, left=received
	RE::NiPoint3 screenPos;
	if (isPlayer) {
		screenPos.x = viewCenterX - 0.2f;
	} else {
		screenPos.x = viewCenterX + 0.2f;
	}
	screenPos.y = viewCenterY - 0.15f;
	screenPos.z = 0.5f;

	// Clamp to HUD bounds
	screenPos.x = std::clamp(screenPos.x, 0.05f, 0.95f);
	screenPos.y = std::clamp(screenPos.y, 0.05f, 0.95f);

	double scale = 100.0;

	std::vector<std::uint32_t> colors;
	std::vector<std::string> texts;
	std::vector<int> flags;
	flags.push_back(flag);
	bool flag_Sneak = false;
	bool flag_Critical = false;

	switch (flag) {
	case -4:
		screenPos.x += static_cast<float>(Random() * scale / 100);
		screenPos.y += static_cast<float>(Random() * scale / 100);
		colors.push_back(ini.Critical);
		texts.push_back(ini.CriticalString);
		break;
	case -3:
		screenPos.x += static_cast<float>(Random() * scale / 100);
		screenPos.y += static_cast<float>(Random() * scale / 100);
		colors.push_back(ini.Sneak);
		texts.push_back(ini.SneakString);
		break;
	case -2:
		screenPos.x += static_cast<float>(Random() * scale / 100);
		screenPos.y += static_cast<float>(Random() * scale / 100);
		colors.push_back(ini.Block);
		texts.push_back(ini.BlockString);
		break;
	case -1:
		if (isPlayer || actor->IsPlayerTeammate())
			colors.push_back(ini.FollowerTimeDamage);
		else
			colors.push_back(ini.HostileTimeDamage);
		texts.push_back(ini.TimeDamageFront + std::to_string(damage) + ini.TimeDamageBack);
		break;
	case 0:
		screenPos.x += static_cast<float>(Random() * scale / 100);
		screenPos.y += static_cast<float>(Random() * scale / 100);
		if (isPlayer || actor->IsPlayerTeammate())
			colors.push_back(ini.FollowerSingleDamage);
		else
			colors.push_back(ini.HostileSingleDamage);
		texts.push_back(ini.SingleDamageFront + std::to_string(damage) + ini.SingleDamageBack);

		if (!hitEvents.empty()) {
			bool block = false;
			bool sneak = false;
			bool critical = false;

			auto it = hitEvents.begin();
			while (it != hitEvents.end()) {
				bool done = false;
				switch (it->second) {
				case HitTypes_Block:
					if (!block) {
						block = true;
						if (ini.EnableBlock != 2) {
							colors.push_back(ini.Block);
							texts.push_back(ini.BlockString);
							flags.push_back(-5);
						}
						if (ini.EnableBlock != 1 && (!critical || ini.EnableCritical == 1) && (!sneak || ini.EnableSneak == 1)) {
							colors[0] = ini.Critical;
							flags[0] = HitTypes_Block - 10;
						}
						it = hitEvents.erase(it);
						done = true;
					}
					break;
				case HitTypes_Sneak:
					if (!sneak) {
						sneak = true;
						if (ini.EnableSneak != 2) {
							colors.push_back(ini.Sneak);
							texts.push_back(ini.SneakString);
							flags.push_back(-6);
							flag_Sneak = true;
						}
						if (ini.EnableSneak != 1 && (!critical || ini.EnableCritical == 1)) {
							colors[0] = ini.Sneak;
							flags[0] = HitTypes_Sneak - 10;
						}
						it = hitEvents.erase(it);
						done = true;
					}
					break;
				case HitTypes_Critical:
					if (!critical) {
						critical = true;
						if (ini.EnableCritical != 2) {
							colors.push_back(ini.Critical);
							texts.push_back(ini.CriticalString);
							flags.push_back(-7);
							flag_Critical = true;
						}
						if (ini.EnableCritical != 1) {
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

	if (flags.size() >= 3) {
		for (auto& f : flags) {
			if (f == -5) {
				if (flag_Critical) {
					if (flag_Sneak)
						f = -10;
					else
						f = -8;
				} else if (flag_Sneak) {
					f = -9;
				}
			} else if (f == -6) {
				if (flag_Critical)
					f = -11;
			}
		}
	}

	for (std::size_t i = 0; i < flags.size(); i++) {
		RE::GFxValue args[6];
		args[0].SetString(texts[i].c_str());
		args[1].SetNumber(static_cast<double>(screenPos.x));
		args[2].SetNumber(static_cast<double>(screenPos.y));
		args[3].SetNumber(scale);
		args[4].SetNumber(static_cast<double>(colors[i]));
		args[5].SetNumber(static_cast<double>(flags[i]));

		bool invokeResult = floatingDmg->hudMovie->Invoke("_root.fdClip.widget.SetDamageText", nullptr, args, 6);
		logger::info("Invoke SetDamageText: result={} text='{}' pos=({},{}) scale={} color={} flag={}",
			invokeResult, texts[i], screenPos.x, screenPos.y, scale, colors[i], flags[i]);
	}
}
