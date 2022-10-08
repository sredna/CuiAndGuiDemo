/*

This example uses the fact that .com is listed before .exe in %PathExt% to start the application in the "correct" mode by default. 
The .exe is the main application and the .com is just a launcher. 
Use /GUI or /Console to force a specific mode.

*********
Compiling
*********

cl CuiAndGuiDemo_Com.cxx /FeComDemo.com /DCOMLDR /O1s /Zl /DUNICODE /DWINVER=0x501 /link /subsystem:CONSOLE /OPT:REF /OPT:ICF /MANIFEST:NO kernel32.lib shell32.lib
cl CuiAndGuiDemo_Com.cxx /FeComDemo.exe /O1s /Zl /DUNICODE /DWINVER=0x501 /link /subsystem:WINDOWS /OPT:REF /OPT:ICF /MANIFEST:NO kernel32.lib user32.lib shell32.lib

*********
Executing
*********
CUI mode: cmd.exe /K ComDemo
CUI mode: ComDemo.exe /Console
GUI mode: cmd.exe /C ComDemo /GUI
GUI mode: ComDemo.exe

*/

#include <windows.h>
#include <shellapi.h>

#ifdef COMLDR
EXTERN_C __declspec(noreturn) void __cdecl mainCRTStartup()
{
	DWORD exitcode = 0, forceGUI = FALSE, i;
	WCHAR modbuf[MAX_PATH];
	WCHAR *cmd = GetCommandLine(), **argv;
	int argc;

	argv = CommandLineToArgvW(cmd, &argc);
	if (argv)
	{
		for (UINT i = 0, c = argc; ++i < c;)
		{
			if (!lstrcmpi(argv[i], TEXT("/GUI")))
			{
				++forceGUI;
				break;
			}
		}
		LocalFree(argv);
	}

	i = GetModuleFileName(NULL, modbuf, MAX_PATH);
	if (!i || i == MAX_PATH)
	{
		exitcode = i ? ERROR_INSUFFICIENT_BUFFER : GetLastError();
	}
	else
	{
		for (; i--;)
		{
			if (modbuf[i] == '/' || modbuf[i] == '\\') modbuf[++i] = '.';
			if (modbuf[i] == '.') break;
		}
		if (!lstrcmpi(&modbuf[i], TEXT(".exe")))
		{
			exitcode = ERROR_INVALID_NAME; // We are a .exe, did you forget to rename to .com?
		}
		lstrcpy(&modbuf[i], TEXT(".exe"));
	}

	if (!exitcode)
	{
		PROCESS_INFORMATION pi;
		STARTUPINFO si;

		si.lpReserved = si.lpDesktop = si.lpTitle = NULL;
		si.lpReserved2 = NULL, si.cbReserved2 = 0;
		si.cb = sizeof(si);
		si.dwFlags = forceGUI ? 0 : STARTF_USESTDHANDLES;
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		if (CreateProcess(modbuf, cmd, NULL, NULL, !forceGUI, 0, NULL, NULL, &si, &pi))
		{
			if (!forceGUI)
			{
				WaitForSingleObject(pi.hProcess, INFINITE);
				GetExitCodeProcess(pi.hProcess, &exitcode);
			}
			CloseHandle(pi.hProcess);
			CloseHandle(pi.hThread);
		}
		else
		{
			exitcode = GetLastError();
		}
	}

	ExitProcess(exitcode);
}
#else
EXTERN_C __declspec(noreturn) void __cdecl WinMainCRTStartup()
{
	DWORD gui = TRUE, forceConsole = FALSE, pause = FALSE;
	STARTUPINFO si;
	WCHAR *cmd = GetCommandLine(), **argv;
	int argc;

	argv = CommandLineToArgvW(cmd, &argc);
	if (argv)
	{
		for (UINT i = 0, c = argc; ++i < c;)
		{
			if (!lstrcmpi(argv[i], TEXT("/Console")))
			{
				++forceConsole;
				break;
			}
		}
		LocalFree(argv);
	}

	si.cb = sizeof(si);
	GetStartupInfo(&si);
	if ((si.dwFlags & STARTF_USESTDHANDLES) || forceConsole)
	{
		if (!AttachConsole(ATTACH_PARENT_PROCESS) && forceConsole)
		{
			pause = AllocConsole();
		}
		si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
		si.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
		si.hStdError = GetStdHandle(STD_ERROR_HANDLE);
		gui = (si.hStdOutput == 0 && si.hStdError == 0);
	}

	if (gui)
	{
		MessageBox(0, TEXT("Hello from GUI mode!"), TEXT("DEMO"), MB_ICONINFORMATION);
	}
	else
	{
		LPCSTR msg = "Hello from console mode!\n";
		DWORD cb;

		WriteFile(si.hStdOutput, msg, lstrlenA(msg), &cb, NULL);
		if (pause)
		{
			WinExec("cmd.exe /C pause", SW_SHOW); // Replace with 'system("pause")' if you are using the CRT.
		}
	}

	ExitProcess(0);
}
#endif
