#include "FloatingDamage.h"
#include "ActorData.h"
#include "iniSettings.h"
#include "tes.h"

#include <skse64/PluginAPI.h>
#include "skse64/GameRTTI.h"
#include <skse64/GameForms.h>
#include <skse64/GameReferences.h>
#include <skse64/GameData.h>
#include <skse64/GameObjects.h>
#include <skse64/ScaleformLoader.h>

RelocPtr <TES*> g_TES(0x02F4DB20);   //1.5.39
//RelocPtr <TES*> g_TES(0x02F4CAA0); //1.5.23
//RelocPtr <TES*> g_TES(0x02F4CAA0); //1.5.16

class AutoOpenFloatingDamageSink : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	EventResult ReceiveEvent(MenuOpenCloseEvent *evn, EventDispatcher<MenuOpenCloseEvent> * dispatcher)
	{
		MenuManager *mm = MenuManager::GetSingleton();
		UIStringHolder *holder = UIStringHolder::GetSingleton();

		if (evn->menuName == holder->loadingMenu && !evn->opening && !mm->IsMenuOpen(&holder->mainMenu))
		{
			FloatingDamage *FloatingDamage = FloatingDamage::GetSingleton();
			if (!FloatingDamage)
			{
				UIManager *ui = UIManager::GetSingleton();
				StringCache::Ref	menuNameRef;
				CALL_MEMBER_FN(&menuNameRef, ctor)("Floating Damage");
				CALL_MEMBER_FN(ui, AddMessage)(&menuNameRef, UIMessage::kMessage_Open, nullptr);
			}

			mm->MenuOpenCloseEventDispatcher()->RemoveEventSink(this);
			delete this;
		}

		return kEvent_Continue;
	}

	static void Register()
	{
		MenuManager *mm = MenuManager::GetSingleton();
		if (mm)
		{
			AutoOpenFloatingDamageSink *sink = new AutoOpenFloatingDamageSink();
			if (sink)
			{
				mm->MenuOpenCloseEventDispatcher()->AddEventSink(sink);
			}
		}
	}
};


FloatingDamage * FloatingDamage::ms_pSingleton = nullptr;
SimpleLock FloatingDamage::ms_lock;
SKSETaskInterface		* g_task = NULL;

FloatingDamage::FloatingDamage() : list_reset(false), updateCount(0)
{
	const char swfName[] = "FloatingDamage";

	if (CALL_MEMBER_FN(GFxLoader::GetSingleton(), LoadMovie)(this, &view, swfName, 0, 0.0))
	{
		_MESSAGE("loaded Inteface/%s.swf", swfName);

		unk0C = 2;
		flags = 0x10802;
	}
}


FloatingDamage::~FloatingDamage()
{
}


bool FloatingDamage::Register()
{
	MenuManager *mm = MenuManager::GetSingleton();
	if (!mm)
		return false;

	mm->Register("Floating Damage", []() -> IMenu * { return new FloatingDamage; });

	AutoOpenFloatingDamageSink::Register();

	return true;
}


UInt32 FloatingDamage::ProcessUnkData1(UnkData1* data)
{
	UInt32 result = 2;

	if (view)
	{
		switch (data->unk04)
		{
		case 1:
			OnMenuOpen();
			result = 0;
			break;
		case 3:
			OnMenuClose();
			result = 0;
			break;
		}
	}

	return result;
}


void FloatingDamage::Render()
{
	UpdateActorData();

	if (++updateCount > ini.ListUpdateTime)
	{
		updateCount = 0;

		if (ms_pSingleton)
		{
			list_reset = false;

			g_task->AddUITask(new UpdateActorListTask());
		}
	}

	IMenu::Render();
}


