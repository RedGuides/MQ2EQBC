/***************************************************************/
/* Version 1.0 by
/*
/*
/* Version 14.1014 by EqMule
/* -Added /bcg support for sending everyone in group a comand or some text
/* Version 15.0503 by EqMule
/* -Added /bcga support for sending everyone in group including yourself a comand or some text
// v16.0 - Eqmule 07-22-2016 - Added string safety.
// v16.1 - Eqmule 07-27-2016 - Fixed a crash that would occur when using channels.
// v16.2 - Eqmule 01-29-2017 - Fixed a crash that would occur when doing /bcaa //tar %t
// v16.21 - jimbob added BCI request for eqbci
// v16.3 - plure Made it so other plugins can check if someone is connected to your eqbc server
// v16.4 - Sym added SaveConnectByChar setting to autoconnect to different servers/ports per character
// v16.4 - Chatwiththisname - No version change, made it so TLO EQBC.Names is updated anytime someone connects or disconnects.
// v16.5 - Redbot 5-30-2018 added "NameAnnounce" setting to hide name spam.
// v19.0605 - jimbob - Added "Silent" commands for /bc commands - /bcsa, /bcsaa, /bcsg, /bcsga, /bcst
//          - Also changed MODULE_VERSION to be YY.MMDD of latest change.
//			- Also added Watch Raid Say option, 'cause Why not?!
/***************************************************************/


#pragma comment(lib,"wsock32.lib")
#include "../MQ2Plugin.h"
using namespace std;
#include <vector>
const char*        MODULE_NAME        = "MQ2EQBC";
const double       MODULE_VERSION     = 19.0605;
PreSetup(MODULE_NAME);
PLUGIN_VERSION(MODULE_VERSION);

// --------------------------------------
// constants
const char*        PROG_VERSION       = "MQ2EQBC 16.600";
const char*        CONNECT_START      = "LOGIN";
const char*        CONNECT_START2     = "=";
const char*        CONNECT_END        = ";";
const char*        CONNECT_PWSEP      = ":";
const char*        SEND_LINE_TERM     = "\n";
const unsigned int MAX_READBUF        = 2048;

const unsigned int COMMAND_HIST_SIZE  = 50;
const int          MAX_PASSWORD       = 40; // do not change without checking out the cmd buffer in eqbcs

									  // consistency
#define COLOR_NAME "\ay"
#define COLOR_NAME_BRACKET "\ar"
#define COLOR_OFF "\ax"
#define COLOR_STELL1 "\ax\ar[(\ax\aymsg\ax\ar)\ax\ay"
#define COLOR_STELL2 "\ax\ar]\ax "

									  // eqbcs msg types
#define CMD_DISCONNECT "\tDISCONNECT\n"
#define CMD_NAMES "\tNAMES\n"
#define CMD_PONG "\tPONG\n"
#define CMD_MSGALL "\tMSGALL\n"
#define CMD_TELL "\tTELL\n"
#define CMD_CHANNELS "\tCHANNELS\n"
#define CMD_LOCALECHO "\tLOCALECHO "
#define CMD_BCI "\tBCI\n"
#define CMD_STELL "\tSTELL\n"
#define CMD_SMSGALL "\tSMSGALL\n"


// commands & settings
const char* VALID_COMMANDS     = "connect quit help status reconnect names version colordump channels stopreconnect forceconnect iniconnect";
const char* VALID_SETTINGS     = "autoconnect control compatmode window reconnectsecs localecho tellwatch guildwatch groupwatch fswatch silentcmd savebychar silentinccmd notifycontrol silentoutmsg echoall";
const char* szCmdConnect       = "connect";
const char* szCmdDisconnect    = "quit";
const char* szCmdHelp          = "help";
const char* szCmdStatus        = "status";
const char* szCmdReconnect     = "reconnect";
const char* szCmdNames         = "names";
const char* szCmdRelog         = "relog";
const char* szCmdVersion       = "version";
const char* szCmdColorDump     = "colordump";
const char* szCmdChannels      = "channels";
const char* szCmdNoReconnect   = "stopreconnect";
const char* szCmdForceConnect  = "forceconnect";
const char* szCmdIniConnect    = "iniconnect";
const char* szSetReconnect     = "reconnectsecs";
const char* szSetAutoConnect   = "autoconnect";
const char* szSetControl       = "control";
const char* szSetCompatMode    = "compatmode";
const char* szSetAutoReconnect = "reconnect";
const char* szSetWindow        = "window";
const char* szSetLocalEcho     = "localecho";
const char* szSetTellWatch     = "tellwatch";
const char* szSetRaidWatch     = "raidwatch";
const char* szSetGuildWatch    = "guildwatch";
const char* szSetGroupWatch    = "groupwatch";
const char* szSetFSWatch       = "fswatch";
const char* szSetSilentCmd     = "silentcmd";
const char* szSetSaveByChar    = "savebychar";
const char* szSetSilentIncCmd  = "silentinccmd";
const char* szSetSilentOutMsg  = "silentoutmsg";
const char* szSetNotifyControl = "notifycontrol";
const char* szSetEchoAll       = "echoall";
const char* szSetNameAnnounce  = "nameannounce";
const char* szSetSaveConnectByChar = "saveconnectbychar";

// --------------------------------------
// strings
char szPassword[MAX_PASSWORD]   = {0};
char szServer[MAX_STRING]       = {0};
char szPort[MAX_STRING]         = {0};
char szToonName[MAX_STRING]     = {0};
char szCharName[MAX_STRING]     = {0};
char szToonCmdStart[MAX_STRING] = {0};
char szColorChars[]             = "yogurtbmpwx";

// --------------------------------------
// winsock

int                iNRet = 0;
CRITICAL_SECTION   ConnectCS;
sockaddr_in        serverInfo;
SOCKET             theSocket;

std::list<std::string>connectedcharacters; // Will be used so other plugins can determine who is connected to the eqbc server
CHAR szConnectedChars[4096] = { 0 };
bool bGotNames = false;
// --------------------------------------
// class instances
class EQBCType*        pEQBCType = NULL;
class CSettingsMgr*          SET = NULL;
class CEQBCWndHandler*    WINDOW = NULL;
class CConnectionMgr*       EQBC = NULL;

// --------------------------------------
// function prototypes
typedef void (__cdecl *fNetBotOnMsg)(char*, char*);
typedef void (__cdecl *fNetBotOnEvent)(char*);
void WriteOut(char* szText);
unsigned long __stdcall EQBCConnectThread(void* lpParam);

// --------------------------------------
// utility functions

inline bool ValidIngame()
{
	// CTD prevention function
	PSPAWNINFO pChSpawn = (PSPAWNINFO)pCharSpawn;
	if (GetGameState() != GAMESTATE_INGAME || !pLocalPlayer || !pChSpawn->SpawnID)
	{
		return false;
	}
	return true;
}

inline char* GetCharName()
{
	PCHARINFO pChar = GetCharInfo();
	if (GetGameState() != GAMESTATE_INGAME || !pChar || !pChar->Name) return NULL;
	return pChar->Name;
}

inline void SetPlayer()
{
	char* pszName = GetCharName();
	strcpy_s(szToonName, pszName ? pszName : "YouPlayer");
	sprintf_s(szToonCmdStart, "%s //", szToonName);
}

// --------------------------------------
// Configuration

class CSettingsMgr
{
public:
	int AllowControl;
	int AutoConnect;
	int AutoReconnect;
	int CustTitle;
	int IRCMode;
	int EchoAll;
	int LocalEcho;
	int NotifyControl;
	int ReconnectSecs;
	int SaveByChar;
	int SetTitle;
	int SilentCmd;
	int SilentIncCmd;
	int SilentOutMsg;
	int WatchFsay;
	int WatchGroup;
	int WatchGuild;
	int WatchTell;
	int WatchRaid;
	int Window;
	int SaveConnectByChar;
	int NameAnnounce;

	char WndKey[MAX_STRING];
	bool Loaded;
	bool FirstLoad;

	void LoadINI()
	{
		char szTemp[MAX_STRING] = { 0 };
        AllowControl  = GetPrivateProfileInt("Settings", "AllowControl",           1, INIFileName);
        AutoConnect   = GetPrivateProfileInt("Settings", "AutoConnect",            0, INIFileName);
        AutoReconnect = GetPrivateProfileInt("Settings", "AutoReconnect",          1, INIFileName);
        IRCMode       = GetPrivateProfileInt("Settings", "IRCCompatMode",          1, INIFileName);
        LocalEcho     = GetPrivateProfileInt("Settings", "LocalEcho",              1, INIFileName);
        EchoAll       = GetPrivateProfileInt("Settings", "EchoAll",                0, INIFileName);
        NotifyControl = GetPrivateProfileInt("Settings", "NotifyControl",          0, INIFileName);
        ReconnectSecs = GetPrivateProfileInt("Settings", "ReconnectRetrySeconds", 15, INIFileName);
        SaveByChar    = GetPrivateProfileInt("Settings", "SaveByCharacter",        1, INIFileName);
        SilentCmd     = GetPrivateProfileInt("Settings", "SilentCmd",              0, INIFileName);
        SilentIncCmd  = GetPrivateProfileInt("Settings", "SilentIncCmd",           0, INIFileName);
        SilentOutMsg  = GetPrivateProfileInt("Settings", "SilentOutMsg",           0, INIFileName);
        WatchFsay     = GetPrivateProfileInt("Settings", "FSWatch",                0, INIFileName);
        WatchGroup    = GetPrivateProfileInt("Settings", "GroupWatch",             0, INIFileName);
        WatchGuild    = GetPrivateProfileInt("Settings", "GuildWatch",             0, INIFileName);
        WatchTell     = GetPrivateProfileInt("Settings", "TellWatch",              0, INIFileName);
		WatchRaid     = GetPrivateProfileInt("Settings", "RaidWatch",              0, INIFileName);
		Window        = GetPrivateProfileInt("Settings", "UseWindow",              0, INIFileName);
		SaveConnectByChar = GetPrivateProfileInt("Settings", "SaveConnectByCharacter", 1, INIFileName);
		NameAnnounce = GetPrivateProfileInt("Settings", "NameAnnounce", 1, INIFileName);

		GetPrivateProfileString(SET->SaveConnectByChar ? szCharName : "Last Connect", "Server", "127.0.0.1", szServer, MAX_STRING, INIFileName);
		GetPrivateProfileString(SET->SaveConnectByChar ? szCharName : "Last Connect", "Port", "2112", szPort, MAX_STRING, INIFileName);

		GetPrivateProfileString("Settings", "Keybind", "~", WndKey, MAX_STRING, INIFileName);
		KeyCombo Combo;
		ParseKeyCombo(WndKey, Combo);
		SetMQ2KeyBind("EQBC", FALSE, Combo);
		Loaded = true;
	};

	void UpdateServer()
	{
		WritePrivateProfileString(SET->SaveConnectByChar ? szCharName : "Last Connect", "Server", szServer, INIFileName);
		WritePrivateProfileString(SET->SaveConnectByChar ? szCharName : "Last Connect", "Port", szPort, INIFileName);
	};

