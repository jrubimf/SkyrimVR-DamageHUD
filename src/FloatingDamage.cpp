#include "FloatingDamage.h"
#include "ActorData.h"
#include "Settings.h"

#include <unordered_set>

#include <openvr.h>

class AutoOpenFloatingDamageSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* a_event,
		RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
	{
		auto* ui = RE::UI::GetSingleton();

		if (a_event->menuName == RE::LoadingMenu::MENU_NAME && !a_event->opening &&
			!ui->IsMenuOpen(RE::MainMenu::MENU_NAME)) {
			if (!FloatingDamage::GetSingleton()) {
				auto* msgQueue = RE::UIMessageQueue::GetSingleton();
				if (msgQueue) {
					msgQueue->AddMessage("Floating Damage", RE::UI_MESSAGE_TYPE::kShow, nullptr);
				}
			}

			ui->RemoveEventSink<RE::MenuOpenCloseEvent>(this);
			delete this;
		}

		return RE::BSEventNotifyControl::kContinue;
	}

	static void Register()
	{
		auto* ui = RE::UI::GetSingleton();
		if (ui) {
			ui->AddEventSink<RE::MenuOpenCloseEvent>(new AutoOpenFloatingDamageSink());
		}
	}
};

FloatingDamage* FloatingDamage::ms_pSingleton = nullptr;
RE::BSSpinLock FloatingDamage::ms_lock;

FloatingDamage::FloatingDamage()
	: list_reset(false)
	, updateCount(0)
{
	// Lightweight IMenu — no SWF loaded here.
	// Our SWF is injected into HUDMenu's movie in OnMenuOpen.
	menuFlags.set(
		RE::UI_MENU_FLAGS::kAlwaysOpen,
		RE::UI_MENU_FLAGS::kRequiresUpdate,
		RE::UI_MENU_FLAGS::kAllowSaving);
	depthPriority = 0;

	logger::info("FloatingDamage menu created (HUD injection mode)");
}

FloatingDamage::~FloatingDamage()
{
}

bool FloatingDamage::Register()
{
	auto* ui = RE::UI::GetSingleton();
	if (!ui)
		return false;

	ui->Register("Floating Damage", []() -> RE::IMenu* { return new FloatingDamage(); });

	AutoOpenFloatingDamageSink::Register();

	return true;
}

RE::UI_MESSAGE_RESULTS FloatingDamage::ProcessMessage(RE::UIMessage& a_message)
{
	using Type = RE::UI_MESSAGE_TYPE;

	auto result = RE::IMenu::ProcessMessage(a_message);

	switch (*a_message.type) {
	case Type::kShow:
		OnMenuOpen();
		break;
	case Type::kHide:
		OnMenuClose();
		break;
	default:
		break;
	}

	return result;
}