void FloatingDamage::OnMenuOpen()
{
	SimpleLocker guard(&ms_lock);

	if (!ms_pSingleton)
	{
		ms_pSingleton = this;

		if (ini.EnableBlock != 0 || ini.EnableSneak != 0)
		{
			auto eventDispatchers = GetEventDispatcherList();
			eventDispatchers->unk630.AddEventSink(this);
		}

		if (view)
		{
			GFxValue args[8];
			args[0].SetString((ini.Name).c_str());
			args[1].SetNumber((double)ini.Size);
			args[2].SetNumber((double)ini.Alpha);
			args[3].SetNumber((double)ini.BlockSize);
			args[4].SetNumber((double)ini.SneakSize);
			args[5].SetNumber((double)ini.CriticalSize);
			args[6].SetNumber((double)ini.DamageDisplayMode);
			args[7].SetNumber((double)ini.HealDisplayMode);

			view->Invoke("_root.widget.SetSettings", nullptr, args, 8);
		}
	}
}


void FloatingDamage::OnMenuClose()
{
	SimpleLocker guard(&ms_lock);

	if (ms_pSingleton)
	{
		ms_pSingleton = nullptr;
		m_list.clear();

		if (ini.EnableBlock != 0 || ini.EnableSneak != 0)
		{
			auto eventDispatchers = GetEventDispatcherList();
			eventDispatchers->unk630.RemoveEventSink(this);
		}

		list_reset = true;
	}

	AutoOpenFloatingDamageSink::Register();
}


EventResult FloatingDamage::ReceiveEvent(TESHitEvent *evn, EventDispatcher<TESHitEvent> *dispatcher)
{
	if ((evn->flags & TESHitEvent::kFlag_Blocked) != 0 && ini.EnableBlock > 0 && evn->target)
		SetBlock(evn->target->formID);

	if ((evn->flags & TESHitEvent::kFlag_SneakAttack) != 0 && ini.EnableSneak > 0 && evn->target)
		SetSneak(evn->target->formID);

	return kEvent_Continue;
}


void FloatingDamage::SetBlock(UInt32 formId)
{
	ActorData actorData(formId);

	SimpleLocker guard(&ms_lock);

	auto it = m_list.find(actorData);
	if (it != m_list.end())
		(const_cast<ActorData*>(&(*it)))->hitEvents.push_back(std::make_pair(4, ActorData::HitTypes_Block));
}


void FloatingDamage::SetSneak(UInt32 formId)
{
	ActorData actorData(formId);

	SimpleLocker guard(&ms_lock);

	auto it = m_list.find(actorData);
	if (it != m_list.end())
		(const_cast<ActorData*>(&(*it)))->hitEvents.push_back(std::make_pair(4, ActorData::HitTypes_Sneak));
}


void FloatingDamage::SetCritical(UInt32 formId)
{
	ActorData actorData(formId);

	SimpleLocker guard(&ms_lock);

	auto it = m_list.find(actorData);
	if (it != m_list.end())
		(const_cast<ActorData*>(&(*it)))->hitEvents.push_back(std::make_pair(4, ActorData::HitTypes_Critical));
}


void FloatingDamage::UpdateActorData()
{
	SimpleLocker guard(&ms_lock);

	if (!ms_pSingleton)
		return;

	if (ini.HideRegen)
	{
		t_list.push_back(std::chrono::system_clock::now());
		while (t_list.size() > ini.TimeDamageInterval)
		{
			t_list.pop_front();
		}
	}

	auto it = m_list.begin();
	while (it != m_list.end())
	{
		if ((const_cast<ActorData*>(&(*it)))->Update())
			m_list.erase(it++);
		else
			it++;
	}
}


