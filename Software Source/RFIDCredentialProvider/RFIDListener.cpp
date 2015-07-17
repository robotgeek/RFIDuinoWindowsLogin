//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) 2010 Amal Graafstra. All rights reserved.
//

#include "RFIDListener.h"
#include <strsafe.h>

// length of RFID data in badge
const int RFID_Length = 50;	// maximum RFID message length
TCHAR RFID_Port[] = _T("3"); //append COM port data to this string to set communications port (e.g. COM3)
TCHAR RFID_Lead[] = _T("ACK "); //preamble to look for to indicate an RFID tag ID is being sent
TCHAR RFID_Term[] = _T("\r\n"); //post tag ID string

CRFIDListener::CRFIDListener(void)
{
    _pProvider = NULL;
	_bQuit = FALSE;
	ZeroMemory(_UserName, sizeof(_UserName));
	ZeroMemory(_Password, sizeof(_Password));

	ReadSettings();
	ReadCredentials();
}

CRFIDListener::~CRFIDListener(void)
{
	// signal thread to die
	_bQuit = TRUE;
	// wait for it to go away, but no more than 10 seconds
	// this is a little overkill, since the thread should die in less than 2 seconds
	WaitForSingleObject(_hThread, 10000);

    // We'll also make sure to release any reference we have to the provider.
    if (_pProvider != NULL)
    {
        _pProvider->Release();
        _pProvider = NULL;
    }
}

// Read the credentials file C:\Windows\System32\RFIDCredentials.txt
// The file is formated with one set of credentials per line.
// The fields are separated by a pipe character '|'.
// Each set of credentials consists of three fields: 
//		RFID Tag ID
//		User Name
//		Password
// Domain logins are supported by prefixing the User Name field with Domain backslash:
//		Domain\User Name
// All the data is currently stored as clear text, and any use of this project in a 
// live environment should use some form of encryption to protect account credentials
void CRFIDListener::ReadCredentials()
{
	HANDLE hFile = CreateFile(_T("C:/Windows/System32/RFIDCredentials.txt"), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile != INVALID_HANDLE_VALUE)
	{
		// read the entire file into memory
		DWORD size = GetFileSize(hFile, NULL);
		char *pText = new char[size+1];
		ZeroMemory(pText, size+1);
		ReadFile(hFile, pText, size, &size, NULL);
		CloseHandle(hFile);
		// TODO: If entire file is encrypted, this is where decryption should occur
		CString text = pText;
		delete []pText;

		// parse text file by lines
		CString sLine;
		int line = 0;
		for (sLine = text.Tokenize(_T("\r\n"), line); line != -1; sLine = text.Tokenize(_T("\r\n"), line))
		{
			// add each credential, parsing the line by pipe characters '|'
			int nIndex = Credentials.Add();
			int field = 0;
			Credentials[nIndex].sID = sLine.Tokenize(_T("|"), field);
			Credentials[nIndex].sUserName = sLine.Tokenize(_T("|"), field);
			Credentials[nIndex].sPassword = sLine.Tokenize(_T("|"), field);
		}
	}
}