	void Change(char* szSetting, bool bToggle)
	{
        char szMsg[MAX_STRING]   = {0};
        char szArg[MAX_STRING] = {0};
        char szState[MAX_STRING] = {0};
        bool bFailed             = false;
        int  bTurnOn             = FALSE;

		GetArg(szArg, szSetting, 1);
		if (!*szArg)
		{
			bFailed = true;
		}
		else if (!_strnicmp(szArg, szSetReconnect, sizeof(szSetReconnect)))
		{
			char szDigit[MAX_STRING] = { 0 };
			GetArg(szDigit, szSetting, 2);
			if (*szDigit && atoi(szDigit) > 0)
			{
				ReconnectSecs = atoi(szDigit);
				sprintf_s(szMsg, "%d", ReconnectSecs);
				WritePrivateProfileString("Settings", "ReconnectRetrySeconds", szMsg, INIFileName);
				sprintf_s(szMsg, "\ar#\ax Will now try to reconnect every %d seconds after server disconnect.", ReconnectSecs);
			}
			else
			{
				sprintf_s(szMsg, "\ar#\ax Invalid value given - proper example: /bccmd set reconnectsecs 15");
			}
			WriteOut(szMsg);
			return;
		}

		if (!bFailed && !bToggle)
		{
			GetArg(szState, szSetting, 2);
			if (!_strnicmp(szState, "on", 3))
			{
				bTurnOn = TRUE;
			}
			else if (!_strnicmp(szState, "off", 4))
			{
				bTurnOn = FALSE; // validate input
			}
			else
			{
				sprintf_s(szMsg, "\ar#\ax Invalid 'set %s' syntax (\ar%s\ax) -- Use [ \ayon\ax | \ayoff\ax ]", szArg, szState);
				WriteOut(szMsg);
				return;
			}
		}

		if (!bFailed)
		{
			if (!_strnicmp(szArg, szSetAutoConnect, sizeof(szArg)))
			{
				ToggleSetting(&AutoConnect, &bToggle, &bTurnOn, "AutoConnect", "Auto Connect");
			}
			else if (!_strnicmp(szArg, szSetControl, sizeof(szArg)))
			{
				ToggleSetting(&AllowControl, &bToggle, &bTurnOn, "AllowControl", "Allow Control");
			}
			else if (!_strnicmp(szArg, szSetCompatMode, sizeof(szArg)))
			{
				ToggleSetting(&IRCMode, &bToggle, &bTurnOn, "IRCCompatMode", "IRC Compat Mode");
			}
			else if (!_strnicmp(szArg, szSetReconnect, sizeof(szArg)))
			{
				ToggleSetting(&AutoReconnect, &bToggle, &bTurnOn, "AutoReconnect", "Auto Reconnect (on remote disconnect)");
			}
			else if (!_strnicmp(szArg, szSetWindow, sizeof(szArg)))
			{
				if (ToggleSetting(&Window, &bToggle, &bTurnOn, "UseWindow", "Use Dedicated Window"))
				{
					HandleWnd(true);
				}
				else
				{
					HandleWnd(false);
				}
			}
			else if (!_strnicmp(szArg, szSetLocalEcho, sizeof(szArg)))
			{
				ToggleSetting(&LocalEcho, &bToggle, &bTurnOn, "LocalEcho", "Echo my channel commands back to me");
				HandleEcho();
			}
			else if (!_strnicmp(szArg, szSetEchoAll, sizeof(szArg)))
			{
				ToggleSetting(&EchoAll, &bToggle, &bTurnOn, "EchoAll", "Echo outgoing /bca messages");
			}
			else if (!_strnicmp(szArg, szSetFSWatch, sizeof(szArg)))
			{
				ToggleSetting(&WatchFsay, &bToggle, &bTurnOn, "FSWatch", "Relay fellowship chat to /bc");
			}
			else if (!_strnicmp(szArg, szSetGroupWatch, sizeof(szSetGroupWatch)))
			{
				ToggleSetting(&WatchGroup, &bToggle, &bTurnOn, "GroupWatch", "Relay group chat to /bc");
			}
			else if (!_strnicmp(szArg, szSetGuildWatch, sizeof(szArg)))
			{
				ToggleSetting(&WatchGuild, &bToggle, &bTurnOn, "GuildWatch", "Relay guild chat to /bc");
			}
			else if (!_strnicmp(szArg, szSetTellWatch, sizeof(szArg)))
			{
				ToggleSetting(&WatchTell, &bToggle, &bTurnOn, "TellWatch", "Relay all tells to /bc");
			}
			else if (!_strnicmp(szArg, szSetRaidWatch, sizeof(szArg)))
			{
				ToggleSetting(&WatchRaid, &bToggle, &bTurnOn, "RaidWatch", "Relay all raid to /bc");
			}
			else if (!_strnicmp(szArg, szSetSaveByChar, sizeof(szArg)))
			{
				ToggleSetting(&SaveByChar, &bToggle, &bTurnOn, "SaveByCharacter", "Save UI data by character name");
			}
			else if (!_strnicmp(szArg, szSetSaveConnectByChar, sizeof(szArg)))
			{
				ToggleSetting(&SaveByChar, &bToggle, &bTurnOn, "SaveConnectByCharacter", "Save server connection data by character name");
			}
			else if (!_strnicmp(szArg, szSetSilentCmd, sizeof(szArg)))
			{
				ToggleSetting(&SilentCmd, &bToggle, &bTurnOn, "SilentCmd", "Silence 'CMD: [command]' echo");
			}
			else if (!_strnicmp(szArg, szSetNameAnnounce, sizeof(szArg)))
			{
				ToggleSetting(&NameAnnounce, &bToggle, &bTurnOn, "NameAnnounce", "Announce names on connect and disconnect");
			}
			else if (!_strnicmp(szArg, szSetSilentIncCmd, sizeof(szArg)))
			{
				ToggleSetting(&SilentIncCmd, &bToggle, &bTurnOn, "SilentIncCmd", "Silence incoming commands");
			}
			else if (!_strnicmp(szArg, szSetSilentOutMsg, sizeof(szArg)))
			{
				ToggleSetting(&SilentOutMsg, &bToggle, &bTurnOn, "SilentOutMsg", "Silence outgoing msgs");
			}
			else if (!_strnicmp(szArg, szSetNotifyControl, sizeof(szArg)))
			{
				ToggleSetting(&NotifyControl, &bToggle, &bTurnOn, "NotifyControl", "Notify /bc when disabled control blocks incoming command");
			}
			else
			{
				bFailed = true;
			}
		}

		if (!bFailed) return;
		sprintf_s(szMsg, "\ar#\ax Unsupported parameter. Valid options: %s", VALID_SETTINGS);
		WriteOut(szMsg);
	};

	CSettingsMgr()
	{
        AllowControl  = 1;
        AutoConnect   = 0;
        AutoReconnect = 1;
        CustTitle     = 0;
        EchoAll       = 0;
        IRCMode       = 1;
        LocalEcho     = 1;
        NotifyControl = 0;
        ReconnectSecs = 15;
        SaveByChar    = 1;
        SetTitle      = 0;
        SilentCmd     = 0;
        SilentIncCmd  = 0;
        SilentOutMsg  = 0;
        WatchFsay     = 0;
        WatchGroup    = 0;
        WatchGuild    = 0;
        WatchTell     = 0;
		WatchRaid     = 0;
		Window        = 0;
		SaveConnectByChar = 1;
		NameAnnounce  = 1;
		memset(&WndKey, 0, MAX_STRING);
		FirstLoad     = false;
        Loaded        = false;
	};

private:
	int ToggleSetting(int* pbOption, bool* pbToggle, int* pbTurnOn, char* szOptName, char* szOptDesc)
	{
		char szTemp[MAX_STRING] = { 0 };
		*pbOption = *pbToggle ? (*pbOption ? FALSE : TRUE) : *pbTurnOn;
		sprintf_s(szTemp, "\ar#\ax Setting: %s turned %s", szOptDesc, (*pbOption) ? "ON" : "OFF");
		WriteOut(szTemp);
		sprintf_s(szTemp, "%d", *pbOption);
		WritePrivateProfileString("Settings", szOptName, szTemp, INIFileName);
		return *pbOption;
	};

	void HandleWnd(bool);
	void HandleEcho();
};

// --------------------------------------
// Custom UI Window

class CEQBCWnd : public CCustomWnd
{
public:
	CTextEntryWnd*     InputBox;
	CStmlWnd*          StmlOut;
	CXWnd*             OutWnd;
	struct _CSIDLWND*  OutStruct;

	CEQBCWnd(CXStr& Template) : CCustomWnd(Template)
    {
        iCurCommand = -1;
        SetWndNotification(CEQBCWnd);
        StmlOut = (CStmlWnd *)GetChildItem("CW_ChatOutput");
		RemoveStyle(CWS_TRANSPARENT);
		AddStyle(CWS_RESIZEBORDER | CWS_TITLE | CWS_MINIMIZE);

        SetBGColor(0xFF000000);//black background
        OutWnd = (CXWnd*)StmlOut;
        OutWnd->SetClickable(1);
        OutStruct = (_CSIDLWND *)GetChildItem("CW_ChatOutput");
        InputBox = (CTextEntryWnd*)GetChildItem("CW_ChatInput");
        InputBox->AddStyle(0x800C0);
        InputBox->SetCRNormal(0xFFFFFFFF);
        SetEscapable(0);
        InputBox->SetMaxChars(512);
        RemoveStyle(CWS_CLOSE);
        //        *(unsigned long*)&(((char*)StmlOut)[EQ_CHAT_HISTORY_OFFSET]) = 400;
        StmlOut->MaxLines = 400;
    };

	int WndNotification(CXWnd* pWnd, unsigned int uiMessage, void* pData)
	{
		//static char szBCAA[] = "/bcaa "; // trailing space intentional
		if (pWnd == (CXWnd*)InputBox)
		{
			if (uiMessage == XWM_HITENTER)
			{
				char szBuffer[MAX_STRING] = { 0 };
				GetCXStr((PCXSTR)InputBox->InputText, szBuffer, MAX_STRING);
				if (szBuffer[0])
				{
					if (!sCmdHistory.size() || sCmdHistory.front().compare(szBuffer))
					{
						if (sCmdHistory.size() > COMMAND_HIST_SIZE)
						{
							sCmdHistory.pop_back();
						}
						sCmdHistory.insert(sCmdHistory.begin(), string(szBuffer));
					}
					iCurCommand = -1;
					SetCXStr(&InputBox->InputText, "");
					if (szBuffer[0] == '/')
					{
						DoCommand((PSPAWNINFO)pLocalPlayer, szBuffer);
					}
					else
					{
						WriteToBC(szBuffer);
					}
				}
				((CXWnd*)InputBox)->ClrFocus();
				ResetKeybinds();
			}
			else if (uiMessage == XWM_HISTORY && pData)
			{
				int* pInt      = (int*)pData;
				int  iKeyPress = pInt[1];
				if (iKeyPress == 200) // KeyUp: 0xC8
				{
					if (sCmdHistory.size() > 0)
					{
						iCurCommand++;
						if (iCurCommand < (int)sCmdHistory.size() && iCurCommand >= 0)
						{
							string s = (string)sCmdHistory.at(iCurCommand);
							((CXWnd*)InputBox)->SetWindowTextA(CXStr(s.c_str()));
						}
						else
						{
							iCurCommand = (int)sCmdHistory.size() - 1;
						}
					}
					ResetKeybinds();
				}
				else if (iKeyPress == 208) // KeyDown: 0xD0
				{
					if (sCmdHistory.size() > 0)
					{
						iCurCommand--;
						if (iCurCommand >= 0 && sCmdHistory.size() > 0)
						{
							string s = (string)sCmdHistory.at(iCurCommand);
							((CXWnd*)InputBox)->SetWindowTextA(CXStr(s.c_str()));
						}
						else if (iCurCommand < 0)
						{
							iCurCommand = -1;
							// Hit bottom.
							SetCXStr(&InputBox->InputText, "");
						}
					}
					ResetKeybinds();
				}
			}
		}
		else if (pWnd == NULL && uiMessage == XWM_CLOSE)
		{
			SetVisible(true);
			ResetKeybinds();
			return 0;
		}
		else if (uiMessage == XWM_LINK)
		{
			class CChatWindow* p = (class CChatWindow*)this;
			if (StmlOut != (CStmlWnd*)pWnd)
			{
				CStmlWnd* pTmp = NULL;
				int iRet = 0;
				pTmp = StmlOut;
				StmlOut = (CStmlWnd*)pWnd;
				iRet = p->WndNotification(pWnd, uiMessage, pData);
				StmlOut = pTmp;
				return iRet;
			}
			return p->WndNotification(pWnd, uiMessage, pData);
		}
		return CSidlScreenWnd::WndNotification(pWnd, uiMessage, pData);
	};
private:
	vector<string> sCmdHistory;
	int            iCurCommand;

