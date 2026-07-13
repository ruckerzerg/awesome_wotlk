#include "Spell.h"
#include "Lua.h"
#include "Hooks.h"

#undef min
#undef max

namespace {
// Client-side stance/form (shapeshift) requirement check for spell casts.
//   __thiscall bool CheckShapeshiftRequirement(CGUnit_C* player, SpellRec* spell, int* outResult)
// Returns 1 when the player already satisfies the spell's required stance/form (cast allowed),
// otherwise returns 0 and writes a SPELL_FAILED_* code to *outResult (e.g. 0x5E ONLY_SHAPESHIFT,
// the "you must be in Battle Stance" error for Charge). All three callers (0x73A0F7, 0x80AF6C,
// 0x80B864) block the cast client-side on a 0 return instead of sending it to the server.
// Forcing an unconditional "requirement met" return lets every cast reach the server, which still
// validates stance/form itself, so macros like Charge are no longer swallowed by the client.
constexpr uintptr_t StanceCheckFn_addr = 0x0072BAC0;
constexpr uint8_t StanceCheckOrig[5] = { 0x55, 0x8B, 0xEC, 0x56, 0x8B };  // push ebp; mov ebp,esp; push esi; mov esi,...
constexpr uint8_t StanceCheckPatch[5] = { 0xB0, 0x01, 0xC2, 0x08, 0x00 }; // mov al,1; ret 8

int g_removeStanceRequirement = 0;
CVar* s_cvar_removeStanceRequirement;

int CVarHandler_removeStanceRequirement(CVar* cvar, const char*, const char* value, void*) {
	const int result = cvar->Sync(value, &g_removeStanceRequirement, 0, 1, "%d");
	const uint8_t* bytes = g_removeStanceRequirement ? StanceCheckPatch : StanceCheckOrig;
	Hooks::PatchBytes(reinterpret_cast<void*>(StanceCheckFn_addr), bytes, sizeof(StanceCheckPatch));
	return result;
}

int lua_GetSpellBaseCooldown(lua_State* L) {
	uint32_t spellId = static_cast<uint32_t>(Lua::luaL_checknumber(L, 1));

	auto* spellData = ClientDB::GetRowById<SpellRec>("spell", spellId);
	if (!spellData) return 0;

	uint32_t cdTime = spellData->m_recoveryTime ? spellData->m_recoveryTime : spellData->m_categoryRecoveryTime;
	uint32_t gcdTime = spellData->m_startRecoveryTime;

	if (cdTime == 0) {
		for (uint32_t triggeredId : spellData->m_effectTriggerSpell) {
			if (triggeredId == 0 || triggeredId == spellId) continue;
			if (auto* rec = ClientDB::GetRowById<SpellRec>("spell", triggeredId)) {
				uint32_t trigCd = rec->m_recoveryTime ? rec->m_recoveryTime : rec->m_categoryRecoveryTime;
				cdTime = std::max(cdTime, trigCd);
				gcdTime = std::max(gcdTime, rec->m_startRecoveryTime);
			}
		}
	}

	Lua::lua_pushnumber(L, cdTime);
	Lua::lua_pushnumber(L, gcdTime);
	return 2;
}

int lua_openmisclib(lua_State* L) {
	Lua::lua_pushcfunction(L, lua_GetSpellBaseCooldown);
	Lua::lua_setglobal(L, "GetSpellBaseCooldown");
	return 0;
}
}

void Spell::initialize() {
	Hooks::FrameXML::registerLuaLib(lua_openmisclib);
	Hooks::FrameXML::registerCVar(&s_cvar_removeStanceRequirement, "removeStanceRequirement", nullptr, "0", CVarHandler_removeStanceRequirement);
}
