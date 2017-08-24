/*
Syn's AyyWare Framework 2015
*/

#define _CRT_SECURE_NO_WARNINGS

#include "MiscHacks.h"
#include "Interfaces.h"
#include "RenderManager.h"

#include <time.h>

template<class T, class U>
inline T clamp(T in, U low, U high)
{
	if (in <= low)
		return low;
	else if (in >= high)
		return high;
	else
		return in;
}

inline float bitsToFloat(unsigned long i)
{
	return *reinterpret_cast<float*>(&i);
}

inline float FloatNegate(float f)
{
	return bitsToFloat(FloatBits(f) ^ 0x80000000);
}

Vector AutoStrafeView;

void CMiscHacks::Init()
{
	// Any init
}

void CMiscHacks::Draw()
{
	// Any drawing	
	// Spams
	switch (Menu::Window.MiscTab.OtherChatSpam.GetIndex())
	{
	case 0:
		// No Chat Spam
		break;
	case 1:
		// Namestealer
		ChatSpamName();
		break;
	case 2:
		// Regular
		ChatSpamRegular();
		break;
	case 3:
		// Interwebz
		ChatSpamInterwebz();
		break;
	case 4:
		// Report Spam
		ChatSpamDisperseName();
		break;
	}
}



int CircleFactor = 0;
void CMiscHacks::RotateMovement(CUserCmd* pCmd, float rotation)
{
	rotation = DEG2RAD(rotation);

	float cos_rot = cos(rotation);
	float sin_rot = sin(rotation);

	float new_forwardmove = (cos_rot * pCmd->forwardmove) - (sin_rot * pCmd->sidemove);
	float new_sidemove = (sin_rot * pCmd->forwardmove) + (cos_rot * pCmd->sidemove);

	pCmd->forwardmove = new_forwardmove;
	pCmd->sidemove = -new_sidemove;

}

bool CMiscHacks::doCircularStrafe(CUserCmd* pCmd, Vector& OriginalView) {




	IClientEntity *pLocalEntity = Interfaces::EntList->GetClientEntity(Interfaces::Engine->GetLocalPlayer());

	if (!pLocalEntity)
		return false;

	if (!pLocalEntity->IsAlive())
		return false;

	CircleFactor++;
	if (CircleFactor > 361)
		CircleFactor = 0;

	int GetItDoubled = 3.0 * CircleFactor - Interfaces::Globals->interval_per_tick;

	Vector StoredViewAngles = pCmd->viewangles;

	if ((pCmd->buttons & IN_JUMP) || !(pLocalEntity->GetFlags() & FL_ONGROUND))
	{
		pCmd->viewangles = OriginalView;
		pCmd->forwardmove = 450.f;
		RotateMovement(pCmd, GetItDoubled);
	}

	return true;
}


void CMiscHacks::Move(CUserCmd *pCmd, bool &bSendPacket)
{
	// Any Move Stuff
	
	// Bhop
	switch (Menu::Window.MiscTab.OtherAutoJump.GetIndex())
	{
	case 0:
		break;
	case 1:
		AutoJump(pCmd);
		break;
	}

	if (Menu::Window.MiscTab.OtherCircleStrafe.GetState() && Menu::Window.MiscTab.OtherCircleStrafeKey.GetKey() > 0)
	{
		int CircleKey = Menu::Window.MiscTab.OtherCircleStrafeKey.GetKey();
		if (GUI.GetKeyState(CircleKey))
		{
			doCircularStrafe(pCmd, AutoStrafeView);
		}
	}

	switch (Menu::Window.VisualsTab.AmbientSkybox.GetIndex())
	{
	case 0:
		/*Disabled*/
		break;
	case 1:
		if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame()) {
			ConVar* NoSkybox = Interfaces::CVar->FindVar("sv_skyname"); /*No-Skybox*/
			*(float*)((DWORD)&NoSkybox->fnChangeCallback + 0xC) = NULL;
			NoSkybox->SetValue("sky_l4d_rural02_ldr");
		}
		break;

	case 2:
		if (Interfaces::Engine->IsConnected() && Interfaces::Engine->IsInGame()) {
			ConVar* NightSkybox1 = Interfaces::CVar->FindVar("sv_skyname"); /*Night-Skybox*/
			*(float*)((DWORD)&NightSkybox1->fnChangeCallback + 0xC) = NULL;
			NightSkybox1->SetValue("sky_csgo_night02b");
		}
		break;

	}

	// AutoStrafe
	Interfaces::Engine->GetViewAngles(AutoStrafeView);
	switch (Menu::Window.MiscTab.OtherAutoStrafe.GetIndex())
	{
	case 0:
		// Off
		break;
	case 1:
		LegitStrafe(pCmd);
		break;
	case 2:
		RageStrafe(pCmd);
		break;
	}

	//Fake Lag
	if (Menu::Window.MiscTab.FakeLagEnable.GetState())
		Fakelag(pCmd, bSendPacket);

	if (Menu::Window.MiscTab.FakeWalk.GetState())
		FakeWalk(pCmd, bSendPacket);


}