void FloatingDamage::AdvanceMovie(float a_interval, std::uint32_t a_currentTime)
{
	// Safety: verify HUDMenu still exists (gets destroyed on save/load)
	if (hudMovie) {
		auto* ui = RE::UI::GetSingleton();
		if (!ui || !ui->IsMenuOpen("HUD Menu")) {
			logger::info("HUDMenu gone — resetting injection state");
			hudMovie.reset();
			hudInjectionReady = false;
			hudSettingsSent = false;
			vrPanelReady = false;
		}
	}

	// Re-inject if HUDMenu reappeared after save/load
	if (!hudMovie && ms_pSingleton) {
		auto* ui = RE::UI::GetSingleton();
		if (ui && ui->IsMenuOpen("HUD Menu")) {
			auto hudMenu = ui->GetMenu("HUD Menu");
			if (hudMenu && hudMenu->uiMovie) {
				hudMovie.reset(hudMenu->uiMovie.get());

				RE::GFxValue root;
				hudMovie->GetVariable(&root, "_root");
				if (root.IsDisplayObject()) {
					RE::GFxValue container;
					root.CreateEmptyMovieClip(&container, "fdClip", 99999);
					if (container.IsDisplayObject()) {
						RE::GFxValue loadArg;
						loadArg.SetString("FloatingDamage.swf");
						container.Invoke("loadMovie", nullptr, &loadArg, 1);
						logger::info("HUD re-injection after save/load");
					}
				}
				hudInjectionReady = false;
				hudSettingsSent = false;
				vrPanelReady = false;
			}
		}
	}

	// Poll for HUD injection completion
	if (hudMovie && !hudInjectionReady) {
		RE::GFxValue widget;
		hudMovie->GetVariable(&widget, "_root.fdClip.widget");
		if (widget.IsObject()) {
			hudInjectionReady = true;

			// Our DamageWidget.as constructor sets Stage.align="TL" and
			// Stage.scaleMode="noScale" which are GLOBAL and break the HUD layout.
			// Restore the HUD's original settings.
			RE::GFxValue scaleMode;
			scaleMode.SetString("showAll");
			hudMovie->SetVariable("Stage.scaleMode", scaleMode, RE::GFxMovie::SetVarType::kNormal);

			RE::GFxValue align;
			align.SetString("");
			hudMovie->SetVariable("Stage.align", align, RE::GFxMovie::SetVarType::kNormal);

			// Log Stage dimensions for coordinate debugging
			RE::GFxValue stageW, stageH;
			hudMovie->GetVariable(&stageW, "Stage.width");
			hudMovie->GetVariable(&stageH, "Stage.height");
			logger::info("HUD injection ready - widget found, Stage: {}x{}",
				stageW.IsNumber() ? stageW.GetNumber() : -1,
				stageH.IsNumber() ? stageH.GetNumber() : -1);
		}
	}

	// Send settings once widget is available
	if (hudInjectionReady && !hudSettingsSent) {
		RE::GFxValue args[8];
		args[0].SetString(ini.Name.c_str());
		args[1].SetNumber(static_cast<double>(ini.Size));
		args[2].SetNumber(static_cast<double>(ini.Alpha));
		args[3].SetNumber(static_cast<double>(ini.BlockSize));
		args[4].SetNumber(static_cast<double>(ini.SneakSize));
		args[5].SetNumber(static_cast<double>(ini.CriticalSize));
		args[6].SetNumber(static_cast<double>(ini.DamageDisplayMode));
		args[7].SetNumber(static_cast<double>(ini.HealDisplayMode));

		bool result = hudMovie->Invoke("_root.fdClip.widget.SetSettings", nullptr, args, 8);
		logger::info("HUD injection SetSettings result: {}", result);
		if (result) {
			hudSettingsSent = true;
		}
	}

	// Initialize VR panel geometry once HUD injection is ready
	if (hudInjectionReady && !vrPanelReady) {
		InitVRPanel();
	}

	UpdateActorData();

	if (++updateCount > ini.ListUpdateTime) {
		updateCount = 0;

		if (ms_pSingleton) {
			list_reset = false;

			SKSE::GetTaskInterface()->AddUITask([]() {
				auto* fd = FloatingDamage::GetSingleton();
				if (fd)
					fd->UpdateActorList();
			});
		}
	}

	// Don't call IMenu::AdvanceMovie — we have no uiMovie to advance
}

