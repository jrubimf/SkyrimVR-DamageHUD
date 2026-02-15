#pragma once

#include "ActorData.h"

#include <chrono>
#include <set>
#include <unordered_set>

class FloatingDamage :
	public RE::IMenu,
	public RE::BSTEventSink<RE::TESHitEvent>,
	public RE::BSTEventSink<RE::CriticalHit::Event>
{
protected:
	static FloatingDamage* ms_pSingleton;
	static RE::BSSpinLock ms_lock;

	std::set<ActorData> m_list;

public:
	FloatingDamage();
	virtual ~FloatingDamage();

	static bool Register();

	static FloatingDamage* GetSingleton()
	{
		return ms_pSingleton;
	}

	std::list<std::chrono::system_clock::time_point> t_list;

	void UpdateActorList();

	void SetBlock(RE::FormID formId);
	void SetSneak(RE::FormID formId);
	void SetCritical(RE::FormID formId);

	// Track last player hit target for correlating CriticalHit::Event
	RE::FormID lastPlayerHitTarget{ 0 };

	// HUD injection: our SWF is loaded into HUDMenu's movie
	RE::GPtr<RE::GFxMovieView> hudMovie;
	bool hudInjectionReady{ false };
	bool hudSettingsSent{ false };

	// VR panel geometry: angular half-size of HUD panel (tan of half-angle)
	float panelTanHalf{ 0.0f };
	bool vrPanelReady{ false };

	// @override IMenu
	virtual RE::UI_MESSAGE_RESULTS ProcessMessage(RE::UIMessage& a_message) override;
	virtual void AdvanceMovie(float a_interval, std::uint32_t a_currentTime) override;

	// @override BSTEventSink<TESHitEvent>
	virtual RE::BSEventNotifyControl ProcessEvent(const RE::TESHitEvent* a_event, RE::BSTEventSource<RE::TESHitEvent>* a_eventSource) override;

	// @override BSTEventSink<CriticalHit::Event>
	virtual RE::BSEventNotifyControl ProcessEvent(const RE::CriticalHit::Event* a_event, RE::BSTEventSource<RE::CriticalHit::Event>* a_eventSource) override;

protected:
	void OnMenuOpen();
	void OnMenuClose();
	void UpdateActorData();
	void GetNearActors(RE::TESObjectCELL* cell, std::unordered_set<RE::FormID>* far_list, std::set<RE::FormID>* near_list);
	void InitVRPanel();

	bool list_reset;
	std::uint32_t updateCount;
};
