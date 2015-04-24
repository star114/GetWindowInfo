// GetWindowInfo.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include "stdafx.h"
#include <Windows.h>
#include "ntdll.h"

#define MAX_NAME_LENGTH 255
#define MAX_COMMANDLINE_LENGTH 1024

typedef NTSTATUS(NTAPI* PFNtQueryInformationProcess)(
	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL
	);
PFNtQueryInformationProcess NtQueryInformationProcess;

bool GetCommandLine(DWORD dwPid, LPWSTR lpwstr)
{
	bool fSuccess = false;
	HANDLE hProcess = NULL;

	hProcess = ::OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, dwPid);
	if (NULL != hProcess)
	{
		PROCESS_BASIC_INFORMATION pbi = { 0 };

		NTSTATUS status = NtQueryInformationProcess(
			hProcess,
			ProcessBasicInformation,
			&pbi,
			sizeof(pbi),
			NULL
			);
		if (NT_SUCCESS(status) && NULL != pbi.PebBaseAddress)
		{
			PEB_ peb = { 0 };
			if (::ReadProcessMemory(
				hProcess,
				pbi.PebBaseAddress,
				&peb,
				sizeof(peb),
				NULL
				))
			{
				RTL_USER_PROCESS_PARAMETERS params = { 0 };
				if (::ReadProcessMemory(
					hProcess,
					peb.ProcessParameters,
					&params,
					sizeof(params),
					NULL
					)
					&& params.CommandLine.Length > 0
					)
				{
					if (params.CommandLine.Length > MAX_COMMANDLINE_LENGTH)
						printf("CommandLine Length : %d > MAX_COMMANDLINE_LENGTH(%d)\n", params.CommandLine.Length, MAX_COMMANDLINE_LENGTH);

					if (::ReadProcessMemory(
						hProcess,
						params.CommandLine.Buffer,
						lpwstr,
						MAX_COMMANDLINE_LENGTH,
						NULL
						))
					{
						fSuccess = true;
					}
				}
			}
		}

		::CloseHandle(hProcess);
	}

	return fSuccess;
}

int _tmain(int argc, _TCHAR* argv[])
{
	printf("*GetWindowInfo*\n");
	printf("You can monitoring the window inforamtion and its process inforamtion that pointed by your mouse cursor\n");
	printf("If you press any key, start monitoring.\n");
	WCHAR wchtemp;
	scanf_s("%c", &wchtemp);

	HMODULE hmodule = GetModuleHandle(L"ntdll.dll");
	if (NULL == hmodule) hmodule = LoadLibraryW(L"ntdll.dll");
	if (NULL == hmodule)
	{
		printf("ntdll load failed with %d", ::GetLastError());
		return -1;
	}

	NtQueryInformationProcess = (PFNtQueryInformationProcess)::GetProcAddress(hmodule, "NtQueryInformationProcess");
	POINT p = { 0, };
	HWND prevhwnd = NULL;
	HWND curhwnd = NULL;
	while (GetCursorPos(&p))
	{
		if ((curhwnd = WindowFromPoint(p)) != prevhwnd)
		{
			WCHAR wchClass[MAX_NAME_LENGTH];
			memset(wchClass, 0, MAX_NAME_LENGTH*sizeof(WCHAR));
			GetClassNameW(curhwnd, wchClass, MAX_NAME_LENGTH);

			WCHAR wchWindow[MAX_NAME_LENGTH];
			memset(wchWindow, 0, MAX_NAME_LENGTH*sizeof(WCHAR));
			GetWindowTextW(curhwnd, wchWindow, MAX_NAME_LENGTH);

			DWORD dwPid = 0;
			DWORD dwTid = 0;
			dwTid = GetWindowThreadProcessId(curhwnd, &dwPid);

			printf("===============================================================================\n");
			printf("GetCursorPos : p.x-%d, p.y-%d\n", p.x, p.y);
			printf("-------------------------------------------------------------------------------\n");
			printf("Window Information\n");
			printf("-------------------------------------------------------------------------------\n");
			printf("Current Hwnd : 0x%x\n", (DWORD)curhwnd);
			printf("ClssName : %ws\n", wchClass);
			printf("WindowTitle : %ws\n", wchWindow);
			printf("-------------------------------------------------------------------------------\n");
			printf("Process Information\n");
			printf("-------------------------------------------------------------------------------\n");
			printf("Pid : %d\n", dwPid);
			printf("Tid : %d\n", dwTid);

			WCHAR wchstr[MAX_COMMANDLINE_LENGTH];
			memset(wchstr, 0, MAX_COMMANDLINE_LENGTH* sizeof(WCHAR));
			if (GetCommandLine(dwPid, wchstr))
			{
				printf("CommandLine : %ws\n", wchstr);
			}
			prevhwnd = curhwnd;
			printf("===============================================================================\n");
			printf("\n\n");
		}
	}

	return 0;
}