void FloatingDamage::OnMenuOpen()
{
	RE::BSSpinLockGuard guard(ms_lock);

	logger::info("OnMenuOpen called, singleton={}", ms_pSingleton == nullptr ? "null" : "exists");

	if (!ms_pSingleton) {
		ms_pSingleton = this;

		// Register event sinks
		auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		if (eventSourceHolder) {
			eventSourceHolder->AddEventSink<RE::TESHitEvent>(this);
		}

		if (ini.EnableCritical != 0) {
			auto* critSource = RE::CriticalHit::GetEventSource();
			if (critSource) {
				critSource->AddEventSink(static_cast<RE::BSTEventSink<RE::CriticalHit::Event>*>(this));
			}
		}

		// Inject our SWF into HUDMenu's Scaleform movie
		auto* ui = RE::UI::GetSingleton();
		if (ui) {
			auto hudMenu = ui->GetMenu("HUD Menu");
			if (hudMenu && hudMenu->uiMovie) {
				hudMovie.reset(hudMenu->uiMovie.get());

				RE::GFxValue root;
				hudMovie->GetVariable(&root, "_root");

				if (root.IsDisplayObject()) {
					// Create a container clip in the HUD and load our SWF into it
					RE::GFxValue container;
					root.CreateEmptyMovieClip(&container, "fdClip", 99999);

					if (container.IsDisplayObject()) {
						RE::GFxValue loadArg;
						loadArg.SetString("FloatingDamage.swf");
						container.Invoke("loadMovie", nullptr, &loadArg, 1);

						logger::info("HUD injection: loadMovie called on fdClip");
					} else {
						logger::error("HUD injection: failed to create fdClip");
					}
				} else {
					logger::error("HUD injection: failed to get _root");
				}
			} else {
				logger::error("HUD injection: HUDMenu not available");
			}
		}

		hudInjectionReady = false;
		hudSettingsSent = false;
	}
}

void FloatingDamage::OnMenuClose()
{
	RE::BSSpinLockGuard guard(ms_lock);

	if (ms_pSingleton) {
		ms_pSingleton = nullptr;
		m_list.clear();

		auto* eventSourceHolder = RE::ScriptEventSourceHolder::GetSingleton();
		if (eventSourceHolder) {
			eventSourceHolder->RemoveEventSink<RE::TESHitEvent>(this);
		}

		if (ini.EnableCritical != 0) {
			auto* critSource = RE::CriticalHit::GetEventSource();
			if (critSource) {
				critSource->RemoveEventSink(static_cast<RE::BSTEventSink<RE::CriticalHit::Event>*>(this));
			}
		}

		// Clean up our clip from the HUD movie
		if (hudMovie) {
			RE::GFxValue root;
			hudMovie->GetVariable(&root, "_root");
			if (root.IsDisplayObject()) {
				RE::GFxValue fdClip;
				root.GetMember("fdClip", &fdClip);
				if (fdClip.IsDisplayObject()) {
					fdClip.Invoke("removeMovieClip");
					logger::info("HUD injection: removed fdClip");
				}
			}
			hudMovie.reset();
		}

		hudInjectionReady = false;
		hudSettingsSent = false;
		vrPanelReady = false;
		list_reset = true;
	}

	AutoOpenFloatingDamageSink::Register();
}

RE::BSEventNotifyControl FloatingDamage::ProcessEvent(
	const RE::TESHitEvent* a_event,
	RE::BSTEventSource<RE::TESHitEvent>*)
{
	if (!a_event || !a_event->target)
		return RE::BSEventNotifyControl::kContinue;

	auto* target = a_event->target.get();
	if (!target)
		return RE::BSEventNotifyControl::kContinue;

	// Track last player hit target for critical hit correlation
	auto* player = RE::PlayerCharacter::GetSingleton();
	if (a_event->cause.get() == player) {
		lastPlayerHitTarget = target->GetFormID();
	}

	if (a_event->flags.any(RE::TESHitEvent::Flag::kHitBlocked) && ini.EnableBlock > 0)
		SetBlock(target->GetFormID());

	if (a_event->flags.any(RE::TESHitEvent::Flag::kSneakAttack) && ini.EnableSneak > 0)
		SetSneak(target->GetFormID());

	return RE::BSEventNotifyControl::kContinue;
}

