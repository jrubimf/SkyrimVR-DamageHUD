#pragma once

#include <skse64/GameMenus.h>
#include <skse64/GameEvents.h>
#include <skse64/Hooks_UI.h>
#include <skse64/GameTypes.h>

#include <set>
#include <unordered_set>
#include <chrono>

struct ActorData;

class FloatingDamage : public IMenu,
	public BSTEventSink<TESHitEvent>
{
protected:
	static FloatingDamage * ms_pSingleton;
	static SimpleLock ms_lock;

	std::set<ActorData> m_list;

public:
	FloatingDamage();
	virtual ~FloatingDamage();

	static bool Register();

	static FloatingDamage * GetSingleton()
	{
		return ms_pSingleton;
	}

	std::list<std::chrono::system_clock::time_point> t_list;

	void UpdateActorList();

	void SetBlock(UInt32 formId);
	void SetSneak(UInt32 formId);
	void SetCritical(UInt32 formId);

	// @override IMenu
	virtual UInt32	ProcessUnkData1(UnkData1* data) override;
	virtual void	Render() override;

	// @override BSTEventSink
	virtual EventResult ReceiveEvent(TESHitEvent *evn, EventDispatcher<TESHitEvent> *dispatcher) override;

protected:
	void OnMenuOpen();
	void OnMenuClose();
	void UpdateActorData();
	void GetNearActors(TESObjectCELL* cell, std::unordered_set<UInt32> *far_list, std::set<UInt32> *near_list);

	bool list_reset;
	UInt32 updateCount;
};

class UpdateActorListTask : public UIDelegate_v1
{
public:
	UpdateActorListTask::UpdateActorListTask() {}
	virtual void Run();
	virtual void Dispose()
	{
		delete this;
	}
};

class CriticalEventTask : public UIDelegate_v1
{
public:
	CriticalEventTask::CriticalEventTask(UInt32 formId) : m_formId(formId) {}
	virtual void Run();
	virtual void Dispose()
	{
		delete this;
	}

private:
	UInt32			m_formId;
};