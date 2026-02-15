#pragma once
#include "skse64/GameObjects.h"

class GridArray
{
public:
	virtual ~GridArray();

	virtual void Unk_01(void);
	virtual void Unk_02(void);
	virtual void Unk_03(void);
	virtual void Unk_04(void);
	virtual void Unk_05(void);
	virtual void Unk_06(void) = 0;
	virtual void Unk_07(void) = 0;
	virtual void Unk_08(void) = 0;
	virtual void Unk_09(void) = 0;
};

class GridCellArray : public GridArray
{
public:
	virtual ~GridCellArray();

	virtual void Unk_03(void) override;
	virtual void Unk_04(void) override;
	virtual void Unk_05(void) override;
	virtual void Unk_06(void) override;
	virtual void Unk_07(void) override;
	virtual void Unk_08(void) override;
	virtual void Unk_09(void) override;
	virtual void Unk_0A(void);

	UInt32	unk04;
	UInt32	unk08;
	UInt32	size;
	TESObjectCELL**	cells;
};

class TES
{
public:
	virtual ~TES();

	UInt32  unk04;  //04
	UInt32  unk08;  //08
	UInt32  unk0C;  //0C
	UInt32  unk10;  //10
	UInt32  unk14;  //14
	UInt32  unk18;  //18
	UInt32  unk1C;  //1C
	UInt32  unk20;  //20
	UInt32  unk24;  //24
	UInt32  unk28;  //28
	UInt32  unk2C;  //2C
	UInt32  unk30;  //30
	UInt32  unk34;  //34
	UInt32  unk38;  //38
	UInt32  unk3C;  //3C
	UInt32  unk40;  //40
	UInt32  unk44;  //44
	UInt32  unk48;  //48
	UInt32  unk4C;  //4C
	UInt32  unk50;  //50
	UInt32  unk54;  //54
	UInt32  unk58;  //58
	UInt32  unk5C;  //5C
	UInt32  unk60;  //60
	UInt32  unk64;  //64
	UInt32  unk68;  //68
	UInt32  unk6C;  //6C
	UInt32  unk70;  //70 
	GridCellArray* cellArray; // 74
							  // ... 
};