	void WriteToBC(char*);
	void ResetKeybinds();
};
template <unsigned int _Size>LPSTR SafeItoa(int _Value, char(&_Buffer)[_Size], int _Radix)
{
	errno_t err = _itoa_s(_Value, _Buffer, _Radix);
	if (!err) {
		return _Buffer;
	}
	return "";
}
class CEQBCWndHandler
{
public:
	void Create()
	{
		if (!SET->Window || !ValidIngame() || BCWnd)
			return;
		NewWnd();
	};

	void Destroy(bool bSave)
	{
		if (!BCWnd)
			return;
		if (bSave)
			SaveWnd();
		delete BCWnd;
		BCWnd = NULL;
	};

	void Clear()
	{
		if (!BCWnd) return;
		((CChatWindow*)BCWnd)->Clear();
	};

	void Hover()
	{
		if (!BCWnd) return;
		((CXWnd*)BCWnd)->DoAllDrawing();
	};

	void Min()
	{
		if (!BCWnd) return;
		((CXWnd*)BCWnd)->OnMinimizeBox();
	};

	void Save()
	{
		if (!BCWnd) return;
		SaveWnd();
	};

	void Write(char* szText)
	{
		if (!BCWnd) return;
		Output(szText);
	};

	void NewFont(int iSize)
	{
		if (!BCWnd || iSize < 0) return;
		SetFontSize(iSize);
	};

	void UpdateTitle()
	{
		if (!BCWnd || SET->CustTitle) return;
		char szWindowText[MAX_STRING] = { 0 };
		GetCXStr(BCWnd->CGetWindowText(), szWindowText);
		if (_strnicmp(szWindowText, szServer, sizeof(szWindowText)))
		{
			BCWnd->CSetWindowText(szServer);
			//SetCXStr(&BCWnd->WindowText, szServer);
		}
	};

	void Keybind(int iDown)
	{
		if (!ValidIngame() || !BCWnd) return;
		if (iDown)
		{
			if (!KeyActive)
			{
				CXRect rect = ((CXWnd*)BCWnd->InputBox)->GetScreenRect();
				CXPoint pt  = rect.CenterPoint();
				((CXWnd*)BCWnd->InputBox)->SetWindowTextA(CXStr(""));
				((CXWnd*)BCWnd->InputBox)->HandleLButtonDown(&pt, 0);
				KeyActive = true;
				return;
			}
			SetCXStr(&BCWnd->InputBox->InputText, "");
			((CXWnd*)BCWnd->InputBox)->ClrFocus();
			KeyActive = false;
		}
	};

	void ResetKeys()
	{
		KeyActive = false;
	};