RE::BSEventNotifyControl FloatingDamage::ProcessEvent(
	const RE::CriticalHit::Event* a_event,
	RE::BSTEventSource<RE::CriticalHit::Event>*)
{
	if (!a_event || !a_event->aggressor)
		return RE::BSEventNotifyControl::kContinue;

	auto* player = RE::PlayerCharacter::GetSingleton();

	if (a_event->aggressor == player) {
		// Player dealt a critical hit - use last known hit target
		if (lastPlayerHitTarget != 0) {
			SetCritical(lastPlayerHitTarget);
		}
	} else {
		// Someone else dealt a critical hit to... we assume the player if they're in combat
		// (In vanilla Skyrim, only the player deals critical hits via perks)
		SetCritical(player->GetFormID());
	}

	return RE::BSEventNotifyControl::kContinue;
}

void FloatingDamage::SetBlock(RE::FormID formId)
{
	ActorData actorData(formId);

	RE::BSSpinLockGuard guard(ms_lock);

	auto it = m_list.find(actorData);
	if (it != m_list.end())
		(const_cast<ActorData*>(&(*it)))->hitEvents.push_back(std::make_pair(4u, ActorData::HitTypes_Block));
}

void FloatingDamage::SetSneak(RE::FormID formId)
{
	ActorData actorData(formId);

	RE::BSSpinLockGuard guard(ms_lock);

	auto it = m_list.find(actorData);
	if (it != m_list.end())
		(const_cast<ActorData*>(&(*it)))->hitEvents.push_back(std::make_pair(4u, ActorData::HitTypes_Sneak));
}

void FloatingDamage::SetCritical(RE::FormID formId)
{
	ActorData actorData(formId);

	RE::BSSpinLockGuard guard(ms_lock);

	auto it = m_list.find(actorData);
	if (it != m_list.end())
		(const_cast<ActorData*>(&(*it)))->hitEvents.push_back(std::make_pair(4u, ActorData::HitTypes_Critical));
}

void FloatingDamage::InitVRPanel()
{
	auto* ui = RE::UI::GetSingleton();
	if (!ui)
		return;

	auto hudMenu = ui->GetMenu("HUD Menu");
	if (!hudMenu)
		return;

	auto* wsMenu = static_cast<RE::WorldSpaceMenu*>(hudMenu.get());
	auto* menuNode = wsMenu->menuNode.get();
	if (!menuNode) {
		logger::info("VR panel init: menuNode is null");
		return;
	}

	// Find the actual panel mesh (first child) to get its size.
	// Using first child avoids the parent menuNode's worldBound which
	// includes dynamically-added HUD children and is unstable.
	float panelRadius = 0.0f;
	for (std::uint16_t i = 0; i < menuNode->children.capacity(); i++) {
		auto* child = menuNode->children[i].get();
		if (child) {
			logger::info("VR panel child[{}]: name='{}' radius={:.3f}",
				i, child->name.c_str(), child->worldBound.radius);
			if (panelRadius == 0.0f)
				panelRadius = child->worldBound.radius;
		}
	}

	if (panelRadius == 0.0f)
		panelRadius = menuNode->worldBound.radius;

	// Panel is square (1024x1024 Stage). Bounding sphere radius = halfDiag = halfSide * sqrt(2).
	float panelHalfSize = panelRadius * 0.707f;

	// Forward distance from headset to panel (menuNode local Y = forward in headset frame)
	float forwardDist = std::fabs(menuNode->local.translate.y);
	if (forwardDist < 1.0f)
		forwardDist = 15.0f;  // fallback

	// Angular half-size: tan(halfAngle) = panelHalfSize / forwardDist
	panelTanHalf = panelHalfSize / forwardDist;

	logger::info("VR panel init: panelRadius={:.3f} panelHalfSize={:.3f} forwardDist={:.3f} panelTanHalf={:.3f}",
		panelRadius, panelHalfSize, forwardDist, panelTanHalf);

	if (panelTanHalf > 0.1f) {
		vrPanelReady = true;
		logger::info("VR panel ready");
	}
}

void FloatingDamage::UpdateActorData()
{
	RE::BSSpinLockGuard guard(ms_lock);

	if (!ms_pSingleton)
		return;

	if (ini.HideRegen) {
		t_list.push_back(std::chrono::system_clock::now());
		while (t_list.size() > ini.TimeDamageInterval) {
			t_list.pop_front();
		}
	}

	auto it = m_list.begin();
	while (it != m_list.end()) {
		if ((const_cast<ActorData*>(&(*it)))->Update())
			m_list.erase(it++);
		else
			it++;
	}
}

