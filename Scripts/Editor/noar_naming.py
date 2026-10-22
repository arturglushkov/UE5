"""
NOAR naming convention helpers (Standard v1.1, section 2).

Interactive millwork mesh name:
    SM_<APT>_<ROOM>_<UNIT>_<PART>[_<INDEX>][_<SIDE>]
    e.g. SM_DEM_KIT_B03_DOOR_01_L, SM_DEM_KIT_B03_DRW_02, SM_DEM_KIT_B03_LED
"""
import re

ROOMS = {"FOY", "KIT", "DIN", "LIV", "PBR", "WIC", "PEN", "GBR", "GEN", "DEN", "PWD", "LAU", "BAL", "HAL", "MEC"}
UNIT_RE = re.compile(r"^[BWTIVPSA]\d{2}$")  # B01 base, W01 wall, T01 tall, I01 island, V01 vanity, P01 panel, S01 shelving, A01 appliance
PARTS = {"CARC", "DOOR", "DRW", "FLAP", "SLIDE", "POCKET", "INT", "LED", "SHELF", "TRAY", "TOP", "BED", "LEGS", "FRAME", "GLASS"}
MOVING_PARTS = {"DOOR", "DRW", "FLAP", "SLIDE", "POCKET", "TRAY", "BED", "LEGS"}
SIDES = {"L", "R", "T", "B"}

NAME_RE = re.compile(
    r"^SM_(?P<apt>[A-Z]{3})_(?P<room>[A-Z]{3})_(?P<unit>[BWTIVPSA]\d{2})_(?P<part>[A-Z]+)"
    r"(?:_(?P<index>\d{2}))?(?:_(?P<side>[LRTB]))?$"
)


def parse(name: str):
    """Return dict of name segments or None when the name does not follow the convention."""
    m = NAME_RE.match(name)
    if not m:
        return None
    d = m.groupdict()
    if d["room"] not in ROOMS or d["part"] not in PARTS:
        return None
    return d


def is_moving_part(name: str) -> bool:
    d = parse(name)
    return bool(d) and d["part"] in MOVING_PARTS


def actor_name(mesh_name: str) -> str:
    """Level actor name = mesh name without the SM_ prefix."""
    return mesh_name[3:] if mesh_name.startswith("SM_") else mesh_name


def bp_class_for_part(part: str) -> str:
    return {
        "DOOR": "/Game/NOAR/Interaction/Doors/BP_InteractiveDoor.BP_InteractiveDoor_C",
        "DRW": "/Game/NOAR/Interaction/Drawers/BP_InteractiveDrawer.BP_InteractiveDrawer_C",
        "TRAY": "/Game/NOAR/Interaction/Drawers/BP_InteractiveDrawer.BP_InteractiveDrawer_C",
        "FLAP": "/Game/NOAR/Interaction/Mechanisms/BP_InteractiveFlap.BP_InteractiveFlap_C",
        "BED": "/Game/NOAR/Interaction/Mechanisms/BP_InteractiveFlap.BP_InteractiveFlap_C",
        "SLIDE": "/Game/NOAR/Interaction/Doors/BP_InteractiveSlidingDoor.BP_InteractiveSlidingDoor_C",
        "POCKET": "/Game/NOAR/Interaction/Mechanisms/BP_InteractivePocket.BP_InteractivePocket_C",
        "LED": "/Game/NOAR/Lighting/BP_InteractiveLight.BP_InteractiveLight_C",
    }.get(part, "")
