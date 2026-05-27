
#include "semiclip.h"

::plugin_info_t Plugin_info
{
    META_INTERFACE_VERSION,
    SEMICLIP_TITLE,
    SEMICLIP_VERSION,
    __DATE__,
    SEMICLIP_AUTHOR,
    SEMICLIP_URL,
    SEMICLIP_TAG,
    PT_ANYTIME,
    PT_ANYTIME,
};

::enginefuncs_s g_engfuncs{ };
::globalvars_t* gpGlobals = NULL;
::DLL_FUNCTIONS g_ApiFuncTable{ };
::DLL_FUNCTIONS* g_pApiFuncTable = NULL;
::DLL_FUNCTIONS g_ApiFuncTable_Post{ };
::DLL_FUNCTIONS* g_pApiFuncTable_Post = NULL;
::META_FUNCTIONS g_MetaFuncTable{ };
::meta_globals_s* gpMetaGlobals = NULL;
::meta_util_funcs_s* gpMetaUtilFuncs = NULL;
::gamedll_funcs_t* gpGamedllFuncs = NULL;
::patch_s g_origPatch{ };
::edict_s* g_pEntities = NULL;
::cvar_s g_Version{ "semiclip_version", SEMICLIP_VERSION_FULL, FCVAR_SERVER | FCVAR_SPONLY, };
::cvar_s g_Enabled{ "semiclip_enabled", "1", };
::cvar_s g_Style{ "semiclip_style", "3", };
::cvar_s g_Head{ "semiclip_head", "1", };
::cvar_s g_Team{ "semiclip_team", "3", };
::cvar_s g_teamOffs{ "semiclip_teamoffs", "0", };
::cvar_s g_obsOffs{ "semiclip_obsoffs", "0", };
::cvar_s g_Patch{ "semiclip_patch", "1", };
::cvar_s g_Dying{ "semiclip_dying", "2", };
::cvar_s g_Observer{ "semiclip_observer", "2", };
::cvar_s* g_pEnabled = NULL;
::cvar_s* g_pStyle = NULL;
::cvar_s* g_pHead = NULL;
::cvar_s* g_pTeam = NULL;
::cvar_s* g_pTeamOffs = NULL;
::cvar_s* g_pObsOffs = NULL;
::cvar_s* g_pPatch = NULL;
::cvar_s* g_pDying = NULL;
::cvar_s* g_pObserver = NULL;
bool g_reHLDS = false;
bool g_isEnabled = false;
unsigned char g_Type = false;
bool g_onHead = false;
unsigned char g_teamType = false;
::size_t g_teamOffsNum = false;
bool g_doPatch = false;
unsigned char g_dyingType = false;
unsigned char g_obsType = false;
::size_t g_obsOffsNum = false;
void* g_pPatchAddr = NULL;
float g_execTime = 0.f;
::SourceHook::CVector < ::sigdata_s > g_Sigs{ };

bool readCfg(bool forLinux)
{
    char Buffer[256], Game[64];
    ::g_engfuncs.pfnGetGameDir(Game);
    ::_snprintf(Buffer, sizeof Buffer, "%s/addons/semiclip/semiclip.ini", Game);
#ifndef __linux__
    ::_iobuf* pConfig;
    ::fopen_s(&pConfig, Buffer, "r");
#else
    ::FILE* pConfig = ::fopen(Buffer, "r");
#endif
    if (!pConfig)
        return false;
    ::SourceHook::String Line;
    ::sigdata_s sigData;
    bool Linux;
    while (::fgets(Buffer, sizeof Buffer, pConfig))
    {
        Line = Buffer;
        Line.trim();
        if (Line.empty() || ';' == Line[0] || '/' == Line[0])
            continue;
        else if (false == Line.icmp("[Linux]"))
        {
            Linux = true;
            continue;
        }
        else if (false == Line.icmp("[Windows]"))
        {
            Linux = false;
            continue;
        }
        if (Linux != forLinux)
            continue;
        if (false == Line.icmpn("Sig:", 4))
        {
            sigData.IsSymbol = false;
            ::vectorizeSignature(Line, sigData.Signature);
        }
        else if (false == Line.icmpn("OffsDec:", 8))
        {
            Line.erase(false, 8);
            Line.trim();
            sigData.Offs = ::strtoull(Line.c_str(), NULL, int(10));
        }
        else if (false == Line.icmpn("OffsHex:", 8))
        {
            Line.erase(false, 8);
            Line.trim();
            sigData.Offs = ::strtoull(Line.c_str(), NULL, int(16));
        }
        else
        {
            sigData.IsSymbol = true;
            sigData.Symbol = Line;
        }
        ::g_Sigs.push_back(sigData);
    }
    ::fclose(pConfig);
    return true;
}

