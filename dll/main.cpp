#include "skse64/PluginAPI.h"
#include "skse64_common/skse_version.h"
#include "skse64_common/BranchTrampoline.h"
#include "skse64_common/SafeWrite.h"
#include "skse64/GameAPI.h"
#include <shlobj.h>

#include "skse64/GameRTTI.h"
#include "skse64/GameReferences.h"
#include "skse64/GameEvents.h"

#include "FloatingDamage.h"
#include "iniSettings.h"

#include <typeinfo>

RelocAddr<uintptr_t> kCriticalHit_Hook(0x00626A9B); //1.5.39
//RelocAddr<uintptr_t> kCriticalHit_Hook(0x0062642B);	//1.5.23
//RelocAddr<uintptr_t> kCriticalHit_Hook(0x006263FB);	//1.5.16

IDebugLog	gLog;
UInt32 g_skseVersion;
PluginHandle	g_pluginHandle = kPluginHandle_Invalid;
SKSEMessagingInterface	* g_messaging = nullptr;
extern SKSETaskInterface		* g_task;

class FreezeEventHandler : public BSTEventSink<MenuOpenCloseEvent>
{
public:
	typedef EventResult(FreezeEventHandler::*FnReceiveEvent)(MenuOpenCloseEvent *evn, EventDispatcher<MenuOpenCloseEvent> *dispatcher);

	static std::unordered_map<UInt64, FnReceiveEvent> ms_handlers;

	UInt64 GetVPtr() const
	{
		return *(UInt64*)this;
	}

	EventResult ReceiveEvent_Hook(MenuOpenCloseEvent *evn, EventDispatcher<MenuOpenCloseEvent> *dispatcher)
	{
		static BSFixedString menuName = "Floating Damage";

		if (evn->menuName == menuName)
		{
			return kEvent_Continue;
		}

		return (ms_handlers.count(GetVPtr()) > 0) ? (this->*(ms_handlers.at(GetVPtr())))(evn, dispatcher) : kEvent_Continue;
	}

	void InstallHook()
	{
		UInt64 vptr = GetVPtr();
		FreezeEventHandler::FnReceiveEvent pFn = SafeWrite64(vptr + 0x08, &FreezeEventHandler::ReceiveEvent_Hook);
		ms_handlers.insert(std::make_pair(vptr, pFn));
	}
};

std::unordered_map<UInt64, FreezeEventHandler::FnReceiveEvent> FreezeEventHandler::ms_handlers;

class MenuOpenCloseEventSource : public EventDispatcher<MenuOpenCloseEvent>
{
public:
	void ProcessHook()
	{
		lock.Lock();

		BSTEventSink<MenuOpenCloseEvent> *sink;
		UInt32 idx = 0;
		while (eventSinks.GetNthItem(idx, sink))
		{
			const type_info& id = typeid(*sink);

			if (strcmp(id.name(), "class hdt::FreezeEventHandler") == 0)
			{
				FreezeEventHandler *freezeEventHandler = static_cast<FreezeEventHandler *>(sink);
				freezeEventHandler->InstallHook();
			}

			++idx;
		}

		lock.Release();
	}

	static void InitHook()
	{
		MenuManager *mm = MenuManager::GetSingleton();
		if (mm)
		{
			MenuOpenCloseEventSource *pThis = static_cast<MenuOpenCloseEventSource*>(mm->MenuOpenCloseEventDispatcher());
			pThis->ProcessHook();
		}
	}
};

class TESObjectREFREx : public TESObjectREFR
{
public:
	const char* CriticalHit_Hook()
	{
		g_task->AddUITask(new CriticalEventTask(formID));

		return CALL_MEMBER_FN(this, GetReferenceName)();
	}

	static void InitHook()
	{
		g_branchTrampoline.Write5Call(kCriticalHit_Hook.GetUIntPtr(), GetFnAddr(&CriticalHit_Hook));
	}
};

void SKSEMessageHandler(SKSEMessagingInterface::Message* msg)
{
	if (msg->type == SKSEMessagingInterface::kMessage_DataLoaded)
	{
		ini.Load();

		if (FloatingDamage::Register())
		{
			MenuOpenCloseEventSource::InitHook();

			if (ini.EnableCritical != 0)
			{
				if (g_branchTrampoline.Create(1024 * 64))
					TESObjectREFREx::InitHook();
				else
					_ERROR("couldn't create branch trampoline. this is fatal. skipping remainder of init process.");
			}
		}
	}
}

extern "C"
{

	bool SKSEPlugin_Query(const SKSEInterface * skse, PluginInfo * info)
	{
		SInt32	logLevel = IDebugLog::kLevel_DebugMessage;
		gLog.SetLogLevel((IDebugLog::LogLevel)logLevel);

		gLog.OpenRelative(CSIDL_MYDOCUMENTS, "\\My Games\\Skyrim Special Edition\\SKSE\\FloatingDamage.log");

		_MESSAGE("FloatingDamage");

		g_skseVersion = skse->skseVersion;

		info->infoVersion = PluginInfo::kInfoVersion;
		info->name = "FloatingDamage";
		info->version = 1;

		g_pluginHandle = skse->GetPluginHandle();

		if (skse->isEditor)
		{
			_MESSAGE("loaded in editor, marking as incompatible");
			return false;
		}
		else if (skse->runtimeVersion != RUNTIME_VERSION_1_5_39)
		{
			_MESSAGE("unsupported runtime version %08X", skse->runtimeVersion);
			return false;
		}

		g_messaging = (SKSEMessagingInterface *)skse->QueryInterface(kInterface_Messaging);
		if (!g_messaging)
		{
			_MESSAGE("couldn't get messaging interface");
			return false;
		}
		if (g_messaging->interfaceVersion < 1)
		{
			_MESSAGE("messaging interface too old (%d expected %d)", g_messaging->interfaceVersion, 1);
			return false;
		}

		g_task = (SKSETaskInterface *)skse->QueryInterface(kInterface_Task);
		if (!g_task)
		{
			_MESSAGE("couldn't get task interface");
			return false;
		}
		if (g_task->interfaceVersion < 2)
		{
			_MESSAGE("task interface too old (%d expected %d)", g_task->interfaceVersion, 1);
			return false;
		}

		return true;
	}

	bool SKSEPlugin_Load(const SKSEInterface * skse)
	{
		_MESSAGE("FloatingDamage Loaded");

		g_messaging->RegisterListener(g_pluginHandle, "SKSE", SKSEMessageHandler);

		return true;
	}

};