	CEQBCWndHandler()
	{
		BCWnd = NULL;
		FontSize = 4;
		KeyActive = false;
	};
private:
	void NewWnd()
	{
		char szWindowText[MAX_STRING] = { 0 };
		sprintf_s(szWindowText, "%s", szServer);
		BCWnd = new CEQBCWnd(CXStr("ChatWindow"));

        SET->CustTitle         = GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "UseMyTitle",   0,    INIFileName);
		//left top right bottom
		BCWnd->SetLocation({ (LONG)GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "ChatLeft",     10,   INIFileName),
			(LONG)GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "ChatTop",      10,   INIFileName),
			(LONG)GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "ChatRight",    410,  INIFileName),
			(LONG)GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "ChatBottom",   210,  INIFileName) });

        BCWnd->SetFades((GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Fades",        0,    INIFileName) ? true:false));
        BCWnd->SetAlpha(GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Alpha",        255,  INIFileName));
        BCWnd->SetFadeToAlpha(GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "FadeToAlpha",  255,  INIFileName));
        BCWnd->SetFadeDuration(GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Duration",     500,  INIFileName));
		BCWnd->SetLocked((GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Locked", 0, INIFileName) ? true:false));
        BCWnd->SetFadeDelay(GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "Delay",        2000, INIFileName));
        BCWnd->SetBGType(GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGType",       1,    INIFileName));
		ARGBCOLOR col = { 0 };
		col.A				   = GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGTint.alpha", 255, INIFileName);
		col.R				   = GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGTint.red", 0, INIFileName);
        col.G			       = GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGTint.green", 0,  INIFileName);
        col.B			       = GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "BGTint.blue",  0,  INIFileName);

		BCWnd->SetBGColor(col.ARGB);
		NewFont(GetPrivateProfileInt(SET->SaveByChar ? szCharName : "Window", "FontSize", 4, INIFileName));
		if (SET->CustTitle)
		{
			GetPrivateProfileString(SET->SaveByChar ? szCharName : "Window", "WindowTitle", szWindowText, szWindowText, MAX_STRING, INIFileName);
		}
		BCWnd->CSetWindowText(szWindowText);
		//SetCXStr(&BCWnd->WindowText, szWindowText);
		((CXWnd*)BCWnd)->Show(1, 1);
		BCWnd->OutStruct->RemoveStyle(CWS_CLOSE);
		//BitOff(BCWnd->OutStruct->WindowStyle, CWS_CLOSE);
	};

	void SaveWnd()
	{
		//return;
		PCSIDLWND UseWnd = (PCSIDLWND)BCWnd;
        char szTemp[2048]              = {0};

        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "ChatTop",      SafeItoa(UseWnd->GetLocation().top,    szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "ChatBottom",   SafeItoa(UseWnd->GetLocation().bottom, szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "ChatLeft",     SafeItoa(UseWnd->GetLocation().left,   szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "ChatRight",    SafeItoa(UseWnd->GetLocation().right,  szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "Fades",        SafeItoa(UseWnd->GetFades(),           szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "Alpha",        SafeItoa(UseWnd->GetAlpha(),           szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "FadeToAlpha",  SafeItoa(UseWnd->GetFadeToAlpha(),     szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "Duration",     SafeItoa(UseWnd->GetFadeDuration(),    szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "Locked",       SafeItoa(UseWnd->IsLocked(),          szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "Delay",        SafeItoa(UseWnd->GetFadeDelay(),   szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "BGType",       SafeItoa(UseWnd->GetBGType(),          szTemp, 10), INIFileName);

		ARGBCOLOR col = { 0 };
		col.ARGB = UseWnd->GetBGColor();
		WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "BGTint.alpha",   SafeItoa(col.A,       szTemp, 10), INIFileName);
		WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "BGTint.red",   SafeItoa(col.R,       szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "BGTint.green", SafeItoa(col.G,       szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "BGTint.blue",  SafeItoa(col.B,       szTemp, 10), INIFileName);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "FontSize",     SafeItoa(FontSize,                szTemp, 10), INIFileName);
        GetCXStr(UseWnd->CGetWindowText(), szTemp,MAX_STRING);
        WritePrivateProfileString(SET->SaveByChar ? szCharName : "Window", "WindowTitle",                                szTemp,      INIFileName);
	};

	void Output(char* szText)
	{
		((CXWnd*)BCWnd)->Show(1, 1);
		bool bScrollDown = (BCWnd->OutWnd->GetVScrollPos() == BCWnd->OutWnd->GetVScrollMax()) ? true : false;
		char szProcessed[MAX_STRING] = { 0 };
		StripMQChat(szText, szProcessed);
		CheckChatForEvent(szProcessed);
		MQToSTML(szText, szProcessed, MAX_STRING);
		strcat_s(szProcessed, "<br>");
		CXStr NewText(szProcessed);
		ConvertItemTags(NewText, TRUE);
		(BCWnd->StmlOut)->AppendSTML(NewText);
		if (bScrollDown) (BCWnd->OutWnd)->SetVScrollPos(BCWnd->OutStruct->GetVScrollMax());
	};

	void SetFontSize(unsigned int uiSize)
	{
		struct FONTDATA
		{
			unsigned long ulNumFonts;
			char**        ppFonts;
		};
		FONTDATA*      pFonts;       // font array structure
		unsigned long* pulSelFont;   // selected font
		pFonts = (FONTDATA*)&(((char*)pWndMgr)[EQ_CHAT_FONT_OFFSET]);
		if (!pFonts->ppFonts || uiSize >= (int)pFonts->ulNumFonts)
		{
			return;
		}
		pulSelFont = (unsigned long*)pFonts->ppFonts[uiSize];

		CXStr ContStr(((CStmlWnd*)BCWnd->StmlOut)->GetSTMLText());
        ((CXWnd*)   BCWnd->StmlOut)->SetFont(pulSelFont);
        ((CStmlWnd*)BCWnd->StmlOut)->SetSTMLText(ContStr, 1, 0);
        ((CStmlWnd*)BCWnd->StmlOut)->ForceParseNow();
        ((CXWnd*)   BCWnd->StmlOut)->SetVScrollPos(BCWnd->StmlOut->GetVScrollMax());

		FontSize = uiSize;
	};

	CEQBCWnd*     BCWnd;
	unsigned long FontSize;
	bool          KeyActive;
};

void WriteOut(char *szText)
{
    typedef int (__cdecl *fMQWriteBC)(char *szText);
    int bWrite        = true;
	PMQPLUGIN pPlugin = pPlugins;
	while (pPlugin)
	{
		fMQWriteBC WriteBC = (fMQWriteBC)GetProcAddress(pPlugin->hModule, "OnWriteBC");
		if (WriteBC)
		{
			if (!WriteBC(szText)) bWrite = false;
		}
		pPlugin = pPlugin->pNext;
	}
	if (!bWrite) return;

	if (SET->Window)
	{
		WINDOW->Write(szText);
		return;
	}
	WriteChatColor(szText);
}

void CEQBCWnd::ResetKeybinds()
{
	WINDOW->ResetKeys();
}

// -------------------------------------------------------------
// connection management class (handles majority of plugin i/o)

class CConnectionMgr
{
public:
	bool Connected;
	bool Connecting;
	bool TriedConnect;
	ULONGLONG LastSecs;

	void NewThread()
	{
		InitializeCriticalSection(&ConnectCS);
	};

	void KillThread()
	{
		EnterCriticalSection(&ConnectCS);
		LeaveCriticalSection(&ConnectCS);
		DeleteCriticalSection(&ConnectCS);
	};

	void Transmit(bool bHandleDisconnect, char* szMsg)
	{
		if (!Connected) return;
		int iErr = 0;
		iErr = send(theSocket, szMsg, strlen(szMsg), 0);
		if (bHandleDisconnect) CheckError("Transmit:SendMsg", iErr);
		iErr = send(theSocket, SEND_LINE_TERM, strlen(SEND_LINE_TERM), 0);
		if (bHandleDisconnect) CheckError("Transmit:SendTerm", iErr);
	};

	void SendLocalEcho()
	{
		if (!Connected) return;
        int  iErr          = 0;
        char szCommand[15] = {0};
		sprintf_s(szCommand, "%s%i\n", CMD_LOCALECHO, SET->LocalEcho);
		iErr = send(theSocket, szCommand, strlen(szCommand), 0);
		CheckError("SendLocalEcho:Send1", iErr);
	};

	void BC(char* szLine)
	{
		if (!ConnectReady()) return;
		if (szLine && strlen(szLine))
		{
			Transmit(true, szLine);
		}
	};

	void BCG(char* szLine, bool silent=false)
	{
		if (!ConnectReady()) return;
		PCHARINFO pChar = GetCharInfo();
		//char szCmdBct[] = (silent == false ? CMD_STELL : CMD_TELL); // ? : in assignment won't work with char name[] type string pointer.
		char *szCmdBct = (silent ? CMD_STELL : CMD_TELL);
		if (szLine && strlen(szLine))
		{
			for (DWORD N = 1; N<6; N++) {
				if (pChar && pChar->pGroupInfo && pChar->pGroupInfo->pMember[N] && pChar->pGroupInfo->pMember[N]->Mercenary == 0) {
					CHAR Name[MAX_STRING] = { 0 };
					GetCXStr(pChar->pGroupInfo->pMember[N]->pName, Name, MAX_STRING);
					strcat_s(Name, " ");
					strcat_s(Name, szLine);
					ChanTransmit(szCmdBct, Name);
					if (SET->IRCMode && !(SET->SilentOutMsg  || silent))
					{
						char szTemp[MAX_STRING] = {0};
						int iSrc                = 0;
						int iDest               = 0;
						int iLen                = 0;

						iLen   = strlen(Name);
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_STELL1);
						while (Name[iSrc] != ' ' && Name[iSrc] != '\n' && iSrc <= iLen)
						{
							szTemp[iDest++] = Name[iSrc++];
						}
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_STELL2);
						iSrc++;
						while (iSrc <= iLen)
						{
							szTemp[iDest++] = Name[iSrc++];
						}
						szTemp[iDest] = '\n';
						WriteOut(szTemp);
					}
				}
			}
		}
	};

	void BCGA(char* szLine, bool silent=false)
	{
		if (!ConnectReady()) return;
		PCHARINFO pChar = GetCharInfo();
		//char szCmdBct[] = CMD_TELL;
		char *szCmdBct = (silent ? CMD_STELL : CMD_TELL);
		if (szLine && strlen(szLine))
		{
			for (DWORD N = 0; N<6; N++) {
				if (pChar && pChar->pGroupInfo && pChar->pGroupInfo->pMember[N] && pChar->pGroupInfo->pMember[N]->Mercenary == 0) {
					CHAR Name[MAX_STRING] = { 0 };
					GetCXStr(pChar->pGroupInfo->pMember[N]->pName, Name, MAX_STRING);
					strcat_s(Name, " ");
					strcat_s(Name, szLine);
					ChanTransmit(szCmdBct, Name);
					if (SET->IRCMode && !(SET->SilentOutMsg || silent))
					{
						char szTemp[MAX_STRING] = { 0 };
						int iSrc = 0;
						int iDest = 0;
						int iLen = 0;

						iLen = strlen(Name);
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_STELL1);
						while (Name[iSrc] != ' ' && Name[iSrc] != '\n' && iSrc <= iLen)
						{
							szTemp[iDest++] = Name[iSrc++];
						}
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_STELL2);
						iSrc++;
						while (iSrc <= iLen)
						{
							szTemp[iDest++] = Name[iSrc++];
						}
						szTemp[iDest] = '\n';
						WriteOut(szTemp);
					}
				}
			}
		}
	};

	void BCT(char* szLine, bool silent=false)
	{
		if (!ConnectReady()) return;
		//char szCmdBct[] = CMD_TELL;
		char *szCmdBct = (silent ? CMD_STELL : CMD_TELL);
		if (szLine && strlen(szLine))
		{
			ChanTransmit(szCmdBct, szLine);
			if (SET->IRCMode && !(SET->SilentOutMsg || silent))
			{
				char szTemp[MAX_STRING] = {0};
                int iSrc                = 0;
                int iDest               = 0;
                int iLen                = 0;

				iLen   = strlen(szLine);
				iDest += WriteStringGetCount(&szTemp[iDest], COLOR_STELL1);
				while (szLine[iSrc] != ' ' && szLine[iSrc] != '\n' && iSrc <= iLen)
				{
					szTemp[iDest++] = szLine[iSrc++];
				}
				iDest += WriteStringGetCount(&szTemp[iDest], COLOR_STELL2);
				iSrc++;
				while (iSrc <= iLen)
				{
					szTemp[iDest++] = szLine[iSrc++];
				}
				szTemp[iDest] = '\n';
				WriteOut(szTemp);
			}
		}
	};

	bool BCA(char* szLine, bool silent=false)
	{
		if (!ConnectReady()) return false;

		//char szCmdAll[] = CMD_MSGALL;
		char *szCmdAll = (silent ? CMD_SMSGALL : CMD_MSGALL);
		if (szLine && strlen(szLine))
		{
			if (SET->EchoAll)
			{
				char szTemp[MAX_STRING] = { 0 };
				sprintf_s(szTemp, "<%s> [+r+]([+o+]to all[+r+])[+w+] %s", ((PSPAWNINFO)pLocalPlayer)->Name, szLine);
				HandleIncomingString(szTemp, false, silent);
			}
			ChanTransmit(szCmdAll, szLine);
		}
		return true;
	};

	void BCAA(PSPAWNINFO pLPlayer, char* szLine, bool silent=false)
	{
		if (!BCA(szLine, silent)) return;

		char szTemp[MAX_STRING] = { 0 };
		sprintf_s(szTemp, "<%s> %s %s", pLPlayer->Name, pLPlayer->Name, szLine);
		HandleIncomingString(szTemp, true, silent);
	};

	void HandleChannels(PCHAR szLine, SIZE_T BufferSize)
	{
		if (!ConnectReady())
			return;

		char  szTemp1[MAX_STRING] = { 0 };
		char  szTemp2[MAX_STRING] = { 0 };
        char* szArg                = NULL;
        char  szCmdChan[]          = CMD_CHANNELS;
		char *next_token1 = NULL;
		if (char* pName = GetCharName()) {

			if (!szLine || (szLine && szLine[0] == '\0'))
			{
				GetPrivateProfileString(pName ? pName : "Settings", "Channels", "", szTemp2, MAX_STRING, INIFileName);
				_strlwr_s(szTemp2);
				szArg = strtok_s(szTemp2, " \n", &next_token1);
				if (!szArg)
					return;
			}
			else
			{
				_strlwr_s(szLine, BufferSize);
				szArg = strtok_s(szLine, " \n", &next_token1);// first token will be command CHANNELS, skip it
				szArg = strtok_s(NULL, " \n", &next_token1);
			}

			while (szArg != NULL)
			{
				strcat_s(szTemp1, szArg);
				szArg = strtok_s(NULL, " \n", &next_token1);
				if (szArg)
					strcat_s(szTemp1, " ");
			}

			if (*pName)
				WritePrivateProfileString(pName, "Channels", szTemp1, INIFileName);

			ChanTransmit(szCmdChan, szTemp1);
		}
	}
	template <unsigned int _Size>void HandleChannels(CHAR(&szLine)[_Size])
	{
		HandleChannels(szLine, _Size);
	};

	void ConnectINI(char* szName)
	{
		if (Connecting)
		{
			WriteOut("\ar#\ax Already trying to connect! Hold on a minute there...");
			return;
		}
		if (!*szName)
		{
			WriteOut("\ar#\ax No INI key name given. Aborting...");
			return;
		}
		if (Connected)
		{
			Disconnect(true);
		}

		char szMsg[MAX_STRING]    = { 0 };
		char szTemp[MAX_STRING]   = { 0 };
		char szPass[MAX_PASSWORD] = { 0 };

		SetPlayer();

		GetPrivateProfileString(szName, "Server", NULL, szTemp, MAX_STRING, INIFileName);
		if (!*szTemp)
		{
			sprintf_s(szMsg, "\ar#\ax Server for key \ay%s\ax not found.", szName);
			WriteOut(szMsg);
			return;
		}
		strcpy_s(szServer, szTemp);
		GetPrivateProfileString(szName, "Port", "2112", szTemp, MAX_STRING, INIFileName);
		strcpy_s(szPort, szTemp);
		GetPrivateProfileString(szName, "Password", "", szPass, MAX_STRING, INIFileName);
		strcpy_s(szPassword, szPass);
		DoConnectEQBCS();
	};

	void Connect(char* szLine, bool bForce)
	{
		if (Connected && !bForce)
		{
			WriteOut("\ar#\ax Already connected. Use \ag/bccmd quit\ax to disconnect first.");
			return;
		}
		if (Connecting)
		{
			WriteOut("\ar#\ax Already trying to connect! Hold on a minute there...");
			return;
		}
		if (Connected && bForce)
		{
			Disconnect(true);
		}

		char szCurArg[MAX_STRING] = { 0 };
		char szTemp[MAX_STRING] = { 0 };
		char szPass[MAX_PASSWORD] = { 0 };

		SetPlayer();

		GetArg(szCurArg, szLine, 2); // 1 was the connect statement.
		if (!*szCurArg)
		{
			GetPrivateProfileString(SET->SaveConnectByChar ? szCharName : "Last Connect", "Server", "127.0.0.1", szTemp, MAX_STRING, INIFileName);
			strcpy_s(szServer, szTemp);
			GetPrivateProfileString(SET->SaveConnectByChar ? szCharName : "Last Connect", "Port", "2112", szTemp, MAX_STRING, INIFileName);
			strcpy_s(szPort, szTemp);
			GetPrivateProfileString("Last Connect", "Password", "", szPass, 40, INIFileName);
			strcpy_s(szPassword, szPass);
		}
		else
		{
			strcpy_s(szServer, szCurArg);
			GetArg(szCurArg, szLine, 3);
			if (!*szCurArg)
			{
				GetPrivateProfileString(SET->SaveConnectByChar ? szCharName : "Last Connect", "Port", "2112", szTemp, MAX_STRING, INIFileName);
				strcpy_s(szPort, szTemp);
				GetPrivateProfileString("Last Connect", "Password", "", szPass, MAX_STRING, INIFileName);
				strcpy_s(szPassword, szPass);
			}
			else
			{
				strcpy_s(szPort, szCurArg);
				GetArg(szCurArg, szLine, 4);
				if (!*szCurArg)
				{
					GetPrivateProfileString("Last Connect", "Password", "", szPass, MAX_STRING, INIFileName);
					strcpy_s(szPassword, szPass);
				}
				else
				{
					strcpy_s(szPassword, szCurArg);
				}
			}
		}
		DoConnectEQBCS();
		return;
	};

	void HandleControlMsg(char* pRawmsg)
	{
		if (!strncmp(pRawmsg, "PING", 4))
		{
			LastPing = clock();
			Transmit(true, CMD_PONG);
		}
		else if (!strncmp(pRawmsg, "NBPKT:", 6))
		{
			SendNetBotMsg(pRawmsg + 6);
		}
		else if (!strncmp(pRawmsg, "NB", 2))
		{
			//char Output[64] = {0}; //long interface name overflow
			char szOutput[MAX_STRING] = { 0 };
			if (!strncmp(pRawmsg, "NBJOIN=", 7))
			{
				sprintf_s(szOutput, "%s", &pRawmsg[7]);
				connectedcharacters.push_back(szOutput); // Add new people to connectedcharacters when they connect to the server
				sprintf_s(szOutput, "\ar#\ax - %s has joined the server.", &pRawmsg[7]);
				WriteOut(szOutput);
				if (SET->NameAnnounce)
				{
					EQBC->Names(); //Now requests for names occurs when -anyone- connects. 
				}
			}
			if (!strncmp(pRawmsg, "NBQUIT=", 7))
			{
				sprintf_s(szOutput, "%s", &pRawmsg[7]);
				connectedcharacters.remove(szOutput); // Remove people to connectedcharacters when they leave the server
				sprintf_s(szOutput, "\ar#\ax - %s has left the server.", &pRawmsg[7]);
				WriteOut(szOutput);
				if (SET->NameAnnounce)
				{ 
					EQBC->Names(); //Now requests for names occurs when -anyone- disconnects.
				}
			}
			SendNetBotEvent(pRawmsg);
		}
	};
	template <unsigned int _Size>void HandleIncomingString(CHAR(&pRawmsg)[_Size], bool bForce, bool silent=false)
	{
		if (!pRawmsg) return;
		char szTemp[MAX_STRING] = { 0 };
		char cLastChar = 0;
		int  iSrc = 0;
		int  iDest = 0;
		int  iCharCount = -1;
		int  iMsgTell = 0;
		char cColor = 0;
		unsigned char bSysMsg = FALSE;
		char* pszCmdStart = NULL;
		bool  bBciCmd = false;

		bSysMsg = (*pRawmsg == '-') ? TRUE : FALSE;

		if (*pRawmsg == '\t')
		{
			HandleControlMsg(pRawmsg + 1);
			return;
		}

		while (pRawmsg[iSrc] != 0 && iDest < MAX_STRING - 1)
		{
			if (iSrc == 0 && bSysMsg) // first char
			{
				iDest += WriteStringGetCount(&szTemp[iDest], "\ar#\ax ");
				cLastChar = *pRawmsg;
				iCharCount = 1;
			}
			else if (cLastChar != ' ' || pRawmsg[iSrc] != ' ') // Do not add extra spaces
			{
				if (iCharCount <= 1 && _strnicmp(&pRawmsg[iSrc], szToonCmdStart, strlen(szToonCmdStart)) == 0)
				{
					pszCmdStart = &pRawmsg[iSrc + strlen(szToonCmdStart) - 1];
				}
				if (iCharCount <= 1 && iMsgTell == 1 && _strnicmp(&pRawmsg[iSrc], "//", 2) == 0)
				{
					pszCmdStart = &pRawmsg[iSrc + 1];
				}
				if (iCharCount <= 1 && bBciCmd)
				{
					pszCmdStart = &pRawmsg[iSrc];
				}
				if (iCharCount >= 0)
				{
					iCharCount++;
					// if not in cmdMode and have room, check for color code.
					if (pszCmdStart == NULL && (cColor = GetCharColor(&pRawmsg[iSrc])) != 0 && (iDest + (isupper(cColor) ? 5 : 4)) < 2047)
					{
						//DebugSpewAlways("Got Color: %c", cColor);
						unsigned char bDark = isupper(cColor);
						szTemp[iDest++] = '\a';
						if (bDark) szTemp[iDest++] = '-';
						szTemp[iDest++] = tolower(cColor);
						cLastChar = '\a'; // Something that should not exist otherwise
						iSrc += 4; // Color code format is [+x+] - 5 characters.
					}
					else
					{
						cLastChar = szTemp[iDest] = pRawmsg[iSrc];
						iDest++;
					}
				}
				else if (!bSysMsg)
				{
					switch (pRawmsg[iSrc])
					{
					case '<':
					{
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_NAME_BRACKET);
						szTemp[iDest++] = SET->IRCMode ? '<' : '>';
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_OFF);
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_NAME);
						break;
					}
					case '>':
					{
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_OFF);
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_NAME_BRACKET);
						szTemp[iDest++] = SET->IRCMode ? '>' : '<';
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_OFF);
						iCharCount = 0;
						break;
					}
					case '!': // Sneaky jimbob way to cheat silence! I don't think ! can be in the username?
					{
						silent = true;
						break;
					}
					case '[':
					{
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_NAME_BRACKET);
						szTemp[iDest++] = '[';
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_OFF);
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_NAME);
						break;
					}
					case ']':
					{
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_OFF);
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_NAME_BRACKET);
						if (SET->IRCMode)
						{
							szTemp[iDest++] = '(';
							iDest += WriteStringGetCount(&szTemp[iDest], COLOR_OFF);
							iDest += WriteStringGetCount(&szTemp[iDest], COLOR_NAME);
							iDest += WriteStringGetCount(&szTemp[iDest], "msg");
							iDest += WriteStringGetCount(&szTemp[iDest], COLOR_OFF);
							iDest += WriteStringGetCount(&szTemp[iDest], COLOR_NAME_BRACKET);
							iDest += WriteStringGetCount(&szTemp[iDest], ")]");
						}
						else
						{
							szTemp[iDest++] = ']';
						}
						iDest += WriteStringGetCount(&szTemp[iDest], COLOR_OFF);
						iCharCount = 0;
						iMsgTell = 1;
						break;
					}
					case '{':
						break;
					case '}':
						iDest++;
						iCharCount = 0;
						bBciCmd = true;
						break;
					default:
					{
						cLastChar = szTemp[iDest] = pRawmsg[iSrc];
						iDest++;
						break;
					}
					}
				}
			}
			iSrc++;
		}

		if (pszCmdStart)
		{
			if (bBciCmd)
			{
				HandleBciMsg(szTemp, pszCmdStart);
				return;
			}
			CHAR Buffer[MAX_STRING] = { 0 };
			strcpy_s(Buffer, pszCmdStart);
			HandleIncomingCmd(Buffer, bForce, silent);
			if (SET->SilentIncCmd) return;
		}
		if (char *pDest = strstr(szTemp, "- Names: ")) {//szTemp
			connectedcharacters.clear();
			//its names and we want them in our tlo so we can check who is online from a macro
			CHAR szNames[MAX_STRING] = { 0 };
			strcpy_s(szNames, &pDest[9]);
			if (pDest = strchr(szNames, '.')) {
				pDest[0] = '\0';
			}
			strcpy_s(szConnectedChars, szNames);
			while (pDest = strchr(szNames, ' ')) {
				pDest[0] = '\0';
				connectedcharacters.push_back(szNames);
				strcpy_s(szNames, &pDest[1]);
			}
			if (szNames[0] != '\0') {
				connectedcharacters.push_back(szNames);
			}
			bGotNames = true;
		}
		if(!silent)
			WriteOut(szTemp);
	};

	void Disconnect(bool bSend)
	{
		if (Connected)
		{
			SendNetBotEvent("NBEXIT");
			Connected = false;
			// Could set linger off here..
			if (bSend)
			{
				int iErr = send(theSocket, CMD_DISCONNECT, sizeof(CMD_DISCONNECT), 0);
				CheckError("Disconnect:Send", iErr);
			}
			closesocket(theSocket);
			LastReadPos = 0;
		}
	};

	void Pulse()
	{
		if (TriedConnect) ConnectStatus();

		if (Connected)
		{
			HandleBuffer();
		}
		else if (LastSecs > 0 && !Connecting)
		{
			if (LastSecs + SET->ReconnectSecs < GetTickCount642() / 1000)
			{
				LastSecs = GetTickCount642() / 1000;
				Connect("", false);
			}
		}
	};

	void Status()
	{
		char szTemp[MAX_STRING] = { 0 };
		if (Connected)
		{
			sprintf_s(szTemp, "\ar#\ax MQ2Eqbc Status: ONLINE - %s - %s", szServer, szPort);
			WriteOut(szTemp);
		}
		else
		{
			WriteOut("\ar#\ax MQ2Eqbc Status: OFFLINE");
		}
		sprintf_s(szTemp, "\ar#\ax Allow Control: %s, Auto Connect: %s", SET->AllowControl ? "ON" : "OFF", SET->AutoConnect ? "ON" : "OFF");
		WriteOut(szTemp);
		sprintf_s(szTemp, "\ar#\ax IRC Compat Mode: %s, Reconnect: %s: (every %d secs)", SET->IRCMode ? "ON" : "OFF", SET->AutoReconnect ? "ON" : "OFF", SET->ReconnectSecs);
		WriteOut(szTemp);
	};

	void Reconnect()
	{
		Disconnect(true);
		Connect("", false);
	};

	void Version()
	{
		char szTemp[MAX_STRING] = { 0 };
		sprintf_s(szTemp, "\ar#\ax %s", PROG_VERSION);
		WriteOut(szTemp);
	};

	void Names()
	{
		szConnectedChars[0] = '\0';
		bGotNames = false;
		if (!ConnectReady()) return;
		WriteOut("\ar#\ax Requesting names...");
		int iErr = send(theSocket, CMD_NAMES, strlen(CMD_NAMES), 0);
		CheckError("HandleNamesRequest:send", iErr);
	};

	void AutoConnect()
	{
		if (!SET->AutoConnect || Connected) return;
		SetPlayer();
		Connect("", false);
	};

	void Logout()
	{
		if (!Connected) return;
		Disconnect(true);
	};

	~CConnectionMgr()
	{
		Connected = false;
		Connecting = false;
		TriedConnect = false;
		SocketGone = 0;
		LastReadPos = 0;
		LastSecs = 0;
		delete pcReadBuf;
		LastPing = 0;
		usSockVersion = 0;
	}
	CConnectionMgr()
	{
		Connected = false;
		Connecting = false;
		TriedConnect = false;
		SocketGone = 0;
		LastReadPos = 0;
		LastSecs = 0;
		pcReadBuf = new char[MAX_READBUF];
		LastPing = 0;
		usSockVersion = 0;
	};