void CMiscHacks::FakeWalk(CUserCmd * pCmd, bool & bSendPacket)
{
	IClientEntity* pLocal = hackManager.pLocal();
	if (GetAsyncKeyState(VK_SHIFT))
	{
		static int iChoked = -1;
		iChoked++;

		if (iChoked < 1)
		{
			bSendPacket = false;

			pCmd->tick_count += 10;
			pCmd->command_number += 7 + pCmd->tick_count % 2 ? 0 : 1;

			pCmd->buttons |= pLocal->GetMoveType() == IN_BACK;
			pCmd->forwardmove = pCmd->sidemove = 0.f;
		}
		else
		{
			bSendPacket = true;
			iChoked = -1;

			Interfaces::Globals->frametime *= (pLocal->GetVelocity().Length2D()) / 1.f;
			pCmd->buttons |= pLocal->GetMoveType() == IN_FORWARD;
		}
	}
}

static __declspec(naked) void __cdecl Invoke_NET_SetConVar(void* pfn, const char* cvar, const char* value)
{
	__asm 
	{
		push    ebp
			mov     ebp, esp
			and     esp, 0FFFFFFF8h
			sub     esp, 44h
			push    ebx
			push    esi
			push    edi
			mov     edi, cvar
			mov     esi, value
			jmp     pfn
	}
}
void DECLSPEC_NOINLINE NET_SetConVar(const char* value, const char* cvar)
{
	static DWORD setaddr = Utilities::Memory::FindPattern("engine.dll", (PBYTE)"\x8D\x4C\x24\x1C\xE8\x00\x00\x00\x00\x56", "xxxxx????x");
	if (setaddr != 0) 
	{
		void* pvSetConVar = (char*)setaddr;
		Invoke_NET_SetConVar(pvSetConVar, cvar, value);
	}
}

void change_name(const char* name)
{
	if (Interfaces::Engine->IsInGame() && Interfaces::Engine->IsConnected())
		NET_SetConVar(name, "name");
}

void CMiscHacks::AutoJump(CUserCmd *pCmd)
{
	if (pCmd->buttons & IN_JUMP && GUI.GetKeyState(VK_SPACE))
	{
		int iFlags = hackManager.pLocal()->GetFlags();
		if (!(iFlags & FL_ONGROUND))
			pCmd->buttons &= ~IN_JUMP;

		if (hackManager.pLocal()->GetVelocity().Length() <= 50)
		{
			pCmd->forwardmove = 450.f;
		}
	}
}

void CMiscHacks::LegitStrafe(CUserCmd *pCmd)
{
	IClientEntity* pLocal = hackManager.pLocal();
	if (!(pLocal->GetFlags() & FL_ONGROUND))
	{
		pCmd->forwardmove = 0.0f;

		if (pCmd->mousedx < 0)
		{
			pCmd->sidemove = -450.0f;
		}
		else if (pCmd->mousedx > 0)
		{
			pCmd->sidemove = 450.0f;
		}
	}
}

