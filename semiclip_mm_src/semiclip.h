
#ifndef _MAIN_H_
#define _MAIN_H_

#include "extdll.h"
#include "meta_api.h"
#include "pm_defs.h"
#include "entity_state.h"
#include "mem.h"
#include "Memory.h" /// IDA signature to address.

#define SEMICLIP_VERSION        "2.2+"
#define SEMICLIP_VERSION_MS     2,2,0,5
#define SEMICLIP_AUTHOR         "s1lent & claudiuhks"
#define SEMICLIP_TITLE          "Team Semiclip"
#define SEMICLIP_TITLE_MS       "MetaMod " SEMICLIP_TITLE
#define SEMICLIP_COMPANY_MS     "AlliedModders LLC"
#define SEMICLIP_FILENAME_MS    "semiclip.dll"
#define SEMICLIP_YEAR_MS        "2026"
#define SEMICLIP_URL            "https://forums.alliedmods.net/"
#define SEMICLIP_TAG            "SEMICLIP"
#define SEMICLIP_VERSION_FULL   SEMICLIP_VERSION " (" SEMICLIP_AUTHOR ")"

#define F_EToI(E) ((::size_t)       (E - ::g_pEntities))
#define F_IToE(I) ((::edict_s *)    (::g_pEntities + I))

enum sig_e : unsigned char {
    opCode = false,
    opCodeRe,
    byteVal,
    byteValRe,
    solidByte,
    solidByteRe,
    patchByte,
    patchByteRe,
    SV_ClipToLinks,
    SV_ClipToLinks_Re,
    SV_ClipToLinks_Sig,
    SV_ClipToLinks_Sig_Re,
    bytesRange,
    bytesRangeRe,
    bytesBackPos,
    bytesBackPosRe,
};

typedef struct sigdata_s {
    ::SourceHook::String Sym{ };
    ::SourceHook::CVector < unsigned char > Sig{ };
    bool isSym = false;
    ::size_t Ofs = false;
} sigdata_t;

C_DLLEXPORT int GetEntityAPI2(::DLL_FUNCTIONS*, int*);
C_DLLEXPORT int GetEntityAPI2_Post(::DLL_FUNCTIONS*, int*);
C_DLLEXPORT int Meta_Query(const char*, ::plugin_info_t**, ::meta_util_funcs_s*);
C_DLLEXPORT int Meta_Attach(::PLUG_LOADTIME, ::META_FUNCTIONS*, ::meta_globals_s*, ::gamedll_funcs_t*);
C_DLLEXPORT int Meta_Detach(::PLUG_LOADTIME, ::PL_UNLOAD_REASON);
C_DLLEXPORT void WINAPI GiveFnptrsToDll(::enginefuncs_s*, ::globalvars_t*);

bool readCfg(bool);
void makePatch();
void delPatch();
bool validObsTarget(::edict_s*);
bool isOut(::edict_s*, ::edict_s*);
bool canPass_Move(::playermove_s*, int, int);
bool canPass_Pack(::edict_s*, ::edict_s*);
void StartFrame();
void PM_Move(::playermove_s*, int); /// Real semiclip.
void ServerActivate_Post(::edict_s*, int, int);
void OnClientPutInServer_Post(::edict_s*);
int AddToFullPack_Post(::entity_state_s*, int, ::edict_s*, ::edict_s*, int, int, unsigned char*); /// Transparency (visual only).

extern ::plugin_info_t Plugin_info;
extern ::enginefuncs_s g_engfuncs;
extern ::globalvars_t* gpGlobals;
extern ::DLL_FUNCTIONS g_ApiFuncTable;
extern ::DLL_FUNCTIONS* g_pApiFuncTable;
extern ::DLL_FUNCTIONS g_ApiFuncTable_Post;
extern ::DLL_FUNCTIONS* g_pApiFuncTable_Post;
extern ::META_FUNCTIONS g_MetaFuncTable;
extern ::meta_globals_s* gpMetaGlobals;
extern ::meta_util_funcs_s* gpMetaUtilFuncs;
extern ::gamedll_funcs_t* gpGamedllFuncs;
extern ::edict_s* g_pEntities;
extern ::cvar_s g_Version;
extern ::cvar_s g_Enabled;
extern ::cvar_s g_Style;
extern ::cvar_s g_Head;
extern ::cvar_s g_Team;
extern ::cvar_s g_teamOffs;
extern ::cvar_s g_obsOffs;
extern ::cvar_s g_Patch;
extern ::cvar_s g_Solid;
extern ::cvar_s g_Dying;
extern ::cvar_s g_Observer;
extern ::cvar_s* g_pEnabled;
extern ::cvar_s* g_pStyle;
extern ::cvar_s* g_pHead;
extern ::cvar_s* g_pTeam;
extern ::cvar_s* g_pTeamOffs;
extern ::cvar_s* g_pObsOffs;
extern ::cvar_s* g_pPatch;
extern ::cvar_s* g_pSolid;
extern ::cvar_s* g_pDying;
extern ::cvar_s* g_pObserver;
extern bool g_Patched;
extern bool g_reHLDS;
extern bool g_isEnabled;
extern unsigned char g_Type;
extern bool g_onHead;
extern unsigned char g_teamType;
extern ::size_t g_teamOffsNum;
extern bool g_doPatch;
extern bool g_doSolid;
extern unsigned char g_dyingType;
extern unsigned char g_obsType;
extern ::size_t g_obsOffsNum;
extern unsigned char* g_pPatchAddr;
extern float g_execTime;
extern ::SourceHook::CVector < ::sigdata_s > g_Sigs;

#endif
