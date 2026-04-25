#pragma once
#include "../../EngineCore.h"
#include "../../Handle.h"
#include <bitset>

// index and version packed into entityhandle
typedef u32 EntityIndex;
typedef u32 EntityVersion;
typedef u64 EntityHandle;

static const u32 c_MaxEntities = 5000;
static const u8 MAX_COMPONENTS = 8;
static u32 s_ComponentCounter = 0;

typedef std::bitset<MAX_COMPONENTS> ComponentMask;

class EntityAdmin;
typedef std::shared_ptr<EntityAdmin> EntityAdminRef;