void CMiscHacks::RageStrafe(CUserCmd *pCmd)
{
	int Circle = Menu::Window.MiscTab.OtherCircleStrafeKey.GetKey();
	if (!GUI.GetKeyState(Circle))
	{

		IClientEntity* pLocal = hackManager.pLocal();

		bool bKeysPressed = true;
		if (GUI.GetKeyState(0x41) || GUI.GetKeyState(0x57) || GUI.GetKeyState(0x53) || GUI.GetKeyState(0x44)) bKeysPressed = false;

		if ((GetAsyncKeyState(VK_SPACE) && !(pLocal->GetFlags() & FL_ONGROUND)) && bKeysPressed)
		{
			if (pCmd->mousedx > 1 || pCmd->mousedx < -1) {
				pCmd->sidemove = pCmd->mousedx < 0.f ? -450.f : 450.f;
			}
			else {
				pCmd->forwardmove = (1800.f * 4.f) / pLocal->GetVelocity().Length2D();
				pCmd->sidemove = (pCmd->command_number % 2) == 0 ? -450.f : 450.f;
				if (pCmd->forwardmove > 450.f)
					pCmd->forwardmove = 450.f;
			}
		}

		/*	IClientEntity* pLocal = hackManager.pLocal();
		static bool bDirection = true;


		static float move = 450; //400.f; // move = max(move, (abs(cmd->move.x) + abs(cmd->move.y)) * 0.5f);
		float s_move = move * 0.5065f;


		if ((pCmd->buttons & IN_JUMP) || !(pLocal->GetFlags() & FL_ONGROUND))
		{
		pCmd->forwardmove = move * 0.015f;
		pCmd->sidemove += (float)(((pCmd->tick_count % 2) * 2) - 1) * s_move;

		if (pCmd->mousedx)
		pCmd->sidemove = (float)clamp(pCmd->mousedx, -1, 1) * s_move;

		static float strafe = pCmd->viewangles.y;

		float rt = pCmd->viewangles.y, rotation;
		rotation = strafe - rt;

		if (rotation < FloatNegate(Interfaces::Globals->interval_per_tick))
		pCmd->sidemove = -s_move;

		if (rotation > Interfaces::Globals->interval_per_tick)
		pCmd->sidemove = s_move;

		strafe = rt;
		} */
	}
}

Vector GetAutostrafeView()
{
	return AutoStrafeView;
}

// �e սʿ
void CMiscHacks::ChatSpamInterwebz()
{
	static clock_t start_t = clock();
	double timeSoFar = (double)(clock() - start_t) / CLOCKS_PER_SEC;
	if (timeSoFar < 0.001)
		return;

	static bool wasSpamming = true;
	//static std::string nameBackup = "INTERWEBZ";

	if (wasSpamming)
	{
		static bool useSpace = true;
		if (useSpace)
		{
			change_name ("SUICIDE~");
			useSpace = !useSpace;
		}
		else
		{
			change_name("~SUICIDE");
			useSpace = !useSpace;
		}
	}

	start_t = clock();
}

void CMiscHacks::Crutcke(CUserCmd *pCmd)
{
	int Circle = Menu::Window.MiscTab.OtherCircleStrafeKey.GetKey();
	if (!GUI.GetKeyState(Circle))
	{

		IClientEntity* pLocal = hackManager.pLocal();
		static bool bDirection = true;

		bool bKeysPressed = true;

		if (GUI.GetKeyState(0x41) || GUI.GetKeyState(0x57) || GUI.GetKeyState(0x53) || GUI.GetKeyState(0x44))
			bKeysPressed = false;
		//if (pCmd->buttons & IN_ATTACK)
		//	bKeysPressed = false;

		float flYawBhop = 0.f;
		if (pLocal->GetVelocity().Length() > 45.f)
		{
			float x = 30.f, y = pLocal->GetVelocity().Length(), z = 0.f, a = 0.f;

			z = x / y;
			z = fabsf(z);

			a = x * z;

			flYawBhop = a;
		}

		if ((GetAsyncKeyState(VK_SPACE) && !(pLocal->GetFlags() & FL_ONGROUND)) && bKeysPressed)
		{

			if (bDirection)
			{
				AutoStrafeView -= flYawBhop;
				GameUtils::NormaliseViewAngle(AutoStrafeView);
				pCmd->sidemove = -450;
				bDirection = false;
			}
			else
			{
				AutoStrafeView += flYawBhop;
				GameUtils::NormaliseViewAngle(AutoStrafeView);
				pCmd->sidemove = 430;
				bDirection = true;
			}

			if (pCmd->mousedx < 0)
			{
				if (pCmd->viewangles.x > 178 || pCmd->viewangles.x < -178)
				{
					//pCmd->forwardmove = -22;
				}
				else
				{
					//pCmd->forwardmove = 22;
				}
				pCmd->sidemove = -450;
			}

			if (pCmd->mousedx > 0)
			{
				if (pCmd->viewangles.x > 178 || pCmd->viewangles.x < -178)
				{
					//pCmd->forwardmove = -22;
				}
				else
				{
					//pCmd->forwardmove = +22;
				}
				pCmd->sidemove = 450;
			}
		}
	}
}