private:
	bool ConnectReady()
	{
		if (!Connected)
		{
			WriteOut("\ar#\ax You are not connected. Use \ag/bccmd connect\ax to establish a connection.");
			return false;
		}
		return true;
	};

	void CheckSocket(char* szFunc, int iErr)
	{
		int iWerr = WSAGetLastError();
		if (iWerr == WSAECONNABORTED)
		{
			SocketGone = TRUE;
		}
		WSASetLastError(0);
		// DebugSpewAlways("Sock Error-%s: %d / w%d", szFunc, err, werr);
	};

	void CheckError(char* szFunc, int iErr)
	{
		if (iErr == SOCKET_ERROR || (iErr == 0 && WSAGetLastError() != WSAEWOULDBLOCK))
		{
			CheckSocket(szFunc, iErr);
		}
	};

	void ConnectStatus()
	{
		TriedConnect = false;
		if (Connected)
		{
			WriteOut("\ar#\ax Connected!");
			SET->UpdateServer();
			LastReadPos = 0;
			LastSecs = 0;
			HandleChannels("", 0);
			SendLocalEcho();
			return;
		}
		WriteOut("\ar#\ax Could not connect.");
	};

	void DoConnectEQBCS()
	{
		char szMsg[MAX_STRING] = { 0 };

		usSockVersion = MAKEWORD(1, 1);
		WSAStartup(usSockVersion, &wsaData);
		pHostEntry = gethostbyname(szServer);
		if (!pHostEntry)
		{
			WriteOut("\ar#\ax gethostbyname error");
			WSACleanup();
			return;
		}
		theSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if (theSocket == INVALID_SOCKET)
		{
			WriteOut("\ar#\ax Socket error");
			WSACleanup();
			return;
		}

		// custom UI window update
		WINDOW->Create();
		WINDOW->UpdateTitle();

		sprintf_s(szMsg, "\ar#\ax Connecting to %s %s...", szServer, szPort);
		WriteOut(szMsg);

		LastPing = 0;
		serverInfo.sin_family = AF_INET;
		serverInfo.sin_addr = *((LPIN_ADDR)*pHostEntry->h_addr_list);
		serverInfo.sin_port = htons(atoi(szPort));
		unsigned long ThreadId = 0;
		CreateThread(NULL, 0, &EQBCConnectThread, 0, 0, &ThreadId);
	};

	void HandleBuffer()
	{
		int iErr = 0;
		// Fill the input buffer with new data, if any
		CHAR Buffer[MAX_READBUF] = { 0 };
		for (; LastReadPos < (MAX_READBUF - 1); LastReadPos++)
		{
			iErr = recv(theSocket, &pcReadBuf[LastReadPos], 1, 0);
			if ((pcReadBuf[LastReadPos] == '\n') || (iErr == 0) || (iErr == SOCKET_ERROR))
			{
				if (pcReadBuf[LastReadPos] == '\n')
				{
					pcReadBuf[LastReadPos] = '\0';
					strcpy_s(Buffer, pcReadBuf);
					HandleIncomingString(Buffer,false);
					LastReadPos = -1;
				}
				//break;
			}
			if (iErr == 0 || iErr == SOCKET_ERROR) break;
		}

		if (LastReadPos < 0) LastReadPos = 0;
		if (iErr == 0 && WSAGetLastError() == 0)
		{
			// Should be giving WSAWOULDBLOCK
			SocketGone = TRUE;
		}

		if (SocketGone)
		{
			SocketGone = FALSE;
			strcpy_s(Buffer, "-- Remote connection closed, you are no longer connected");
			HandleIncomingString(Buffer, false);
			Disconnect(false);
			if (SET->AutoReconnect && SET->ReconnectSecs > 0)
			{
				LastSecs = GetTickCount642() / 1000;
			}
		}
		if (LastPing > 0 && LastPing + 120000 < clock())
		{
			WriteOut("\arMQ2EQBC: did not recieve expected ping from server, pinging...");
			Transmit(true, CMD_PONG);
			LastPing = 0;
		}
	};
	template <unsigned int _Size>void HandleIncomingCmd(CHAR(&pszCmd)[_Size], bool bForce, bool silent=false)
	{
		if (!pszCmd)
			return;

		if (pszCmd && pszCmd[0]) {
			int len = strlen(pszCmd);
			if (char*pDest = strchr(pszCmd, '%')) {
				int len2 = pDest - pszCmd;
				if (pDest[1] == '\0')
				{
					Sleep(0);
					strcat_s(pszCmd, _Size, "%");
				}
				pEverQuest->DoPercentConvert(pszCmd, false);
			}
		}
		char  szTemp[MAX_STRING] = { 0 };
		if (!SET->AllowControl && !bForce)
		{
			if (!SET->SilentCmd)
			{
				sprintf_s(szTemp, "\ar#\ax CMD: [%s] - Not allowed (Control turned off)", pszCmd);
				WriteOut(szTemp);
			}
			if (SET->NotifyControl)
			{
				sprintf_s(szTemp, "Command [ %s ] not processed (Control turned off)", pszCmd);
				Transmit(true, szTemp);
			}
			return;
		}
		if (!(SET->SilentCmd || silent))
		{
			sprintf_s(szTemp, "\ar#\ax CMD: [%s]", pszCmd);
			WriteOut(szTemp);
		}

		if (!pLocalPlayer) return;
		sprintf_s(szTemp, pszCmd);
		CleanEnd(szTemp);
		DoCommand((PSPAWNINFO)pLocalPlayer, szTemp);
	};

	template <unsigned int _Size>void HandleBciMsg(CHAR(&szName)[_Size], char* szMsg)
	{
		// EQBC Interface handling
		// jimbob extended szBuff to MAX_String (2048) from 1024 (just in case)... I doubt it will get that big though
		char szBuff[MAX_STRING] = { 0 };
		if (!strncmp(szMsg, "REQ", 3))
		{
			if (GetGameState() == GAMESTATE_INGAME)
			{
				if (PCHARINFO pChar = GetCharInfo()) {
					if (PCHARINFO2 pChar2 = GetCharInfo2()) {
						if (PSPAWNINFO pSpawn = pChar->pSpawn) {
							// commented by jimbob -- Original RG Bbi Message Response...
							//							sprintf_s(szBuff, "1|%d|%d|%d|%d|%d|%d|%d|%d|%s|%s|%I64d|%d|%d|%c|%c", GetCurHPS(), GetMaxHPS(), GetCurMana(), GetMaxMana(), GetCurEndurance(), GetMaxEndurance(),
							//								pChar->zoneId, pChar2->Level, GetClassDesc(pChar2->Class), pEverQuest->GetRaceDesc(pChar2->Race), pChar->Exp,
							//								pChar->AAExp, pChar2->AAPoints,pSpawn->StandState, (pSpawn->PetID == 0xFFFFFFFF) ? '0' : '1');
							// Extended version for jimbob's eqbci Interface for MQ2EQBCS
							#if !defined(ROF2EMU) && !defined(UFEMU)
							sprintf_s(szBuff, "1|%d|%d|%d|%d|%d|%d|%d|%d|%s|%s|%I64d|%d|%d|%c|%c|%s|%f|%f|%f|%s|%d|%lld|%lld|%d|%s",
							#else
							sprintf_s(szBuff, "1|%d|%d|%d|%d|%d|%d|%d|%d|%s|%s|%I64d|%d|%d|%c|%c|%s|%f|%f|%f|%s|%d|%d|%d|%d|%s",
							#endif
								GetCurHPS(), GetMaxHPS(), GetCurMana(), GetMaxMana(), GetCurEndurance(), GetMaxEndurance(),
								pChar->zoneId, pChar2->Level, GetClassDesc(pChar2->Class), pEverQuest->GetRaceDesc(pChar2->Race), pChar->Exp,
								pChar->AAExp, pChar2->AAPoints, pSpawn->StandState, (pSpawn->PetID == 0xFFFFFFFF) ? '0' : '1',
								((PZONEINFO)pZoneInfo)->ShortName, pSpawn->X, pSpawn->Y, pSpawn->Z,
								(pTarget) ? pTarget->Data.Name : "(null)", (pTarget) ? pTarget->Data.Level : 0, (pTarget) ? pTarget->Data.HPCurrent : 0, (pTarget) ? pTarget->Data.HPMax : 0,
								(pChar->pGroupInfo) ? 1 : 0, (pChar->pGroupInfo) ? pChar->pGroupInfo->pLeader->pName->Text : "(null)"
							);
							BciTransmit(szName, szBuff);
						}
					}
				}
			}
		}
	};

	void ChanTransmit(char* szCommand, char* szLine)
	{
		int iErr = 0;
		iErr = send(theSocket, szCommand, strlen(szCommand), 0);
		CheckError("ChanTransmit:SendCmd", iErr);
		iErr = send(theSocket, szLine, strlen(szLine), 0);
		CheckError("ChanTransmit:SendLine", iErr);
		iErr = send(theSocket, SEND_LINE_TERM, strlen(SEND_LINE_TERM), 0);
		CheckError("ChanTransmit:SendTerm", iErr);
	};

	template <unsigned int _Size>void BciTransmit(CHAR(&szLine)[_Size], char* szCmd)
	{
		char szCommand[] = CMD_BCI;
		int  iErr = 0;

		strcat_s(szLine, " ");
		strcat_s(szLine, szCmd);

		if (szLine && strlen(szLine))
		{
			iErr = send(theSocket, szCommand, strlen(szCommand), 0);
			CheckError("BciTransmit:Send1", iErr);
			iErr = send(theSocket, szLine, strlen(szLine), 0);
			CheckError("BciTransmit:Send2", iErr);
			iErr = send(theSocket, SEND_LINE_TERM, strlen(SEND_LINE_TERM), 0);
			CheckError("BciTransmit:Send3", iErr);
		}
	};

	void SendNetBotMsg(char* szMess)
	{
		char* pPosi = NULL;
		char* pName = NULL;
		if (szMess != NULL && *szMess != NULL)
		{
			if (pPosi = strchr(szMess, ':'))
			{
				if (!_strnicmp(pPosi, ":[NB]|", 6))
				{
					pName = "mq2netbots";
				}
				else if (!_strnicmp(pPosi, ":[NH]|", 6))
				{
					pName = "mq2netheal";
				}
			}
		}
		if (pName == NULL || *pName == NULL)
		{
			DebugSpew("SendNetBotMSG::Bad NBMSG");
			return;
		}

		fNetBotOnMsg pfSendf = NULL;
		PMQPLUGIN    pFind = pPlugins;
		while (pFind && _stricmp(pFind->szFilename, pName))
		{
			pFind = pFind->pNext;
		}
		if (pFind)
		{
			pfSendf = (fNetBotOnMsg)GetProcAddress(pFind->hModule, "OnNetBotMSG");
		}
		if (pfSendf == NULL)
		{
			DebugSpew("SendNetBotMSG::No Handler");
			return;
		}

		*pPosi = 0;
		pfSendf(szMess, pPosi + 1);
		*pPosi = ':';
	};

	void SendNetBotEvent(char* szMess)
	{
		if (szMess == NULL || *szMess == NULL)
		{
			DebugSpew("SendNetBotEVENT::Bad NBMSG");
			return;
		}

		fNetBotOnEvent pfSendf = NULL;
		PMQPLUGIN      pFind = pPlugins;
		while (pFind && _stricmp(pFind->szFilename, "mq2netbots"))
		{
			pFind = pFind->pNext;
		}
		if (pFind)
		{
			pfSendf = (fNetBotOnEvent)GetProcAddress(pFind->hModule, "OnNetBotEVENT");
		}
		if (pfSendf == NULL)
		{
			DebugSpew("SendNetBotEVENT::No Handler");
			return;
		}
		pfSendf(szMess);
	};

	int WriteStringGetCount(char* pDest, char* pSrc)
	{
		int i = 0;
		for (; pDest && pSrc && *pSrc; pSrc++)
		{
			pDest[i++] = *pSrc;
		}
		return i;
	};

	char GetCharColor(char* pTest)
	{
		// Colors From MQToSTML (already assigned here to szColorChars)
		// 'y'ellow, 'o'range, 'g'reen, bl'u'e, 'r'ed, 't'eal, 'b'lack (none),
		// 'm'agenta, 'p'urple, 'w'hite, 'x'=back to default
		// Color code format: "[+r+]"
		if (pTest[0] == '['  &&
			pTest[1] == '+'  &&
			pTest[2] != '\0' &&
			pTest[3] == '+'  &&
			pTest[4] == ']')
		{
			if (strchr(szColorChars, (int)tolower(pTest[2])))
			{
				return pTest[2];
			}
		}
		return 0;
	};

	void CleanEnd(char* pszStr)
	{
		// Remove trailing spaces and CR/LF's
		if (pszStr && *pszStr)
		{
			int iLen = 0;
			for (iLen = strlen(pszStr) - 1; iLen >= 0 && strchr(" \r\n", pszStr[iLen]); pszStr[iLen--] = 0);
		}
	};

	int                SocketGone;
	int                LastReadPos;
	char*              pcReadBuf;
	clock_t            LastPing;
	WSADATA            wsaData;
	hostent*           pHostEntry;
	unsigned short     usSockVersion;
};