C_DLLEXPORT int GetEntityAPI2(::DLL_FUNCTIONS* pApiFuncTable, int*)
{
    ::memset(&::g_ApiFuncTable, false, sizeof(::DLL_FUNCTIONS));
    ::memcpy(pApiFuncTable, &::g_ApiFuncTable, sizeof(::DLL_FUNCTIONS));
    ::g_pApiFuncTable = pApiFuncTable;
    return true;
}

C_DLLEXPORT int GetEntityAPI2_Post(::DLL_FUNCTIONS* pApiFuncTable_Post, int*)
{
    ::memset(&::g_ApiFuncTable_Post, false, sizeof(::DLL_FUNCTIONS));
    ::g_ApiFuncTable_Post.pfnServerActivate = ::ServerActivate_Post;
    ::memcpy(pApiFuncTable_Post, &::g_ApiFuncTable_Post, sizeof(::DLL_FUNCTIONS));
    ::g_pApiFuncTable_Post = pApiFuncTable_Post;
    return true;
}

C_DLLEXPORT int Meta_Query(const char*, ::plugin_info_t** ppPlugInfo, ::meta_util_funcs_s* pMetaUtilFuncs)
{
    *ppPlugInfo = PLID;
    ::gpMetaUtilFuncs = pMetaUtilFuncs;
    return true;
}

C_DLLEXPORT int Meta_Attach
(::PLUG_LOADTIME, ::META_FUNCTIONS* pMetaFuncTable, ::meta_globals_s* pMetaGlobals, ::gamedll_funcs_t* pGameDllFuncs)
{
    ::gpMetaGlobals = pMetaGlobals;
    ::gpGamedllFuncs = pGameDllFuncs;
    ::memset(&::g_MetaFuncTable, false, sizeof(::META_FUNCTIONS));
    ::g_MetaFuncTable.pfnGetEntityAPI2 = ::GetEntityAPI2;
    ::g_MetaFuncTable.pfnGetEntityAPI2_Post = ::GetEntityAPI2_Post;
    ::memcpy(pMetaFuncTable, &::g_MetaFuncTable, sizeof(::META_FUNCTIONS));
    CVAR_REGISTER(&::g_Version);
    CVAR_REGISTER(&::g_Enabled);
    CVAR_REGISTER(&::g_Style);
    CVAR_REGISTER(&::g_Head);
    CVAR_REGISTER(&::g_Team);
    CVAR_REGISTER(&::g_teamOffs);
    CVAR_REGISTER(&::g_Patch);
    CVAR_REGISTER(&::g_Dying);
    CVAR_REGISTER(&::g_obsOffs);
    CVAR_REGISTER(&::g_Observer);
    ::g_pEnabled = CVAR_GET_POINTER(::g_Enabled.name);
    ::g_pStyle = CVAR_GET_POINTER(::g_Style.name);
    ::g_pHead = CVAR_GET_POINTER(::g_Head.name);
    ::g_pTeam = CVAR_GET_POINTER(::g_Team.name);
    ::g_pTeamOffs = CVAR_GET_POINTER(::g_teamOffs.name);
    ::g_pPatch = CVAR_GET_POINTER(::g_Patch.name);
    ::g_pDying = CVAR_GET_POINTER(::g_Dying.name);
    ::g_pObsOffs = CVAR_GET_POINTER(::g_obsOffs.name);
    ::g_pObserver = CVAR_GET_POINTER(::g_Observer.name);
    if (!::g_pPatchAddr)
        LOG_CONSOLE(PLID, "* Semiclip attached with unusable 'semiclip_patch'. Unable to patch engine's SV_ClipToLinks().");
    return true;
}