void FloatingDamage::UpdateActorList()
{
	std::unordered_set<RE::FormID> far_list;
	std::set<RE::FormID> near_list;

	auto* player = RE::PlayerCharacter::GetSingleton();
	auto* currentCell = player->GetParentCell();
	if (!currentCell)
		return;

	if (currentCell->IsInteriorCell()) {
		GetNearActors(currentCell, &far_list, &near_list);
	} else {
		auto* tes = RE::TES::GetSingleton();
		if (tes) {
			tes->ForEachCell([&](RE::TESObjectCELL* cell) {
				GetNearActors(cell, &far_list, &near_list);
			});
		}
	}

	// Add hostile targets from player's combat group
	auto* combatGroup = player->GetCombatGroup();
	if (combatGroup) {
		for (auto& target : combatGroup->targets) {
			auto targetRef = target.targetHandle.get();
			if (targetRef) {
				far_list.insert(targetRef->GetFormID());
				near_list.insert(targetRef->GetFormID());
			}
		}
	}

	RE::BSSpinLockGuard guard(ms_lock);

	if (list_reset)
		return;

	auto it = m_list.begin();
	while (it != m_list.end()) {
		auto it2 = far_list.find((const_cast<ActorData*>(&(*it)))->formId);
		if (it2 == far_list.end())
			m_list.erase(it++);
		else
			it++;
	}

	logger::info("UpdateActorList: near={} far={}", near_list.size(), far_list.size());

	for (auto& formid : near_list) {
		ActorData actorData(formid);

		auto it2 = m_list.find(actorData);
		if (it2 == m_list.end()) {
			auto* form = RE::TESForm::LookupByID(formid);
			if (form) {
				auto* actor = form->As<RE::Actor>();
				if (actor) {
						actorData.preValue[ActorData::ActorValues_Health] = actor->GetActorValue(RE::ActorValue::kHealth);
					actorData.preValue[ActorData::ActorValues_Magicka] = actor->GetActorValue(RE::ActorValue::kMagicka);
					actorData.preValue[ActorData::ActorValues_Stamina] = actor->GetActorValue(RE::ActorValue::kStamina);
					m_list.insert(actorData);
				}
			}
		}
	}
}

void FloatingDamage::GetNearActors(RE::TESObjectCELL* cell, std::unordered_set<RE::FormID>* far_list, std::set<RE::FormID>* near_list)
{
	if (!cell || !cell->IsAttached())
		return;

	auto* player = RE::PlayerCharacter::GetSingleton();
	auto playerPos = player->GetPosition();
	double farRadius = ini.FarRadius;
	double nearRadius = ini.NearRadius;
	bool onlyHostile = ini.OnlyHostile;

	cell->ForEachReference([&](RE::TESObjectREFR* ref) -> RE::BSContainer::ForEachResult {
		auto* actor = ref->As<RE::Actor>();
		if (!actor)
			return RE::BSContainer::ForEachResult::kContinue;

		if (actor->IsInCombat() && (!onlyHostile || player->IsHostileToActor(actor))) {
			far_list->insert(actor->GetFormID());
			near_list->insert(actor->GetFormID());
		} else {
			auto actorPos = actor->GetPosition();
			double dx = actorPos.x - playerPos.x;
			double dy = actorPos.y - playerPos.y;
			double dz = actorPos.z - playerPos.z;
			double d2 = dx * dx + dy * dy + dz * dz;
			if (d2 < farRadius) {
				far_list->insert(actor->GetFormID());

				if (d2 < nearRadius && !actor->IsDead())
					near_list->insert(actor->GetFormID());
			}
		}

		return RE::BSContainer::ForEachResult::kContinue;
	});
}