unsigned long __stdcall EQBCConnectThread(void* lpParam)
{
	EnterCriticalSection(&ConnectCS);
	EQBC->Connecting = true;
	iNRet = connect(theSocket, (sockaddr*)&serverInfo, sizeof(struct sockaddr));

	if (iNRet == SOCKET_ERROR)
	{
		EQBC->Connected = false;
	}
	else
	{
		unsigned long ulNonblocking = 1;
		ioctlsocket(theSocket, FIONBIO, &ulNonblocking);
		Sleep((clock_t)4 * CLOCKS_PER_SEC / 2);

		send(theSocket, CONNECT_START, strlen(CONNECT_START), 0);
		if (*szPassword)
		{
			// DebugSpew("With Password");
			send(theSocket, CONNECT_PWSEP, strlen(CONNECT_PWSEP), 0);
			send(theSocket, szPassword, strlen(szPassword), 0);
		}
		send(theSocket, CONNECT_START2, strlen(CONNECT_START2), 0);
		send(theSocket, szToonName, strlen(szToonName), 0);
		send(theSocket, CONNECT_END, strlen(CONNECT_END), 0);
		EQBC->Connected = true;
	}

	EQBC->TriedConnect = true;
	EQBC->Connecting = false;
	LeaveCriticalSection(&ConnectCS);
	return 0;
}

// --------------------------------------
// placement
void CSettingsMgr::HandleWnd(bool bCreate)
{
	if (bCreate)
	{
		if (WINDOW)
			WINDOW->Create();
		return;
	}
	if (WINDOW)
		WINDOW->Destroy(true);
}

void CSettingsMgr::HandleEcho()
{
	EQBC->SendLocalEcho();
}

void CEQBCWnd::WriteToBC(char* szBuffer)
{
	EQBC->BC(szBuffer);
}

// --------------------------------------
// Custom ${EQBC} TLO
class EQBCType : public MQ2Type
{
public:
	enum VarMembers
	{
        Connected = 1,
        Server    = 2,
        Port      = 3,
        ToonName  = 4,
        Setting   = 5,
		Names     = 6,
		GotNames  = 7,
	};
	EQBCType();
	bool GetMember(MQ2VARPTR VarPtr, char* Member, char* Index, MQ2TYPEVAR &Dest);
	bool ToString(MQ2VARPTR VarPtr, char* Destination);
	bool FromData(MQ2VARPTR &VarPtr, MQ2TYPEVAR &Source);
	bool FromString(MQ2VARPTR &VarPtr, char* Source);
	bool OptStatus(char* Index);
};

EQBCType::EQBCType() :MQ2Type("EQBC")
{
	TypeMember(Connected);
	TypeMember(Server);
	TypeMember(Port);
	TypeMember(ToonName);
	TypeMember(Setting);
	TypeMember(Names);
	TypeMember(GotNames);
}

