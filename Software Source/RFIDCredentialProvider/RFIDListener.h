//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) 2006 Microsoft Corporation. All rights reserved.
//
// CCommandWindow provides a way to emulate external "connect" and "disconnect" 
// events, which are invoked via toggle button on a window. The window is launched
// and managed on a separate thread, which is necessary to ensure it gets pumped.
//

#pragma once

#pragma warning(disable:4995 4996)

#include <windows.h>
#include <atlstr.h>
#include "RFIDCredentialProvider.h"
#include <atlcoll.h>

class CRFIDListener
{
public:
    CRFIDListener(void);
    ~CRFIDListener(void);

    HRESULT Initialize(CRFIDCredentialProvider *pProvider);
    BOOL GetConnectedStatus();
	LPCTSTR GetUserName() { return _UserName; }
	LPCTSTR GetPassword() { return _Password; }
	void Disconnect();

private:
    static DWORD WINAPI _ThreadProc(LPVOID lpParameter);
	void WaitForSerialData();
	void ReadCredentials();
	void ReadSettings();
	CRFIDCredentialProvider                *_pProvider;        // Pointer to our owner.

	// These are TCHAR arrays and not CStrings because of problems with 
	// cross thread memory allocations in the CString class
	TCHAR						_UserName[100];
	TCHAR						_Password[100];
	CString						_RFID;
	volatile BOOL				_bQuit;
	HANDLE						_hThread;

	class RFIDCredential
	{
	public:
		CString sID;
		CString sUserName;
		CString sPassword;
	};

	// an array of credentials read from a file
	CAtlArray<RFIDCredential> Credentials;
};