void FloatingDamage::UpdateActorList()
{
	std::unordered_set<UInt32> far_list;
	std::set<UInt32> near_list;

	TESObjectCELL* currentCell = (*g_thePlayer)->parentCell;
	if (!currentCell)
		return;

	if ((currentCell->unk040 & 1) != 0)
	{
		GetNearActors(currentCell, &far_list, &near_list);
	}
	else
	{
		GridCellArray* arr = (*g_TES)->cellArray;
		if (!arr)
			return;
		int x, y;
		for (x = 0; x < arr->size; x++)
		{
			for (y = 0; y < arr->size; y++)
			{
				GetNearActors(arr->cells[x + y*arr->size], &far_list, &near_list);
			}
		}
	}

	if ((*g_thePlayer)->hostileHandles.count != 0)
	{
		tArray<UInt32> handles = (*g_thePlayer)->hostileHandles;

		for (UInt32 i = 0; i < handles.count; i++)
		{
			if (handles.entries[i] != *g_invalidRefHandle)
			{
				TESObjectREFR* ref = nullptr;
				LookupREFRByHandle(&handles.entries[i], &ref);
				if (ref)
				{
					far_list.insert(ref->formID);
					near_list.insert(ref->formID);
				}
			}
		}
	}

	SimpleLocker guard(&ms_lock);

	if (list_reset)
		return;

	auto it = m_list.begin();
	while (it != m_list.end())
	{
		auto it2 = far_list.find((const_cast<ActorData*>(&(*it)))->formId);
		if (it2 == far_list.end())
			m_list.erase(it++);
		else
			it++;
	}

	for (auto& formid : near_list)
	{
		ActorData actorData(formid);

		auto it2 = m_list.find(actorData);
		if (it2 == m_list.end())
		{
			TESForm* form = LookupFormByID(formid);
			if (form)
			{
				Actor* actor = DYNAMIC_CAST(form, TESForm, Actor);
				if (actor)
				{
					actorData.preValue[ActorData::ActorValues_Health] = actor->actorValueOwner.GetCurrent(24);
					actorData.preValue[ActorData::ActorValues_Magicka] = actor->actorValueOwner.GetCurrent(25);
					actorData.preValue[ActorData::ActorValues_Stamina] = actor->actorValueOwner.GetCurrent(26);
					m_list.insert(actorData);
				}
			}
		}
	}
}


void FloatingDamage::GetNearActors(TESObjectCELL* cell, std::unordered_set<UInt32> *far_list, std::set<UInt32> *near_list)
{
	if (!cell || cell->unk044 != 7)
		return;

	float playerPosX = (*g_thePlayer)->pos.x;
	float playerPosY = (*g_thePlayer)->pos.y;
	float playerPosZ = (*g_thePlayer)->pos.z;
	double farRadius = ini.FarRadius;
	double nearRadius = ini.NearRadius;
	bool onlyHostile = ini.OnlyHostile;

	TESObjectCELL::ReferenceData referenceData = cell->refData;

	for (int i = 0; i < referenceData.maxSize; i++)
	{
		if (referenceData.refArray[i].unk08 == NULL)
			continue;
		TESObjectREFR* ref = referenceData.refArray[i].ref;
		if (!ref)
			continue;
		Actor* actor = DYNAMIC_CAST(ref, TESForm, Actor);
		if (!actor)
			continue;

		if (actor->IsInCombat() && (!onlyHostile || CALL_MEMBER_FN(*g_thePlayer, IsHostileToActor)(actor)))
		{
			far_list->insert(actor->formID);
			near_list->insert(actor->formID);
		}
		else
		{
			double dx = actor->pos.x - playerPosX;
			double dy = actor->pos.y - playerPosY;
			double dz = actor->pos.z - playerPosZ;
			double d2 = dx*dx + dy*dy + dz*dz;
			if (d2 < farRadius)
			{
				far_list->insert(actor->formID);

				if (d2 < nearRadius && !actor->IsDead(1))
					near_list->insert(actor->formID);
			}
		}
	}
}


void UpdateActorListTask::Run()
{
	FloatingDamage *FloatingDamage = FloatingDamage::GetSingleton();
	if (FloatingDamage)
		FloatingDamage->UpdateActorList();
}


void CriticalEventTask::Run()
{
	FloatingDamage *FloatingDamage = FloatingDamage::GetSingleton();
	if (FloatingDamage)
		FloatingDamage->SetCritical(m_formId);
}