bool EQBCType::OptStatus(char* Index)
{
	int* iOption = NULL;
	if (!_stricmp(Index, szSetAutoConnect))
	{
		iOption = &SET->AutoConnect;
	}
	else if (!_stricmp(Index, szSetControl))
	{
		iOption = &SET->AllowControl;
	}
	else if (!_stricmp(Index, szSetCompatMode))
	{
		iOption = &SET->IRCMode;
	}
	else if (!_stricmp(Index, szSetAutoReconnect))
	{
		iOption = &SET->AutoReconnect;
	}
	else if (!_stricmp(Index, szSetWindow))
	{
		iOption = &SET->Window;
	}
	else if (!_stricmp(Index, szSetEchoAll))
	{
		iOption = &SET->EchoAll;
	}
	else if (!_stricmp(Index, szSetLocalEcho))
	{
		iOption = &SET->LocalEcho;
	}
	else if (!_stricmp(Index, szSetTellWatch))
	{
		iOption = &SET->WatchTell;
	}
	else if (!_stricmp(Index, szSetRaidWatch))
	{
		iOption = &SET->WatchRaid;
	}
	else if (!_stricmp(Index, szSetGuildWatch))
	{
		iOption = &SET->WatchGuild;
	}
	else if (!_stricmp(Index, szSetGroupWatch))
	{
		iOption = &SET->WatchGroup;
	}
	else if (!_stricmp(Index, szSetFSWatch))
	{
		iOption = &SET->WatchFsay;
	}
	else if (!_stricmp(Index, szSetSilentCmd))
	{
		iOption = &SET->SilentCmd;
	}
	else if (!_stricmp(Index, szSetNameAnnounce))
	{
		iOption = &SET->NameAnnounce;
	}
	else if (!_stricmp(Index, szSetSaveByChar))
	{
		iOption = &SET->SaveByChar;
	}
	else if (!_stricmp(Index, szSetSilentIncCmd))
	{
		iOption = &SET->SilentIncCmd;
	}
	else if (!_stricmp(Index, szSetSilentOutMsg))
	{
		iOption = &SET->SilentOutMsg;
	}
	else if (!_stricmp(Index, szSetNotifyControl))
	{
		iOption = &SET->NotifyControl;
	}
	return (!iOption ? false : (*iOption ? true : false));
}

bool EQBCType::GetMember(MQ2VARPTR VarPtr, char* Member, char* Index, MQ2TYPEVAR &Dest)
{
	PMQ2TYPEMEMBER pMember = EQBCType::FindMember(Member);
	if (!pMember || !EQBC) return false;
	switch ((VarMembers)pMember->ID)
	{
	case Connected:
		Dest.DWord = EQBC->Connected;
		Dest.Type = pBoolType;
		return true;
	case Server:
		sprintf_s(DataTypeTemp, "OFFLINE");
		if (EQBC->Connected)
		{
			strcpy_s(DataTypeTemp, szServer);
		}
		Dest.Ptr = &DataTypeTemp[0];
		Dest.Type = pStringType;
		return true;
	case Port:
		sprintf_s(DataTypeTemp, "OFFLINE");
		if (EQBC->Connected)
		{
			strcpy_s(DataTypeTemp, szPort);
		}
		Dest.Ptr = &DataTypeTemp[0];
		Dest.Type = pStringType;
		return true;
	case ToonName:
		sprintf_s(DataTypeTemp, "OFFLINE");
		if (EQBC->Connected)
		{
			strcpy_s(DataTypeTemp, szToonName);
		}
		Dest.Ptr = &DataTypeTemp[0];
		Dest.Type = pStringType;
		return true;
	case Setting:
		Dest.DWord = false;
		Dest.Type = pBoolType;
		if (!Index[0]) return true;
		Dest.DWord = OptStatus(Index);
		return true;
	case Names:
	{
		Dest.Ptr = &szConnectedChars[0];
		Dest.Type = pStringType;
		return true;
	}
	case GotNames:
		Dest.DWord = bGotNames;
		Dest.Type = pBoolType;
		return true;
	}
	return false;
}

bool EQBCType::ToString(MQ2VARPTR VarPtr, char* Destination)
{
	strcpy_s(Destination, MAX_STRING, "EQBC");
	return true;
}

bool EQBCType::FromData(MQ2VARPTR &VarPtr, MQ2TYPEVAR &Source)
{
	return false;
}

bool EQBCType::FromString(MQ2VARPTR &VarPtr, char* Source)
{
	return false;
}

int dataEQBC(char* Index, MQ2TYPEVAR &Dest)
{
	Dest.DWord = 1;
	Dest.Type = pEQBCType;
	return true;
}

// --------------------------------------
// Command input

void OutputHelp()
{
	EQBC->Status();
	WriteOut("\ar#\ax Commands Available");
	WriteOut("\ar#\ax \ay/bc your text\ax (send text)");
	WriteOut("\ar#\ax \ay/bc ToonName //command\ax (send Command to ToonName - \arDEPRECATED, use /bct)");
	WriteOut("\ar#\ax \ay/bcg your text\ax (send your text to Group)");
	WriteOut("\ar#\ax \ay/bcg //command\ax (send Command to Group)");
	WriteOut("\ar#\ax \ay/bcga your text\ax (send your text to Group including yourself)");
	WriteOut("\ar#\ax \ay/bcga //command\ax (send Command to Group including yourself)");
	WriteOut("\ar#\ax \ay/bct ToonName your text\ax (send your text to specific Toon)");
	WriteOut("\ar#\ax \ay/bct ToonName //command\ax (send Command to ToonName)");
	WriteOut("\ar#\ax \ay/bca //command\ax (send Command to all connected names EXCLUDING yourself)");
	WriteOut("\ar#\ax \ay/bcaa //command\ax (send Command to all connected names INCLUDING yourself)");
	WriteOut("\ar#\ax \ay/bcsg your text\ax (silently send your text to Group)");
	WriteOut("\ar#\ax \ay/bcsg //command\ax (silently send Command to Group)");
	WriteOut("\ar#\ax \ay/bcsga your text\ax (silently send your text to Group including yourself)");
	WriteOut("\ar#\ax \ay/bcsga //command\ax (silently send Command to Group including yourself)");
	WriteOut("\ar#\ax \ay/bcst ToonName your text\ax (silently send your text to specific Toon)");
	WriteOut("\ar#\ax \ay/bcst ToonName //command\ax (silently send Command to ToonName)");
	WriteOut("\ar#\ax \ay/bcsa //command\ax (silently send Command to all connected names EXCLUDING yourself)");
	WriteOut("\ar#\ax \ay/bcsaa //command\ax (silently send Command to all connected names INCLUDING yourself)");
	WriteOut("\ar#\ax \ay/bccmd connect <server> <port> <pw>\ax (defaults: 127.0.0.1 2112)");
	WriteOut("\ar#\ax \ay/bccmd forceconnect <server> <port> <pw>\ax (defaults: 127.0.0.1 2112)");
	WriteOut("\ar#\ax \ay/bccmd iniconnect [INIKeyName]");
	WriteOut("\ar#\ax \ay/bccmd quit\ax (disconnects)");
	WriteOut("\ar#\ax \ay/bccmd help\ax (show this information)");
	WriteOut("\ar#\ax \ay/bccmd status\ax (show connected or not and settings)");
	WriteOut("\ar#\ax \ay/bccmd reconnect\ax (close current connection and connect again)");
	WriteOut("\ar#\ax \ay/bccmd names\ax (show who is connected)");
	WriteOut("\ar#\ax \ay/bccmd version\ax (show plugin version)");
	WriteOut("\ar#\ax \ay/bccmd colordump\ax (Shows all available color codes)");
	WriteOut("\ar#\ax \ay/bccmd channels <channel list>\ax (set list of channels to receive tells from)");
	WriteOut("\ar#\ax \ay/bccmd stopreconnect\ax (stop trying to reconnect for now)");
	WriteOut("\ar#\ax Settings Available");
	WriteOut("\ar#\ax \ay/bccmd set reconnectsecs n\ax (n is seconds to reconnect: default 15)");
	WriteOut("\ar#\ax \ay/bccmd set OPTION\ax [ \ayon\ax | \ayoff\ax ] (set OPTION directly)");
	WriteOut("\ar#\ax \ay/bccmd toggle OPTION\ax (toggles OPTION) - where OPTION may be:");
	WriteOut("\aw -- \ayautoconnect\ax (auto connect to previous server)");
	WriteOut("\aw -- \aycontrol\ax (allow remote control)");
	WriteOut("\aw -- \aycompatmode\ax (IRC Compatability mode)");
	WriteOut("\aw -- \ayreconnect\ax (auto-reconnect mode on server disconnect)");
	WriteOut("\aw -- \aywindow\ax (use dedicated chat window)");
	WriteOut("\aw -- \aylocalecho\ax (echo my commands back to me if I am in channel)");
	WriteOut("\aw -- \aytellwatch\ax (relay tells received to /bc)");
	WriteOut("\aw -- \ayguildwatch\ax (relay guild chat to /bc)");
	WriteOut("\aw -- \aygroupwatch\ax (relay group chat to /bc)");
	WriteOut("\aw -- \ayfswatch\ax (relay fellowship chat to /bc)");
	WriteOut("\aw -- \aysilentcmd\ax (display 'CMD: [command]' echo)");
	WriteOut("\aw -- \aynameannounce\ax (announce names on connect and disconnect)");
	WriteOut("\aw -- \aysavebychar\ax (save UI data to [CharName] INI section)");
	WriteOut("\aw -- \aysilentinccmd\ax (display incoming commands)");
	WriteOut("\aw -- \aynotifycontrol\ax (notify /bc when disabled control blocks incoming command)");
	WriteOut("\aw -- \aysilentoutmsg\ax (squelch outgoing msgs regardless of compatmode enabled)");
	WriteOut("\aw -- \ayechoall\ax (echo outgoing /bca messages)");
}

