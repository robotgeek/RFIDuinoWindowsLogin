//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) 2010 Amal Graafstra. All rights reserved.
//
//

#ifndef WIN32_NO_STATUS
#include <ntstatus.h>
#define WIN32_NO_STATUS
#endif
#include <unknwn.h>
#include "RFIDCredential.h"
#include "guid.h"
#include "RFIDListener.h"

#define DEBUG 0


// CRFIDCredential ////////////////////////////////////////////////////////

CRFIDCredential::CRFIDCredential():
    _cRef(1),
    _pCredProvCredentialEvents(NULL)
{
    DllAddRef();

    ZeroMemory(_rgCredProvFieldDescriptors, sizeof(_rgCredProvFieldDescriptors));
    ZeroMemory(_rgFieldStatePairs, sizeof(_rgFieldStatePairs));
    ZeroMemory(_rgFieldStrings, sizeof(_rgFieldStrings));

	_pListener = NULL;
}

CRFIDCredential::~CRFIDCredential()
{
    if (_rgFieldStrings[SFI_PASSWORD])
    {
        // CoTaskMemFree (below) deals with NULL, but StringCchLength does not.
        size_t lenPassword;
        HRESULT hr = StringCchLengthW(_rgFieldStrings[SFI_PASSWORD], 128, &(lenPassword));
        if (SUCCEEDED(hr))
        {
            SecureZeroMemory(_rgFieldStrings[SFI_PASSWORD], lenPassword * sizeof(*_rgFieldStrings[SFI_PASSWORD]));
        }
        else
        {
            // TODO: Determine how to handle count error here.
        }
    }
    for (int i = 0; i < ARRAYSIZE(_rgFieldStrings); i++)
    {
        CoTaskMemFree(_rgFieldStrings[i]);
        CoTaskMemFree(_rgCredProvFieldDescriptors[i].pszLabel);
    }

    DllRelease();
}

void CRFIDCredential::SetUserData(LPCTSTR sUserName, LPCTSTR sPassword)
{
	SHStrDupW(sUserName, &_rgFieldStrings[SFI_USERNAME]);
	SHStrDupW(sPassword, &_rgFieldStrings[SFI_PASSWORD]);
	if (_pCredProvCredentialEvents)
	{
		_pCredProvCredentialEvents->SetFieldString(this, SFI_USERNAME, _rgFieldStrings[SFI_USERNAME]);
		_pCredProvCredentialEvents->SetFieldString(this, SFI_PASSWORD, _rgFieldStrings[SFI_PASSWORD]);
	}
}

// Initializes one credential with the field information passed in.
HRESULT CRFIDCredential::Initialize(
    CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
    const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR* rgcpfd,
    const FIELD_STATE_PAIR* rgfsp,
    CRFIDListener * pListener)
{
    HRESULT hr = S_OK;

    _cpus = cpus;
	_pListener = pListener;

    // Copy the field descriptors for each field. This is useful if you want to vary the field
    // descriptors based on what Usage scenario the credential was created for.
    for (DWORD i = 0; SUCCEEDED(hr) && i < ARRAYSIZE(_rgCredProvFieldDescriptors); i++)
    {
        _rgFieldStatePairs[i] = rgfsp[i];
        hr = FieldDescriptorCopy(rgcpfd[i], &_rgCredProvFieldDescriptors[i]);
    }

    if (SUCCEEDED(hr))
    {
        hr = SHStrDupW(L"Submit", &_rgFieldStrings[SFI_SUBMIT_BUTTON]);
    }

    return S_OK;
}

// LogonUI calls this in order to give us a callback in case we need to notify it of anything.
HRESULT CRFIDCredential::Advise(
    ICredentialProviderCredentialEvents* pcpce
    )
{
    if (_pCredProvCredentialEvents != NULL)
    {
        _pCredProvCredentialEvents->Release();
    }
    _pCredProvCredentialEvents = pcpce;
    _pCredProvCredentialEvents->AddRef();

    return S_OK;
}

// LogonUI calls this to tell us to release the callback.
HRESULT CRFIDCredential::UnAdvise()
{
    if (_pCredProvCredentialEvents)
    {
        _pCredProvCredentialEvents->Release();
    }
    _pCredProvCredentialEvents = NULL;
    return S_OK;
}

