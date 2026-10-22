#include "Data/NOARDataAssets.h"

UNOARRoomData* UNOARApartmentData::FindRoom(FName RoomID) const
{
	for (UNOARRoomData* R : Rooms)
	{
		if (R && R->RoomID == RoomID)
		{
			return R;
		}
	}
	return nullptr;
}

UNOARLightPreset* UNOARApartmentData::FindPreset(ENOARLightingPreset Preset) const
{
	for (UNOARLightPreset* P : LightPresets)
	{
		if (P && P->Preset == Preset)
		{
			return P;
		}
	}
	return nullptr;
}