C_DLLEXPORT int Meta_Detach(::PLUG_LOADTIME, ::PL_UNLOAD_REASON)
{
    ::delPatch();
    return true;
}

C_DLLEXPORT void WINAPI GiveFnptrsToDll(::enginefuncs_s* pEngineFuncs, ::globalvars_t* pGlobalVars)
{
    ::memcpy(&::g_engfuncs, pEngineFuncs, sizeof(::enginefuncs_s));
    ::gpGlobals = pGlobalVars;
#ifdef __linux__
    if (false == ::readCfg(true))
    {
        pEngineFuncs->pfnServerPrint("[WARNING] Team Semiclip: Can't read 'addons/semiclip/semiclip.ini'.\n");
        return;
    }
    ::Dl_info memInfo;
    ::dladdr(pGlobalVars /** engine_i486.so file. */, &memInfo);
    auto pLib = ::dlopen(memInfo.dli_fname, RTLD_LAZY | RTLD_NOLOAD | RTLD_NODELETE);
    if (!pLib)
        pLib = ::dlopen(memInfo.dli_fname, RTLD_NOW);
    auto pAddr = (const unsigned char*) ::dlsymComplex(pLib, ::g_Sigs[::sig_e::SV_ClipToLinks_Re].Symbol.c_str()); /// By symbol (ReHLDS).
    if (pAddr)
        ::g_reHLDS = true;
    else
        pAddr = (const unsigned char*) ::dlsymComplex(pLib, ::g_Sigs[::sig_e::SV_ClipToLinks].Symbol.c_str()); /// By symbol (non-ReHLDS).
    ::dlclose(pLib);
    if (!pAddr)
    { /// No 'SV_ClipToLinks' symbol found. Search by signature.
        struct ::stat memData;
        ::stat(memInfo.dli_fname, &memData);
        ::size_t Addr;
        if (::findInMemory(memInfo.dli_fbase, memData.st_size, ::g_Sigs[::sig_e::SV_ClipToLinks_Sig].Signature, &Addr, true)) /// By signature.
            pAddr = (const unsigned char*)Addr;
    }
#else
    if (false == ::readCfg(false))
    {
        pEngineFuncs->pfnServerPrint("[WARNING] Team Semiclip: Can't read 'addons/semiclip/semiclip.ini'.\n");
        return;
    }
    ::_MEMORY_BASIC_INFORMATION memInfo;
    ::VirtualQuery(pGlobalVars /** swds.dll file. */, &memInfo, sizeof memInfo);
    auto pDosHdr = (::_IMAGE_DOS_HEADER*)memInfo.AllocationBase;
    auto pNtHdr = (::_IMAGE_NT_HEADERS*)((::size_t)pDosHdr + (::size_t)pDosHdr->e_lfanew);
    auto pAddr = ::findStrPush((const unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage,
        (const unsigned char*) ::g_Sigs[::sig_e::SV_ClipToLinks_Re].Symbol.c_str(),
        ::g_Sigs[::sig_e::SV_ClipToLinks_Re].Symbol.size()); /// Function approx. addr. by const. string (ReHLDS).
#endif
#ifdef __linux__
    if (!pAddr)
        return; /// 'SV_ClipToLinks' function is missing [Linux] (both signature and symbol failed). Stop.
#else
    if (!pAddr)
    { /// Maybe running Windows non-ReHLDS.
        pAddr = ::findStrPush((const unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage,
            (const unsigned char*) ::g_Sigs[::sig_e::SV_ClipToLinks].Symbol.c_str(),
            ::g_Sigs[::sig_e::SV_ClipToLinks].Symbol.size()); /// Function approx. addr. by const. string (non-ReHLDS).
        if (!pAddr)
        {
            pAddr = (const unsigned char*) ::GetProcAddress((::HINSTANCE__*)memInfo.AllocationBase, ::g_Sigs[::sig_e::SV_ClipToLinks_Re].Symbol.c_str());
            if (pAddr)
                ::g_reHLDS = true;
            else
            {
                pAddr = (const unsigned char*) ::GetProcAddress((::HINSTANCE__*)memInfo.AllocationBase, ::g_Sigs[::sig_e::SV_ClipToLinks].Symbol.c_str());
                if (!pAddr)
                {
                    ::size_t Addr;
                    if (::findInMemory((const unsigned char*)pDosHdr, pNtHdr->OptionalHeader.SizeOfImage, ::g_Sigs[::sig_e::SV_ClipToLinks_Sig].Signature, &Addr, true)) /// By signature.
                        pAddr = (const unsigned char*)Addr;
                    else
                        return; /// 'SV_ClipToLinks' function is missing (Windows). Stop.
                }
            }
        }
    }
    else
        ::g_reHLDS = true;
#endif
    if (false == ::g_reHLDS)
        pAddr = ::memCmp(pAddr - ::g_Sigs[::sig_e::bytesBackPos].Offs,
            ::g_Sigs[::sig_e::bytesRange].Offs, SOLID_SLIDEBOX, ::g_Sigs[::sig_e::solidByte].Offs);
    else
        pAddr = ::memCmp(pAddr - ::g_Sigs[::sig_e::bytesBackPos].Offs,
            ::g_Sigs[::sig_e::bytesRange].Offs, SOLID_SLIDEBOX, ::g_Sigs[::sig_e::solidByteRe].Offs);
    if (!pAddr)
        return;
    ::g_pPatchAddr = (void*)pAddr;
}

::patch_s::patch_s()
{
    addr = NULL;
}

::patch_s::~patch_s()
{
    addr = NULL;
}

void makePatch()
{
    static char Data[32];
    if (!::g_origPatch.addr && ::g_pPatchAddr)
    {
        ::g_origPatch.addr = ::g_pPatchAddr;
        ::memcpy(::g_origPatch.data, ::g_pPatchAddr,
            false == ::g_reHLDS ? ::g_Sigs[::sig_e::patchBytes].Offs : ::g_Sigs[::sig_e::patchBytesRe].Offs);
        ::memcpy(Data, ::g_pPatchAddr,
            false == ::g_reHLDS ? ::g_Sigs[::sig_e::patchBytes].Offs : ::g_Sigs[::sig_e::patchBytesRe].Offs);
        *(Data + (false == ::g_reHLDS ? ::g_Sigs[::sig_e::solidByte].Offs : ::g_Sigs[::sig_e::solidByteRe].Offs)) =
            SOLID_NOT;
        ::memCpy(::g_pPatchAddr, Data,
            false == ::g_reHLDS ? ::g_Sigs[::sig_e::patchBytes].Offs : ::g_Sigs[::sig_e::patchBytesRe].Offs);
    }
}

void delPatch()
{
    if (::g_origPatch.addr)
    {
        ::memCpy(::g_origPatch.addr, ::g_origPatch.data,
            false == ::g_reHLDS ? ::g_Sigs[::sig_e::patchBytes].Offs : ::g_Sigs[::sig_e::patchBytesRe].Offs);
        ::g_origPatch.addr = NULL;
    }
}

bool canPass_Move(::playermove_s* pMove, int Host, int Player)
{ /// If false is returned, collision is turned off.
    static bool Can, Alive;
    static unsigned char hTeam, pTeam;
    static ::edict_s* pHost, * pPlayer;
    static ::entvars_s* pHVar, * pPVar;

    if (Host == Player)
        return true;
    pHost = F_IToE(Host);
    pPlayer = F_IToE(Player);
    pHVar = &pHost->v;
    pPVar = &pPlayer->v;
    if (::g_teamType)
    {
        hTeam = ::g_teamOffsNum ?
            (unsigned char)(*(int*)(((unsigned char*)(pHost->pvPrivateData)) + ::g_teamOffsNum)) :
            (unsigned char)pHVar->team;
        pTeam = ::g_teamOffsNum ?
            (unsigned char)(*(int*)(((unsigned char*)(pPlayer->pvPrivateData)) + ::g_teamOffsNum)) :
            (unsigned char)pPVar->team;
    }
    else
    {
        hTeam = false;
        pTeam = false;
    } /// Fully excludes dead and dying players from collision.
    Alive = hTeam > 0 && pTeam > 0 && hTeam < 3 && pTeam < 3 && !pMove->dead && !pMove->spectator && !pMove->deadflag &&
        !pHVar->deadflag && !pPVar->deadflag && pHVar->health > 0.f && pPVar->health > 0.f;
    Can = !Alive || !::g_teamType ? true : ::g_teamType == 3 ? hTeam == pTeam :
        (hTeam == ::g_teamType && pTeam == ::g_teamType);
    if (Can && Alive && ::g_onHead)
    { /// If both fully alive and one is above the other, collide them.
        if (pHVar->groundentity == pPlayer || pPVar->groundentity == pHost ||
            pHVar->absmin.z > pPVar->absmax.z || pPVar->absmin.z > pHVar->absmax.z)
            return false;
    }
    return Can;
}

bool validObsTarget(::edict_s* pPlayer)
{
    static int Idx;
    static ::size_t uIdx;
    switch (::g_obsType)
    {
    case 1:
    {
        Idx = pPlayer->v.iuser1;
        return Idx > 0 && Idx != F_EToI(pPlayer);
    }
    case 2:
    {
        Idx = pPlayer->v.iuser2;
        return Idx > 0 && Idx != F_EToI(pPlayer);
    }
    case 3:
    {
        Idx = pPlayer->v.iuser3;
        return Idx > 0 && Idx != F_EToI(pPlayer);
    }
    case 4:
    {
        Idx = pPlayer->v.iuser4;
        return Idx > 0 && Idx != F_EToI(pPlayer);
    }
    case 5:
    {
        uIdx = (::size_t)pPlayer->v.euser1;
        return uIdx > 0 && uIdx != (::size_t)pPlayer;
    }
    case 6:
    {
        uIdx = (::size_t)pPlayer->v.euser2;
        return uIdx > 0 && uIdx != (::size_t)pPlayer;
    }
    case 7:
    {
        uIdx = (::size_t)pPlayer->v.euser3;
        return uIdx > 0 && uIdx != (::size_t)pPlayer;
    }
    case 8:
    {
        uIdx = (::size_t)pPlayer->v.euser4;
        return uIdx > 0 && uIdx != (::size_t)pPlayer;
    }
    case 9:
    {
        Idx = *(signed char*)(((unsigned char*)(pPlayer->pvPrivateData)) + ::g_obsOffsNum);
        return Idx > 0 && Idx != F_EToI(pPlayer);
    }
    case 10:
    {
        uIdx = *(unsigned char*)(((unsigned char*)(pPlayer->pvPrivateData)) + ::g_obsOffsNum);
        return uIdx > 0 && uIdx != F_EToI(pPlayer);
    }
    case 11:
    {
        Idx = *(signed short*)(((unsigned char*)(pPlayer->pvPrivateData)) + ::g_obsOffsNum);
        return Idx > 0 && Idx != F_EToI(pPlayer);
    }
    case 12:
    {
        uIdx = *(unsigned short*)(((unsigned char*)(pPlayer->pvPrivateData)) + ::g_obsOffsNum);
        return uIdx > 0 && uIdx != F_EToI(pPlayer);
    }
    case 13:
    {
        Idx = *(signed int*)(((unsigned char*)(pPlayer->pvPrivateData)) + ::g_obsOffsNum);
        return Idx > 0 && Idx != F_EToI(pPlayer);
    }
    case 14:
    {
        uIdx = *(::size_t*)(((unsigned char*)(pPlayer->pvPrivateData)) + ::g_obsOffsNum);
        return uIdx > 0 && uIdx != F_EToI(pPlayer);
    }
    case 15:
    {
        uIdx = *(::size_t*)(((unsigned char*)(pPlayer->pvPrivateData)) + ::g_obsOffsNum);
        return uIdx > 0 && uIdx != (::size_t)pPlayer;
    }
    case 16:
    {
        uIdx = *(::size_t*)(((unsigned char*)(pPlayer->pvPrivateData)) + ::g_obsOffsNum);
        return uIdx > 0 && uIdx != (::size_t)pPlayer->pvPrivateData;
    }
    }
    uIdx = *(::size_t*)(((unsigned char*)(pPlayer->pvPrivateData)) + ::g_obsOffsNum);
    return uIdx > 0 && uIdx != (::size_t)&pPlayer->v;
}

bool canPass_Pack(::edict_s* pHost, ::edict_s* pPlayer)
{ /// If false is returned, transparency is stopped.
    static bool Can;
    static unsigned char hTeam, pTeam;
    static ::entvars_s* pHVar, * pPVar;

    pHVar = &pHost->v;
    pPVar = &pPlayer->v;
    switch (::g_dyingType)
    {
    case 1:
    {
        if (pHVar->deadflag > DEAD_DYING || pPVar->deadflag > DEAD_DYING)
            return false; /// Wait a bit after a player dies to turn off transparency.
        break;
    }
    case 2:
    {
        if (((pHVar->deadflag || pHVar->health <= 0.f) && ::validObsTarget(pHost)) ||
            ((pPVar->deadflag || pPVar->health <= 0.f) && ::validObsTarget(pPlayer)))
            return false; /// Wait until a player starts spectating someone else to turn off transparency.
        break;
    }
    default:
    {
        if (pHVar->deadflag || pPVar->deadflag || pHVar->health <= 0.f || pPVar->health <= 0.f)
            return false; /// Once a player is dead, instantly turns off transparency.
        break;
    }
    }
    if (::g_teamType)
    {
        hTeam = ::g_teamOffsNum ? (unsigned char)(*(int*)(((unsigned char*)(pHost->pvPrivateData)) + ::g_teamOffsNum)) :
            (unsigned char)pHVar->team;
        pTeam = ::g_teamOffsNum ? (unsigned char)(*(int*)(((unsigned char*)(pPlayer->pvPrivateData)) + ::g_teamOffsNum)) :
            (unsigned char)pPVar->team;
    }
    else
    {
        hTeam = false;
        pTeam = false;
    }
    Can = !::g_teamType ? true : ::g_teamType == 3 ? hTeam == pTeam :
        (hTeam == ::g_teamType && pTeam == ::g_teamType);
    if (Can && ::g_onHead)
    { /// If one is above the other, no transparency.
        if (pHVar->groundentity == pPlayer || pPVar->groundentity == pHost ||
            pHVar->absmin.z > pPVar->absmax.z || pPVar->absmin.z > pHVar->absmax.z)
            return false;
    }
    return Can;
}

void PM_Move(::playermove_s* pMove, int)
{
    static int Iter, Host, Players, Entities, Player;

    Entities = -1;
    Players = false;
    Host = pMove->player_index + 1;
    for (Iter = false; Iter < pMove->numphysent; ++Iter)
    {
        if (pMove->physents[++Entities].player && ++Players)
            break;
    }
    for (Iter = Entities; Iter < pMove->numphysent; ++Iter)
    {
        Player = pMove->physents[Iter].player;
        if (!Player || !::canPass_Move(pMove, Host, Player))
            pMove->physents[Entities++] = pMove->physents[Iter];
    }
    pMove->numphysent = Entities;
    ::gpMetaGlobals->mres = ::META_RES::MRES_IGNORED;
}

bool isOut(::edict_s* pEntity, ::edict_s* pHost)
{
    static ::Vector* eAbsMin, * eAbsMax, * hAbsMin, * hAbsMax;
    eAbsMin = &pEntity->v.absmin;
    eAbsMax = &pEntity->v.absmax;
    hAbsMin = &pHost->v.absmin;
    hAbsMax = &pHost->v.absmax;
    return hAbsMin->x > eAbsMax->x || hAbsMin->y > eAbsMax->y || hAbsMin->z > eAbsMax->z ||
        hAbsMax->x < eAbsMin->x || hAbsMax->y < eAbsMin->y || hAbsMax->z < eAbsMin->z;
}

int AddToFullPack_Post
(::entity_state_s* pState, int eIdx, ::edict_s* pEntity, ::edict_s* pHost, int hostFlags, int isPlayer, unsigned char* pSet)
{
    static float Dis;
    if (pState && pHost && pEntity && isPlayer && pHost != pEntity && ::canPass_Pack(pHost, pEntity))
    {
        switch (::g_Type)
        {
        case 1:
        {
            Dis = (pEntity->v.origin - pHost->v.origin).Length();
            if (Dis < 256.f)
            {
                pState->rendermode = ::kRenderTransAlpha;
                pState->renderamt = int(Dis * 255.f / 255.f); /// 0 <-> 255
            }
            break;
        }
        case 2:
        {
            if (!::isOut(pEntity, pHost))
            {
                pState->rendermode = ::kRenderTransAlpha;
                pState->renderamt = false;
            }
            break;
        }
        default:
        {
            if (!::isOut(pEntity, pHost))
            {
                pState->rendermode = ::kRenderTransAlpha;
                pState->renderamt = int((pEntity->v.origin - pHost->v.origin).Length() * 255.f / 255.f);
            }
            break;
        }
        }
    }
    ::gpMetaGlobals->mres = ::META_RES::MRES_IGNORED;
    return false;
}

void StartFrame()
{
    static char Cmd[256], Pfx[64];
    if (::gpGlobals->time >= ::g_execTime)
    {
#ifndef __linux__
        ::strncpy_s(Cmd, sizeof Cmd, "exec addons/semiclip/semiclip.cfg\n", _TRUNCATE);
#else
        ::strcpy(Cmd, "exec addons/semiclip/semiclip.cfg\n");
#endif
        SERVER_COMMAND(Cmd);
        auto pMap = STRING(::gpGlobals->mapname);
#ifndef __linux__
        ::strncpy_s(Pfx, sizeof Pfx, pMap, _TRUNCATE);
#else
        ::strcpy(Pfx, pMap);
#endif
        auto pPfxPos = ::strchr(Pfx, '_');
        if (pPfxPos)
        {
            *(pPfxPos + 1) = false;
            ::_snprintf(Cmd, sizeof Cmd, "exec \"addons/semiclip/maps/%s.cfg\"\n", Pfx);
            SERVER_COMMAND(Cmd);
        }
        ::_snprintf(Cmd, sizeof Cmd, "exec \"addons/semiclip/maps/%s.cfg\"\n", pMap);
        SERVER_COMMAND(Cmd);
        ::g_execTime = 0.f;
        ::g_pApiFuncTable->pfnStartFrame = NULL;
    }
    ::gpMetaGlobals->mres = ::META_RES::MRES_IGNORED;
}

void ServerActivate_Post(::edict_s* pEntities, int, int)
{
    ::g_pEntities = pEntities;
    ::g_execTime = ::gpGlobals->time + .1f; /// Executing cfg. files too soon doesn't work.
    ::g_pApiFuncTable->pfnStartFrame = ::StartFrame;
    ::g_pApiFuncTable_Post->pfnClientPutInServer = ::OnClientPutInServer_Post;
    ::gpMetaGlobals->mres = ::META_RES::MRES_IGNORED;
}

void OnClientPutInServer_Post(::edict_s*)
{
    ::g_isEnabled = bool(::g_pEnabled->value);
    ::g_Type = (unsigned char) ::g_pStyle->value;
    ::g_onHead = bool(::g_pHead->value);
    ::g_teamType = (unsigned char) ::g_pTeam->value;
    ::g_teamOffsNum = (::size_t) ::g_pTeamOffs->value;
    ::g_doPatch = bool(::g_pPatch->value);
    ::g_dyingType = (unsigned char) ::g_pDying->value;
    ::g_obsOffsNum = (::size_t) ::g_pObsOffs->value;
    ::g_obsType = (unsigned char) ::g_pObserver->value;
    switch (::g_doPatch)
    {
    case true:
    {
        ::makePatch();
        break;
    }
    default:
    {
        ::delPatch();
        break;
    }
    }
    switch (::g_Type)
    {
    case 1: case 2: case 3:
    {
        ::g_pApiFuncTable_Post->pfnAddToFullPack = ::AddToFullPack_Post;
        break;
    }
    default:
    {
        ::g_pApiFuncTable_Post->pfnAddToFullPack = NULL;
        break;
    }
    }
    switch (::g_isEnabled)
    {
    case true:
    {
        ::g_pApiFuncTable->pfnPM_Move = ::PM_Move;
        break;
    }
    default:
    {
        ::g_pApiFuncTable->pfnPM_Move = NULL;
        ::g_pApiFuncTable_Post->pfnAddToFullPack = NULL;
        ::delPatch();
        break;
    }
    }
    ::g_pApiFuncTable_Post->pfnClientPutInServer = NULL;
    ::gpMetaGlobals->mres = ::META_RES::MRES_IGNORED;
}