// LogonUI calls this function when our tile is selected (zoomed)
// If you simply want fields to show/hide based on the selected state,
// there's no need to do anything here - you can set that up in the 
// field definitions.  But if you want to do something
// more complicated, like change the contents of a field when the tile is
// selected, you would do it here.
HRESULT CRFIDCredential::SetSelected(BOOL* pbAutoLogon)  
{
    *pbAutoLogon = _pListener->GetConnectedStatus();  
    return S_OK;
}

// Similarly to SetSelected, LogonUI calls this when your tile was selected
// and now no longer is.  The most common thing to do here (which we do below)
// is to clear out the password field.
HRESULT CRFIDCredential::SetDeselected()
{
    HRESULT hr = S_OK;
    if (_rgFieldStrings[SFI_PASSWORD] && _pCredProvCredentialEvents)
        _pCredProvCredentialEvents->SetFieldString(this, SFI_PASSWORD, _rgFieldStrings[SFI_PASSWORD]);

    return hr;
}

// Get info for a particular field of a tile. Called by LogonUI to get information to 
// display the tile.
HRESULT CRFIDCredential::GetFieldState(
    DWORD dwFieldID,
    CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs,
    CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis
    )
{
    HRESULT hr;
    
    if (dwFieldID < ARRAYSIZE(_rgFieldStatePairs) && pcpfs && pcpfis)
    {
        *pcpfis = _rgFieldStatePairs[dwFieldID].cpfis;
        *pcpfs = _rgFieldStatePairs[dwFieldID].cpfs;

        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

// Sets ppwsz to the string value of the field at the index dwFieldID.
HRESULT CRFIDCredential::GetStringValue(
    DWORD dwFieldID, 
    PWSTR* ppwsz
    )
{
    HRESULT hr;

    // Check to make sure dwFieldID is a legitimate index.
    if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) && ppwsz) 
    {
        // Make a copy of the string and return that. The caller
        // is responsible for freeing it.
        hr = SHStrDupW(_rgFieldStrings[dwFieldID], ppwsz);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

// Get the image to show in the user tile.
HRESULT CRFIDCredential::GetBitmapValue(
    DWORD dwFieldID, 
    HBITMAP* phbmp
    )
{
    //UNREFERENCED_PARAMETER(dwFieldID);
    //UNREFERENCED_PARAMETER(phbmp);
    //return E_NOTIMPL;

    HRESULT hr;
    if ((SFI_TILEIMAGE == dwFieldID) && phbmp)
    {
        HBITMAP hbmp = LoadBitmap(HINST_THISDLL, MAKEINTRESOURCE(IDB_TILE_IMAGE));
        if (hbmp != NULL)
        {
            hr = S_OK;
            *phbmp = hbmp;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

// Sets pdwAdjacentTo to the index of the field the submit button should be 
// adjacent to. We recommend that the submit button is placed next to the last
// field which the user is required to enter information in. Optional fields
// should be below the submit button.
HRESULT CRFIDCredential::GetSubmitButtonValue(
    DWORD dwFieldID,
    DWORD* pdwAdjacentTo
    )
{
    HRESULT hr;

    if (SFI_SUBMIT_BUTTON == dwFieldID && pdwAdjacentTo)
    {
        // pdwAdjacentTo is a pointer to the fieldID you want the submit button to 
        // appear next to.
        *pdwAdjacentTo = SFI_PASSWORD;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

// Sets the value of a field which can accept a string as a value.
// This is called on each keystroke when a user types into an edit field
HRESULT CRFIDCredential::SetStringValue(
    DWORD dwFieldID, 
    PCWSTR pwz      
    )
{
    HRESULT hr;

    if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) && 
       (CPFT_EDIT_TEXT == _rgCredProvFieldDescriptors[dwFieldID].cpft || 
        CPFT_PASSWORD_TEXT == _rgCredProvFieldDescriptors[dwFieldID].cpft)) 
    {
        PWSTR* ppwszStored = &_rgFieldStrings[dwFieldID];
        CoTaskMemFree(*ppwszStored);

        hr = SHStrDupW(pwz, ppwszStored);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//------------- 
// The following methods are for LogonUI to get the values of various UI elements and then communicate
// to the credential about what the user did in that field.  However, these methods are not implemented
// because our tile doesn't contain these types of UI elements
HRESULT CRFIDCredential::GetCheckboxValue(
    DWORD dwFieldID, 
    BOOL* pbChecked,
    PWSTR* ppwszLabel
    )
{
    UNREFERENCED_PARAMETER(dwFieldID);
    UNREFERENCED_PARAMETER(pbChecked);
    UNREFERENCED_PARAMETER(ppwszLabel);

    return E_NOTIMPL;
}

HRESULT CRFIDCredential::GetComboBoxValueCount(
    DWORD dwFieldID, 
    DWORD* pcItems, 
    DWORD* pdwSelectedItem
    )
{
    UNREFERENCED_PARAMETER(dwFieldID);
    UNREFERENCED_PARAMETER(pcItems);
    UNREFERENCED_PARAMETER(pdwSelectedItem);
    return E_NOTIMPL;
}

HRESULT CRFIDCredential::GetComboBoxValueAt(
    DWORD dwFieldID, 
    DWORD dwItem,
    PWSTR* ppwszItem
    )
{
    UNREFERENCED_PARAMETER(dwFieldID);
    UNREFERENCED_PARAMETER(dwItem);
    UNREFERENCED_PARAMETER(ppwszItem);
    return E_NOTIMPL;
}

HRESULT CRFIDCredential::SetCheckboxValue(
    DWORD dwFieldID, 
    BOOL bChecked
    )
{
    UNREFERENCED_PARAMETER(dwFieldID);
    UNREFERENCED_PARAMETER(bChecked);

    return E_NOTIMPL;
}

HRESULT CRFIDCredential::SetComboBoxSelectedValue(
    DWORD dwFieldId,
    DWORD dwSelectedItem
    )
{
    UNREFERENCED_PARAMETER(dwFieldId);
    UNREFERENCED_PARAMETER(dwSelectedItem);
    return E_NOTIMPL;
}

HRESULT CRFIDCredential::CommandLinkClicked(DWORD dwFieldID)
{
    UNREFERENCED_PARAMETER(dwFieldID);
    return E_NOTIMPL;
}
//------ end of methods for controls we don't have in our tile ----//

// Collect the username and password into a serialized credential for the correct usage scenario 
// (logon/unlock is what's demonstrated in this sample).  LogonUI then passes these credentials 
// back to the system to log on.
HRESULT CRFIDCredential::GetSerialization(
    CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
    CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs, 
    PWSTR* ppwszOptionalStatusText, 
    CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon
    )
{
    UNREFERENCED_PARAMETER(ppwszOptionalStatusText);
    UNREFERENCED_PARAMETER(pcpsiOptionalStatusIcon);

    KERB_INTERACTIVE_LOGON kil;
    ZeroMemory(&kil, sizeof(kil));

    HRESULT hr;

    WCHAR wsz[MAX_COMPUTERNAME_LENGTH+1];
    DWORD cch = ARRAYSIZE(wsz);
	BOOL bResult = TRUE;

	CString sUserName = _rgFieldStrings[SFI_USERNAME];
	// is there a slash indicating a domain login?
	int pos = sUserName.Find(_T('\\'));
	if (pos != -1)
	{
		// split the domain and user name into two fields
		lstrcpy(wsz, sUserName.Left(pos));
		sUserName = sUserName.Mid(pos+1);
	}
	else
		bResult = GetComputerNameW(wsz, &cch);	// get computer name for local login
    
	if (DEBUG) // change (0) to (1) for debug purposes
	{
		CString msg;
		msg.Format(_T("User = %s\r\nDomain = %s\r\nPassword = %s"), sUserName, wsz, _rgFieldStrings[SFI_PASSWORD]);
		MessageBox(NULL, msg, _T(__FUNCTION__), MB_OK | MB_TOPMOST);
	}
	
	if (bResult)
	{
		PWSTR pwzProtectedPassword;

        hr = ProtectIfNecessaryAndCopyPassword(_rgFieldStrings[SFI_PASSWORD], _cpus, &pwzProtectedPassword);

        if (SUCCEEDED(hr))
        {
            KERB_INTERACTIVE_UNLOCK_LOGON kiul;

            // Initialize kiul with weak references to our credential.
            hr = KerbInteractiveUnlockLogonInit(wsz, (PWSTR)(LPCTSTR)sUserName, pwzProtectedPassword, _cpus, &kiul);

            if (SUCCEEDED(hr))
            {
                // We use KERB_INTERACTIVE_UNLOCK_LOGON in both unlock and logon scenarios.  It contains a
                // KERB_INTERACTIVE_LOGON to hold the creds plus a LUID that is filled in for us by Winlogon
                // as necessary.
                hr = KerbInteractiveUnlockLogonPack(kiul, &pcpcs->rgbSerialization, &pcpcs->cbSerialization);

                if (SUCCEEDED(hr))
                {
                    ULONG ulAuthPackage;
                    hr = RetrieveNegotiateAuthPackage(&ulAuthPackage);
                    if (SUCCEEDED(hr))
                    {
                        pcpcs->ulAuthenticationPackage = ulAuthPackage;
                        pcpcs->clsidCredentialProvider = CLSID_CSampleProvider;
 
                        // At this point the credential has created the serialized credential used for logon
                        // By setting this to CPGSR_RETURN_CREDENTIAL_FINISHED we are letting LogonUI know
                        // that we have all the information we need and it should attempt to submit the 
                        // serialized credential.
                        *pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
                    }
                }
            }

            CoTaskMemFree(pwzProtectedPassword);
        }
    }
    else
    {
        DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }

    return hr;
}
struct REPORT_RESULT_STATUS_INFO
{
    NTSTATUS ntsStatus;
    NTSTATUS ntsSubstatus;
    PWSTR     pwzMessage;
    CREDENTIAL_PROVIDER_STATUS_ICON cpsi;
};

static const REPORT_RESULT_STATUS_INFO s_rgLogonStatusInfo[] =
{
    { STATUS_LOGON_FAILURE, STATUS_SUCCESS, L"Incorrect password or username.", CPSI_ERROR, },
    { STATUS_ACCOUNT_RESTRICTION, STATUS_ACCOUNT_DISABLED, L"The account is disabled.", CPSI_WARNING },
};

// ReportResult is completely optional.  Its purpose is to allow a credential to customize the string
// and the icon displayed in the case of a logon failure.  For example, we have chosen to 
// customize the error shown in the case of bad username/password and in the case of the account
// being disabled.
HRESULT CRFIDCredential::ReportResult(
    NTSTATUS ntsStatus, 
    NTSTATUS ntsSubstatus,
    PWSTR* ppwszOptionalStatusText, 
    CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon
    )
{
    *ppwszOptionalStatusText = NULL;
    *pcpsiOptionalStatusIcon = CPSI_NONE;

    DWORD dwStatusInfo = (DWORD)-1;

    // Look for a match on status and substatus.
    for (DWORD i = 0; i < ARRAYSIZE(s_rgLogonStatusInfo); i++)
    {
        if (s_rgLogonStatusInfo[i].ntsStatus == ntsStatus && s_rgLogonStatusInfo[i].ntsSubstatus == ntsSubstatus)
        {
            dwStatusInfo = i;
            break;
        }
    }

    if ((DWORD)-1 != dwStatusInfo)
    {
        if (SUCCEEDED(SHStrDupW(s_rgLogonStatusInfo[dwStatusInfo].pwzMessage, ppwszOptionalStatusText)))
        {
            *pcpsiOptionalStatusIcon = s_rgLogonStatusInfo[dwStatusInfo].cpsi;
        }
    }

    // If we failed the logon, try to erase the password field.
    if (!SUCCEEDED(HRESULT_FROM_NT(ntsStatus)))
    {
		MessageBox(NULL, _T("Login failed"), _T(__FUNCTION__), MB_OK | MB_TOPMOST);
        if (_pCredProvCredentialEvents)
        {
            _pCredProvCredentialEvents->SetFieldString(this, SFI_PASSWORD, L"");
        }
		// clear credentials
		SetUserData(_T(""), _T(""));
		// disconnect listener if login failed
		if (_pListener)
			_pListener->Disconnect();
    }

    // Since NULL is a valid value for *ppwszOptionalStatusText and *pcpsiOptionalStatusIcon
    // this function can't fail.
    return S_OK;
}
