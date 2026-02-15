#pragma once

#include <list>

struct ActorData
{
	bool operator<(const ActorData& rhs) const { return formId < rhs.formId; }

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

	ActorData(RE::FormID formid);

	bool Update();
	void CheckActorValue(RE::Actor* actor, ActorValues type, float current);
	void SetDamage(RE::Actor* actor, int flag, std::uint32_t damage = 0);

	RE::FormID formId;
	float preValue[ActorValues_NumValues];

	float preDamage[ActorValues_NumValues][3];
	float longDamage[ActorValues_NumValues];
	float longHeal[ActorValues_NumValues];
	std::uint32_t longDamageTime[ActorValues_NumValues];
	std::uint32_t longHealTime[ActorValues_NumValues];
	bool overDamage[ActorValues_NumValues];
	bool overHeal[ActorValues_NumValues];

	std::uint32_t combatCount;
	std::uint32_t removeTime;

	typedef std::pair<std::uint32_t, HitTypes> Pair;
	std::list<Pair> hitEvents;
};