// Read the settings file C:\Windows\System32\RFIDCredSettings.txt
// The file is formated with one setting per line.
// The fields are separated by an = character.
// Expected settings are: 
//		COM=3
//		LEAD=
//		TERM=
void CRFIDListener::ReadSettings()
{
	HANDLE hFile = CreateFile(_T("C:/Windows/System32/RFIDCredSettings.txt"), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (hFile != INVALID_HANDLE_VALUE)
	{
		// read the entire file into memory
		DWORD size = GetFileSize(hFile, NULL);
		char *pText = new char[size+1];
		ZeroMemory(pText, size+1);
		ReadFile(hFile, pText, size, &size, NULL);
		CloseHandle(hFile);
		CString text = pText;
		delete []pText;

		// parse text file by lines
		CString sLine;
		int line = 0;
		for (sLine = text.Tokenize(_T("\r\n"), line); line != -1; sLine = text.Tokenize(_T("\r\n"), line))
		{
			// check lines and adjust settings
			if (sLine.Left(4)=="COM=") {_tcscpy(RFID_Port,sLine.Mid(4));}
			if (sLine.Left(5)=="LEAD=") {_tcscpy(RFID_Lead,sLine.Mid(5));}
			if (sLine.Left(5)=="TERM=") {
				sLine.Replace(_T("\\r"),_T("\r")); // replace carriage return
				sLine.Replace(_T("\\n"),_T("\n")); // replace new line
				_tcscpy(RFID_Term,sLine.Mid(5));
			}
		}

		if (0)	// change (0) to (1) for debugging purposes
		{
			CString msg;
			msg = CString(_T("Port: ")) + RFID_Port + CString(_T("\r\nLead: ")) + RFID_Lead + CString(_T("!!!\r\nTerm: ")) + RFID_Term;
			MessageBox(NULL, msg, _T(__FUNCTION__), MB_OK | MB_TOPMOST);
		}
	}
}

// Performs the work required to spin off our thread so we can listen for serial data
HRESULT CRFIDListener::Initialize(CRFIDCredentialProvider *pProvider)
{
    HRESULT hr = S_OK;

    // Be sure to add a release any existing provider we might have, then add a reference
    // to the provider we're working with now.
    if (_pProvider != NULL)
    {
        _pProvider->Release();
    }
    _pProvider = pProvider;
    _pProvider->AddRef();
    
    // Create and launch the window thread.
    _hThread = ::CreateThread(NULL, 0, CRFIDListener::_ThreadProc, (LPVOID) this, 0, NULL);
    if (_hThread == NULL)
    {
        hr = HRESULT_FROM_WIN32(::GetLastError());
    }

    return hr;
}

// Wraps our internal connected status so callers can easily access it.
BOOL CRFIDListener::GetConnectedStatus()
{
	// we are connected if we have a valid user name
	return lstrlen(_UserName);
}

void CRFIDListener::WaitForSerialData()
{
	// open the com port
	HANDLE hCom = CreateFile(CString(_T("\\\\.\\COM")) + RFID_Port, //RFID_Port should only contain "COM3" or "COM2" etc.
								GENERIC_READ,	 // we are only reading from the COM port
								0,               // exclusive access
								NULL,            // no security
								OPEN_EXISTING,
								0,               // no overlapped I/O
								NULL);           // null template 

	if (hCom == INVALID_HANDLE_VALUE)
		return;

	// initialize port with the correct baud rate, etc.
	DCB  dcb;
	FillMemory( &dcb, sizeof(dcb), 0 );

	BOOL bResult = FALSE;

	if (GetCommState(hCom, &dcb))
	{
		dcb.BaudRate    = CBR_9600;
		dcb.fBinary     = TRUE;
		dcb.ByteSize    = 8;
		dcb.fParity     = FALSE;
		dcb.Parity		= NOPARITY;
		dcb.StopBits    = ONESTOPBIT;
		// TODO: flow control is currently disabled, this can be turned on
		// if the RFID reader uses it
		dcb.fDtrControl = DTR_CONTROL_DISABLE;	//DTR_CONTROL_ENABLE;
		dcb.fRtsControl = RTS_CONTROL_DISABLE;	//RTS_CONTROL_ENABLE;

		if (SetCommState(hCom, &dcb))
		{
			COMMTIMEOUTS CommTimeouts = {0};

			CommTimeouts.ReadIntervalTimeout         = 100000/9600;	// 10x intercharacter timing
			CommTimeouts.ReadTotalTimeoutConstant    = 1000;		// 1 second timeout overall
			CommTimeouts.ReadTotalTimeoutMultiplier  = 0;

			if (SetCommTimeouts(hCom, &CommTimeouts))
			{
				bResult = TRUE;
			}
			else
			{
				CString msg;
				msg.Format(_T("SetCommTimeouts failed with error = %d"), GetLastError());
				MessageBox(NULL, msg, _T(__FUNCTION__), MB_OK | MB_TOPMOST);
			}
		}
		else
		{
			CString msg;
			msg.Format(_T("SetCommState failed with error = %d"), GetLastError());
			MessageBox(NULL, msg, _T(__FUNCTION__), MB_OK | MB_TOPMOST);
		}

	}
	else
		MessageBox(NULL, _T("GetCommState failed"), _T("WaitForSerialData"), MB_OK | MB_TOPMOST);


	if (bResult)
	{
		// buffer for RFID data
		TCHAR _RFID[RFID_Length+1];
		ZeroMemory(_RFID, sizeof(_RFID));
		int nIndex = 0;	// current buffer index

		enum RFIDState
		{
			Lead_in,
			ID_Data,
		};

		RFIDState state = Lead_in;
		int Lead_Length = lstrlen(RFID_Lead);
		int Term_Length = lstrlen(RFID_Term);

		while (!_bQuit)	// _bQuit is set when we want the thread to die
		{
			// read data 1 byte at a time
			char buffer;
			DWORD nBytesRead;
			ReadFile(hCom, &buffer, 1, &nBytesRead, NULL);
			// did we read a byte?
			if (nBytesRead)
			{
				// add it to our buffer
				_RFID[nIndex++] = buffer;

				if (0)	// change (0) to (1) for debugging purposes
				{
					CString msg;
					msg.Format(_T("Reading COM data:\r\nstate = %d\r\nnIndex = %d\r\nRFID = %s"), state, nIndex, _RFID);
					MessageBox(NULL, msg, _T(__FUNCTION__), MB_OK | MB_TOPMOST);
				}
				switch (state)
				{
				case Lead_in:
					if (_tcsncmp(_RFID, RFID_Lead, nIndex))
					{
						// mismatch, lead-in not found
						// if we have read in more than 1 character, shift the buffer
						// down and try again since the lead character may have garbage
						// in front of it which may mask it
						while ((nIndex > 1) && _tcsncmp(_RFID, RFID_Lead, nIndex))
						{
							CopyMemory(&_RFID[0], &_RFID[1], nIndex--);
						}
						_RFID[nIndex--] = 0;	// ignore char
					}
					else if (nIndex == Lead_Length)
					{
						state = ID_Data;
						// started reading a new badge, so clear current user data
						ZeroMemory(_UserName, sizeof(_UserName));
						ZeroMemory(_Password, sizeof(_Password));
						ZeroMemory(_RFID, sizeof(_RFID));
						nIndex = 0;
					}
					break;

				case ID_Data:
					if (nIndex > Term_Length)
					{
						// found termination characters
						if (!_tcsncmp(&_RFID[nIndex-Term_Length], RFID_Term, Term_Length))
						{
							// remove termination characters
							_RFID[nIndex-Term_Length] = 0;

							TCHAR sUserData[200];
							ZeroMemory(sUserData, sizeof(sUserData));
							ZeroMemory(_UserName, sizeof(_UserName));
							ZeroMemory(_Password, sizeof(_Password));
							//state = Lead_in;

							// find ID in our credentials file
							for (size_t i = 0; i < Credentials.GetCount(); ++i)
							{
								// when found, set the user name and password
								// and signal the provider
								if (Credentials[i].sID == _RFID)
								{
									lstrcpy(_UserName, Credentials[i].sUserName);
									lstrcpy(_Password, Credentials[i].sPassword);
									_pProvider->OnConnectStatusChanged();
									break;
								}
							}
							// clear the buffer once we are done with it
							ZeroMemory(_RFID, sizeof(_RFID));
							nIndex = 0;
							state = Lead_in;
						}
						// if buffer limit has been reached, clear it and start over, something is wrong
						if (nIndex == RFID_Length-1)
						{
							// started reading a new badge, so clear current user data
							ZeroMemory(_UserName, sizeof(_UserName));
							ZeroMemory(_Password, sizeof(_Password));
							ZeroMemory(_RFID, sizeof(_RFID));
							nIndex = 0;
							state = Lead_in;
						}
					}
					break;
				}
			}
		}
	}
	else
		MessageBox(NULL, _T("Failed to initialize port"), _T(__FUNCTION__), MB_OK | MB_TOPMOST);

	CloseHandle(hCom);
	return;
}

// Our thread procedure which waits for data on the serial port
DWORD WINAPI CRFIDListener::_ThreadProc(LPVOID lpParameter)
{
    CRFIDListener *pCommandWindow = static_cast<CRFIDListener *>(lpParameter);
    if (pCommandWindow == NULL)
    {
        // TODO: What's the best way to raise this error? This is a programming error
		// and should never actually ever occur.
        return 0;
    }

	// go read the serial data
	pCommandWindow->WaitForSerialData();

	return 0;
}

void CRFIDListener::Disconnect()
{
	MessageBox(NULL, _T("Disconnected"), _T(__FUNCTION__), MB_OK | MB_TOPMOST);
	ZeroMemory(_UserName, sizeof(_UserName));
	ZeroMemory(_Password, sizeof(_Password));
	_pProvider->OnConnectStatusChanged();
}