// bccmd
void BccmdCmd(PSPAWNINFO pLPlayer, char* szline)
{
	CHAR szCmd[MAX_STRING] = { 0 };
	strcpy_s(szCmd, szline);
	char szArg[MAX_STRING] = { 0 };
	char szMsg[MAX_STRING] = { 0 };

	GetArg(szArg, szCmd, 1);

	if (!_strnicmp(szArg, "set", 4))
	{
		char szTempSet[MAX_STRING] = { 0 };
		sprintf_s(szTempSet, "%s", GetNextArg(szCmd, 1, FALSE, 0));
		SET->Change(szTempSet, false);
		return;
	}
	else if (!_strnicmp(szArg, "toggle", 7))
	{
		char szTempSet[MAX_STRING] = { 0 };
		sprintf_s(szTempSet, "%s", GetNextArg(szCmd, 1, FALSE, 0));
		SET->Change(szTempSet, true);
		return;
	}

	if (!_strnicmp(szArg, szCmdHelp, sizeof(szCmdHelp)))
	{
		OutputHelp();
	}
	else if (!_strnicmp(szArg, szCmdConnect, sizeof(szCmdConnect)))
	{
		EQBC->Connect(szCmd, false);
	}
	else if (!_strnicmp(szArg, szCmdDisconnect, sizeof(szCmdDisconnect)))
	{
		EQBC->Disconnect(true);
		WriteOut("\ar#\ax Connection Closed, you can unload MQ2Eqbc now.");
	}
	else if (!_strnicmp(szArg, szCmdStatus, sizeof(szCmdStatus)))
	{
		EQBC->Status();
	}
	else if (!_strnicmp(szArg, szCmdReconnect, sizeof(szCmdReconnect)))
	{
		EQBC->Reconnect();
	}
	else if (!_strnicmp(szArg, szCmdNames, sizeof(szCmdNames)))
	{
		EQBC->Names();
	}
	else if (!_strnicmp(szArg, szCmdRelog, sizeof(szCmdRelog)))
	{
		WriteOut("\ar#\ax Relog command removed. Try out MQ2SwitchChar by ieatacid.");
	}
	else if (!_strnicmp(szArg, szCmdVersion, sizeof(szCmdVersion)))
	{
		sprintf_s(szMsg, "\ar#\ax %s", PROG_VERSION);
		WriteOut(szMsg);
	}
	else if (!_strnicmp(szArg, szCmdColorDump, sizeof(szCmdColorDump)))
	{
		char cCh = 0;
		int  i = 0;
		strcpy_s(szMsg, "\ar#\ax Bright Colors:");
		for (i = 0; i < (int)strlen(szColorChars) - 1; i++)
		{
			cCh = szColorChars[i];
			sprintf_s(szArg, " \a%c[+%c+]", cCh, cCh);
			strcat_s(szMsg, szArg);
		}
		WriteOut(szMsg);
		strcpy_s(szMsg, "\ar#\ax Dark Colors:");
		for (i = 0; i < (int)strlen(szColorChars) - 1; i++)
		{
			cCh = szColorChars[i];
			sprintf_s(szArg, " \a-%c[+%c+]", cCh, toupper(cCh));
			strcat_s(szMsg, szArg);
		}
		WriteOut(szMsg);
		WriteOut("\ar#\ax [+x+] and [+X+] set back to default color.");
	}
	else if (!_strnicmp(szArg, szCmdChannels, sizeof(szCmdChannels)))
	{
		EQBC->HandleChannels(szCmd);
	}
	else if (!_strnicmp(szArg, szCmdNoReconnect, sizeof(szCmdNoReconnect)))
	{
		if (!EQBC->LastSecs)
		{
			WriteOut("\ar#\ax You are not trying to reconnect");
			return;
		}
		WriteOut("\ar#\ax Disabling reconnect mode for now.");
		EQBC->LastSecs = 0;
	}
	else if (!_strnicmp(szArg, szCmdForceConnect, sizeof(szCmdForceConnect)))
	{
		EQBC->Connect(szCmd, true);
	}
	else if (!_strnicmp(szArg, szCmdIniConnect, sizeof(szCmdIniConnect)))
	{
		GetArg(szArg, szCmd, 2);
		EQBC->ConnectINI(szArg);
	}
	else
	{
		sprintf_s(szMsg, "\ar#\ax Unsupported command, supported commands are: %s", VALID_COMMANDS);
		WriteOut(szMsg);
	}
}

// bc
void BcCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	EQBC->BC(szLine);
}

// bcg
void BcgCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	EQBC->BCG(szLine);
}

// bcga
void BcgaCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	EQBC->BCGA(szLine);
}

// bct
void BctCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	EQBC->BCT(szLine);
}

// bca
void BcaCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	EQBC->BCA(szLine);
}

// bcaa
void BcaaCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	EQBC->BCAA(pLPlayer, szLine);
}

// bcsg
void BcsgCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	EQBC->BCG(szLine, true);
}

// bcsga
void BcsgaCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	EQBC->BCGA(szLine, true);
}

// bcst
void BcstCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	EQBC->BCT(szLine, true);
}

// bcsa
void BcsaCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	EQBC->BCA(szLine, true);
}

// bcsaa
void BcsaaCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	EQBC->BCAA(pLPlayer, szLine, true);
}

// bcclear
void ClearWndCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	WINDOW->Clear();
}

// bcfont
void WndFontCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	char szArg[MAX_STRING] = { 0 };
	GetArg(szArg, szLine, 1);
	if (*szArg)
	{
		char* pNotNum = NULL;
		int   iValid = strtoul(szArg, &pNotNum, 10);
		if (iValid >= 0 && iValid <= 10 && !*pNotNum)
		{
			WINDOW->NewFont(iValid);
			return;
		}
	}
	char szError[200] = { 0 };
	sprintf_s(szError, "\ar%s\ax Usage: /bcfont 0-10", MODULE_NAME);
	WriteOut(szError);
}

// bcmin
void MinWndCmd(PSPAWNINFO pLPlayer, char* szLine)
{
	WINDOW->Min();
}

void KeybindEQBC(char* szKeyName, int iDown)
{
	WINDOW->Keybind(iDown);
}

// --------------------------------------
// Exported Functions

// Plugin & MQ2NetBots Support
// note: dont use netbots
PLUGIN_API void NetBotSendMsg(char* szMsg)
{
	if (szMsg && *szMsg)
	{
		EQBC->Transmit(true, "\tNBMSG");
		EQBC->Transmit(true, szMsg);
	}
}

PLUGIN_API void NetBotRequest(char* szMsg)
{
	if (szMsg && *szMsg)
	{
		if (!strncmp(szMsg, "NAMES", 5))
		{
			EQBC->Transmit(true, "\tNBNAMES");
		}
	}
}

PLUGIN_API unsigned short isConnected()
{
	return (EQBC->Connected ? 1 : 0);
}

PLUGIN_API bool AreTheyConnected(char* szName)
{
	if (std::find(connectedcharacters.begin(), connectedcharacters.end(), szName) != connectedcharacters.end())
	{
		return true;
	}
	return false;
}


// MQ2Main
PLUGIN_API void OnPulse()
{
	if (ValidIngame() && InHoverState()) WINDOW->Hover();
	EQBC->Pulse();
}

PLUGIN_API void SetGameState(unsigned long ulGameState)
{
	if (ulGameState == GAMESTATE_INGAME)
	{
		// setup new char name on zone (for INI writes)
		sprintf_s(szCharName, "%s.%s", EQADDR_SERVERNAME, ((PCHARINFO)pCharData)->Name);
		// load ini settings if entering to world
		if (!SET->Loaded)
		{
			// load ini settings
			SET->LoadINI();
			// draw window if desired
			WINDOW->Create();
			// update EQBC player name
			SetPlayer();
			// display welcome first on first entry to world
			if (!SET->FirstLoad)
			{
				char szTemp[MAX_STRING] = { 0 };
				sprintf_s(szTemp, "\ar#\ax Welcome to \ayMQ2EQBC\ax, %s: Use \ar/bccmd help\ax to see help.", szToonName);
				WriteOut(szTemp);
				SET->FirstLoad = true;
			}
		}
		// else redraw window if needed
		else
		{
			WINDOW->Create();
		}
		// handle auto connection
		EQBC->AutoConnect();
		WINDOW->ResetKeys();
	}
	else
	{
		if (ulGameState == GAMESTATE_CHARSELECT)
		{
			// kill window at char select
			WINDOW->Destroy(true);
			// set INI settings to reload on next login
			SET->Loaded = false;
			// disconnect if not logging back in
			EQBC->Logout();
			return;
		}
		// (server select) 0 && -1 normally leave connection open if '/camp server'
		if (ulGameState == -1) // wtb GAMESTATE_SERVERSELECT
		{
			WINDOW->Destroy(false);
			SET->Loaded = false;
			EQBC->Logout();
		}
	}
}

PLUGIN_API void OnCleanUI()
{
	WINDOW->Destroy(true);
}

PLUGIN_API void OnReloadUI()
{
	WINDOW->Create();
}

PLUGIN_API unsigned long OnIncomingChat(char* szLine, unsigned long ulColor)
{
	if (!ValidIngame() || !EQBC->Connected || szLine[0] != '\x12') return 0;

    PSPAWNINFO    pLPlayer               = (PSPAWNINFO)pLocalPlayer;
    char          szSender[MAX_STRING]   = {0};
    char          szTell[MAX_STRING]     = {0};
    char          szOutgoing[MAX_STRING] = {0};
    char*         pszText                = NULL;
    unsigned long n                      = strchr(&szLine[2], '\x12') - &szLine[2];

	strncpy_s(szSender, &szLine[2], n);

	if (SET->WatchRaid && ulColor == USERCOLOR_RAID)
	{
		pszText = GetNextArg(szLine, 1, FALSE, '\'');
		strcpy_s(szTell, pszText);
		szTell[strlen(pszText) - 1] = '\0';
		sprintf_s(szOutgoing, "RSAY from %s: %s", szSender, szTell);
		EQBC->BC(szOutgoing);
		return 0;
	}
	if (SET->WatchTell && ulColor == USERCOLOR_TELL)
	{
		pszText = GetNextArg(szLine, 1, FALSE, '\'');
		strcpy_s(szTell, pszText);
		szTell[strlen(pszText) - 1] = '\0';
		sprintf_s(szOutgoing, "Tell from %s: %s", szSender, szTell);
		EQBC->BC(szOutgoing);
		return 0;
	}
	if (SET->WatchGuild && ulColor == USERCOLOR_GUILD)
	{
		pszText = GetNextArg(szLine, 1, FALSE, '\'');
		strcpy_s(szTell, pszText);
		szTell[strlen(pszText) - 1] = '\0';
		sprintf_s(szOutgoing, "GU from %s: %s", szSender, szTell);
		EQBC->BC(szOutgoing);
		return 0;
	}
	if (SET->WatchGroup && ulColor == USERCOLOR_GROUP)
	{
		pszText = GetNextArg(szLine, 1, FALSE, '\'');
		strcpy_s(szTell, pszText);
		szTell[strlen(pszText) - 1] = '\0';
		sprintf_s(szOutgoing, "GSAY from %s: %s", szSender, szTell);
		EQBC->BC(szOutgoing);
		return 0;
	}
	if (SET->WatchFsay && ulColor == USERCOLOR_FELLOWSHIP)
	{
		pszText = GetNextArg(szLine, 1, FALSE, '\'');
		strcpy_s(szTell, pszText);
		szTell[strlen(pszText) - 1] = '\0';
		sprintf_s(szOutgoing, "FSAY from %s: %s", szSender, szTell);
		EQBC->BC(szOutgoing);
		return 0;
	}
	return 0;
}

PLUGIN_API void InitializePlugin()
{
	AddCommand("/bc", BcCmd);
	AddCommand("/bcg", BcgCmd);
	AddCommand("/bcga", BcgaCmd);
	AddCommand("/bct", BctCmd);
	AddCommand("/bca", BcaCmd);
	AddCommand("/bcaa", BcaaCmd);
	AddCommand("/bcsg", BcsgCmd);
	AddCommand("/bcsga", BcsgaCmd);
	AddCommand("/bcst", BcstCmd);
	AddCommand("/bcsa", BcsaCmd);
	AddCommand("/bcsaa", BcsaaCmd);
	AddCommand("/bccmd", BccmdCmd);
	AddCommand("/bcclear", ClearWndCmd);
	AddCommand("/bcfont", WndFontCmd);
	AddCommand("/bcmin", MinWndCmd);

	SET = new CSettingsMgr();
	WINDOW = new CEQBCWndHandler();
	EQBC = new CConnectionMgr();
	pEQBCType = new EQBCType;

	AddMQ2Data("EQBC", dataEQBC);
	AddMQ2KeyBind("EQBC", KeybindEQBC);

	EQBC->NewThread();
}

PLUGIN_API void ShutdownPlugin()
{
	if (EQBC->Connected)
	{
		WriteOut("\ar#\ax You are still connected! Attempting to disconnect.");
		DebugSpew("MQ2Eqbc::Still Connected::Attempting disconnect.");
		EQBC->Disconnect(false);
	}

	RemoveCommand("/bc");
	RemoveCommand("/bcg");
	RemoveCommand("/bcga");
	RemoveCommand("/bct");
	RemoveCommand("/bca");
	RemoveCommand("/bcaa");
	RemoveCommand("/bcsg");
	RemoveCommand("/bcsga");
	RemoveCommand("/bcst");
	RemoveCommand("/bcsa");
	RemoveCommand("/bcsaa");
	RemoveCommand("/bccmd");
	RemoveCommand("/bcclear");
	RemoveCommand("/bcfont");
	RemoveCommand("/bcmin");

	RemoveMQ2Data("EQBC");
	RemoveMQ2KeyBind("EQBC");
	WINDOW->Destroy(ValidIngame());

	// make sure we're not trying to connect...
	EQBC->KillThread();

	delete pEQBCType;
	delete EQBC;
	delete WINDOW;
	delete SET;
}