void CMiscHacks::ChatSpamDisperseName()
{
	static clock_t start_t = clock();
	double timeSoFar = (double)(clock() - start_t) / CLOCKS_PER_SEC;
	if (timeSoFar < 0.001)
		return;

	static bool wasSpamming = true;
	//static std::string nameBackup = "INTERWEBZ";

	if (wasSpamming)
	{
		change_name("\n�e�e�e\n");
	}

	start_t = clock();
}

void CMiscHacks::Grenade()
{
	ConVar* granade = Interfaces::CVar->FindVar("sv_grenade_trajectory");
	SpoofedConvar* granade_spoofed = new SpoofedConvar(granade);

	if (Menu::Window.VisualsTab.greanadeesp.GetState())
	{
		granade_spoofed->SetInt(1);
	}
	else
	{
		granade_spoofed->SetInt(0);
	}
}


void CMiscHacks::ChatSpamName()
{
	static clock_t start_t = clock();
	double timeSoFar = (double)(clock() - start_t) / CLOCKS_PER_SEC;
	if (timeSoFar < 0.001)
		return;

	std::vector < std::string > Names;

	for (int i = 0; i < Interfaces::EntList->GetHighestEntityIndex(); i++)
	{
		// Get the entity
		IClientEntity *entity = Interfaces::EntList->GetClientEntity(i);

		player_info_t pInfo;
		// If it's a valid entity and isn't the player
		if (entity && hackManager.pLocal()->GetTeamNum() == entity->GetTeamNum() && entity != hackManager.pLocal())
		{
			ClientClass* cClass = (ClientClass*)entity->GetClientClass();

			// If entity is a player
			if (cClass->m_ClassID == (int)CSGOClassID::CCSPlayer)
			{
				if (Interfaces::Engine->GetPlayerInfo(i, &pInfo))
				{
					if (!strstr(pInfo.name, "GOTV"))
						Names.push_back(pInfo.name);
				}
			}
		}
	}

	static bool wasSpamming = true;
	//static std::string nameBackup = "INTERWEBZ.CC";

	int randomIndex = rand() % Names.size();
	char buffer[128];
	sprintf_s(buffer, "%s ", Names[randomIndex].c_str());

	if (wasSpamming)
	{
		change_name(buffer);
	}
	else
	{
		change_name ("~");
	}

	start_t = clock();
}


void CMiscHacks::ChatSpamRegular()
{
	// Don't spam it too fast so you can still do stuff
	static clock_t start_t = clock();
	int spamtime = Menu::Window.MiscTab.OtherChatDelay.GetValue();
	double timeSoFar = (double)(clock() - start_t) / CLOCKS_PER_SEC;
	if (timeSoFar < spamtime)
		return;

	static bool holzed = true;

	if (Menu::Window.MiscTab.OtherTeamChat.GetState())
	{
		SayInTeamChat("SUICIDE - EZ KATA");
	}
	else
	{
		SayInChat("SUICIDE - EZ KATA");
	}

	start_t = clock();
}

void CMiscHacks::Fakelag(CUserCmd *pCmd, bool &bSendPacket)
{
	int iChoke = Menu::Window.MiscTab.FakeLagChoke.GetValue();

	static int iFakeLag = -1;
	iFakeLag++;

	if (iFakeLag <= iChoke && iFakeLag > -1)
	{
		bSendPacket = false;
	}
	else
	{
		bSendPacket = true;
		iFakeLag = -1;
	}
}