#include "NOARGameplayTags.h"

namespace NOARTags
{
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interact_State_Closed,  "Interact.State.Closed",  "Mechanism fully closed");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interact_State_Opening, "Interact.State.Opening", "Timeline playing forward");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interact_State_Open,    "Interact.State.Open",    "Mechanism fully open");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interact_State_Closing, "Interact.State.Closing", "Timeline reversing");

	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interact_Lock_Locked,     "Interact.Lock.Locked",     "Locked by design");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interact_Lock_Dependency, "Interact.Lock.Dependency", "RequiredOpen[] not satisfied");
	UE_DEFINE_GAMEPLAY_TAG_COMMENT(Interact_Lock_Blocked,    "Interact.Lock.Blocked",    "BlockedBy[]: neighbour open in path");

	UE_DEFINE_GAMEPLAY_TAG(Interact_Type_Door,         "Interact.Type.Door");
	UE_DEFINE_GAMEPLAY_TAG(Interact_Type_Drawer,       "Interact.Type.Drawer");
	UE_DEFINE_GAMEPLAY_TAG(Interact_Type_Flap,         "Interact.Type.Flap");
	UE_DEFINE_GAMEPLAY_TAG(Interact_Type_Slide,        "Interact.Type.Slide");
	UE_DEFINE_GAMEPLAY_TAG(Interact_Type_Pocket,       "Interact.Type.Pocket");
	UE_DEFINE_GAMEPLAY_TAG(Interact_Type_Switch,       "Interact.Type.Switch");
	UE_DEFINE_GAMEPLAY_TAG(Interact_Type_Appliance,    "Interact.Type.Appliance");
	UE_DEFINE_GAMEPLAY_TAG(Interact_Type_Configurable, "Interact.Type.Configurable");

	UE_DEFINE_GAMEPLAY_TAG(Zone,         "Zone");
	UE_DEFINE_GAMEPLAY_TAG(LightGroup,   "LightGroup");
	UE_DEFINE_GAMEPLAY_TAG(Configurable, "Configurable");

	UE_DEFINE_GAMEPLAY_TAG(Camera_ThirdPerson,     "Camera.ThirdPerson");
	UE_DEFINE_GAMEPLAY_TAG(Camera_FirstPerson,     "Camera.FirstPerson");
	UE_DEFINE_GAMEPLAY_TAG(Camera_Photo,           "Camera.Photo");
	UE_DEFINE_GAMEPLAY_TAG(Input_Mouse3P,          "Input.Mouse3P");
	UE_DEFINE_GAMEPLAY_TAG(Input_FirstPerson,      "Input.FirstPerson");
	UE_DEFINE_GAMEPLAY_TAG(Input_Gamepad,          "Input.Gamepad");
	UE_DEFINE_GAMEPLAY_TAG(Lighting_Preset_Day,     "Lighting.Preset.Day");
	UE_DEFINE_GAMEPLAY_TAG(Lighting_Preset_Evening, "Lighting.Preset.Evening");
	UE_DEFINE_GAMEPLAY_TAG(Lighting_Preset_Night,   "Lighting.Preset.Night");
}
