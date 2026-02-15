#pragma once

#include <skse64/GameReferences.h>

#include <list>

struct ActorData
{
	bool operator<(const ActorData & rhs) const { return formId < rhs.formId; }

	enum ActorValues
	{
		ActorValues_Health,
		ActorValues_Magicka,
		ActorValues_Stamina,
		ActorValues_NumValues
	};

	enum HitTypes
	{
		HitTypes_Block = -2,
		HitTypes_Sneak = -3,
		HitTypes_Critical = -4
	};

	ActorData(UInt32 formid);

	bool Update();
	void CheckActorValue(Actor* actor, ActorValues type, float current);
	void SetDamage(Actor* actor, int flag, UInt32 damage = 0);

	UInt32 formId;
	float preValue[ActorValues_NumValues];

	float preDamage[ActorValues_NumValues][3];
	float longDamage[ActorValues_NumValues];
	float longHeal[ActorValues_NumValues];
	UInt32 longDamageTime[ActorValues_NumValues];
	UInt32 longHealTime[ActorValues_NumValues];
	bool overDamage[ActorValues_NumValues];
	bool overHeal[ActorValues_NumValues];

	UInt32 combatCount;
	UInt32 removeTime;

	typedef std::pair<UInt32, HitTypes> Pair;
	std::list<Pair> hitEvents;
};