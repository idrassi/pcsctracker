///////////////////////////////////////////////////////////////////////////////
// Name:        PcscTracker.cpp
// Purpose:     Implementation of PC/SC helper functions and classes
// Author:      Mounir IDRASSI
// Created:     2014-05-14
// Copyright:   (c) 2014 Mounir IDRASSI <mounir.idrassi@idrix.fr>
// Licence:     GPLv3/MIT
///////////////////////////////////////////////////////////////////////////////

#include "PcscTracker.h"


#ifndef _WIN32

VOID Sleep(DWORD dwMilliseconds)
{
	struct timespec timeoutValue;
	timeoutValue.tv_sec = dwMilliseconds / 1000;
	timeoutValue.tv_nsec = (dwMilliseconds % 1000) * 1000000;
	
	nanosleep(&timeoutValue, NULL);
}

/*
 * Critical sections functions
 */

void InitializeCriticalSection(
  LPCRITICAL_SECTION lpCriticalSection
)
{
	if(lpCriticalSection)
	{
		pthread_mutexattr_t attr;

		pthread_mutexattr_init(&attr);
		pthread_mutexattr_settype(&attr,PTHREAD_MUTEX_RECURSIVE);
		pthread_mutex_init(lpCriticalSection,&attr);
		pthread_mutexattr_destroy(&attr);
	}
}

void DeleteCriticalSection(
  LPCRITICAL_SECTION lpCriticalSection
)
{
	if(lpCriticalSection)
		pthread_mutex_destroy(lpCriticalSection);
}

void EnterCriticalSection(
  LPCRITICAL_SECTION lpCriticalSection
)
{
	if(lpCriticalSection)
		pthread_mutex_lock(lpCriticalSection);
}

void LeaveCriticalSection(
  LPCRITICAL_SECTION lpCriticalSection
)
{
	if(lpCriticalSection)
		pthread_mutex_unlock(lpCriticalSection);
}

/*
 * dynamic library support
 */
HMODULE LoadLibrary(const char* lpFileName)
{
	return dlopen(lpFileName, RTLD_NOW|RTLD_LOCAL);
}

BOOL FreeLibrary(HMODULE hModule)
{
	if(0 == dlclose(hModule))
		return TRUE;
	else
		return FALSE;
}

void* GetProcAddress(HMODULE hModule, const char* lpProcName)
{
	return dlsym(hModule, lpProcName);
}

#endif




/***************************************************************************************************/
bool Pcsc::Initialize(void)
{
	if (!m_hDll)
	{
		m_SCardEstablishContext = NULL;
		m_SCardReleaseContext = NULL;
		m_SCardIsValidContext = NULL;
		m_SCardListReaders = NULL;
		m_SCardGetStatusChange = NULL;
		m_SCardCancel = NULL;
#ifdef _WIN32
		m_hDll = LoadLibrary(_T("winscard.dll"));
#elif __APPLE__
		m_hDll = LoadLibrary("/System/Library/Frameworks/PCSC.framework/PCSC");
#else
		m_hDll = LoadLibrary("libpcsclite.so.1");
		if (!m_hDll)
		{
#ifdef __LP64__
			m_hDll = LoadLibrary("/lib/x86_64-linux-gnu/libpcsclite.so.1");			
#else
			m_hDll = LoadLibrary("/lib/i386-linux-gnu/libpcsclite.so.1");			
#endif			
		}
#endif
		if (m_hDll)
		{
			m_SCardEstablishContext = (SCardEstablishContextPtr) GetProcAddress(m_hDll, "SCardEstablishContext");
			m_SCardReleaseContext = (SCardReleaseContextPtr) GetProcAddress(m_hDll, "SCardReleaseContext");
			m_SCardIsValidContext = (SCardIsValidContextPtr) GetProcAddress(m_hDll, "SCardIsValidContext");
#ifdef _WIN32
#if defined(UNICODE) || defined(_UNICODE)
			m_SCardListReaders = (SCardListReadersPtr) GetProcAddress(m_hDll, "SCardListReadersW");
			m_SCardGetStatusChange = (SCardGetStatusChangePtr) GetProcAddress(m_hDll, "SCardGetStatusChangeW");
#else
			m_SCardListReaders = (SCardListReadersPtr) GetProcAddress(m_hDll, "SCardListReadersA");
			m_SCardGetStatusChange = (SCardGetStatusChangePtr) GetProcAddress(m_hDll, "SCardGetStatusChangeA");
#endif
#else
			m_SCardListReaders = (SCardListReadersPtr) GetProcAddress(m_hDll, "SCardListReaders");
			m_SCardGetStatusChange = (SCardGetStatusChangePtr) GetProcAddress(m_hDll, "SCardGetStatusChange");
#endif	
			m_SCardCancel = (SCardCancelPtr) GetProcAddress(m_hDll, "SCardCancel");

			if (!m_SCardEstablishContext || !m_SCardReleaseContext || !m_SCardIsValidContext || !m_SCardListReaders
				|| !m_SCardGetStatusChange || !m_SCardCancel)
			{
				Finalize();
			}
		}
	}

	return m_hDll != NULL;
}

void Pcsc::Finalize(void)
{
	if (m_hDll)
	{
		FreeLibrary(m_hDll);
		m_hDll = NULL;
		m_SCardEstablishContext = NULL;
		m_SCardReleaseContext = NULL;
		m_SCardIsValidContext = NULL;
		m_SCardListReaders = NULL;
		m_SCardGetStatusChange = NULL;
		m_SCardCancel = NULL;
	}
}

LONG Pcsc::EstablishContext(DWORD dwScope, LPCVOID pvReserved1, LPCVOID pvReserved2, LPSCARDCONTEXT phContext)
{
	if (!m_SCardEstablishContext && !Initialize())
	{
		return SCARD_E_NO_SERVICE;
	}
	else
		return m_SCardEstablishContext(dwScope, pvReserved1, pvReserved2, phContext);
}

LONG Pcsc::ReleaseContext(SCARDCONTEXT hContext)
{
	if (!m_SCardReleaseContext)
		return SCARD_E_NO_SERVICE;
	else
		return m_SCardReleaseContext(hContext);
}

LONG Pcsc::IsValidContext(SCARDCONTEXT hContext)
{
	if (!m_SCardIsValidContext)
		return SCARD_E_NO_SERVICE;
	else
		return m_SCardIsValidContext(hContext);
}

LONG Pcsc::ListReaders(SCARDCONTEXT hContext, LPCTSTR mszGroups, LPTSTR mszReaders, LPDWORD pcchReaders)
{
	if (!m_SCardListReaders)
		return SCARD_E_NO_SERVICE;
	else
		return m_SCardListReaders(hContext, mszGroups, mszReaders, pcchReaders);
}

LONG Pcsc::GetStatusChange(SCARDCONTEXT hContext, DWORD dwTimeout, SCARD_READERSTATE* rgReaderStates, DWORD cReaders)
{
	if (!m_SCardGetStatusChange)
		return SCARD_E_NO_SERVICE;
	else
		return m_SCardGetStatusChange(hContext, dwTimeout, rgReaderStates, cReaders);
}

LONG Pcsc::Cancel(SCARDCONTEXT hContext)
{
	if (!m_SCardCancel)
		return SCARD_E_NO_SERVICE;
	else
		return m_SCardCancel(hContext);
}

/****************************************************************************************************/
DWORD PcscTracker::g_dwPollingPeriod = 1000;
DWORD PcscTracker::g_pcscScope = 
#ifdef _WIN32
	SCARD_SCOPE_USER
#else
	SCARD_SCOPE_SYSTEM
#endif
	;

void PcscTracker::TrackerProc(void)
{
	LONG err;
	LPCTSTR mszXReaderNames = NULL;
	DWORD cXReaderStates = 0;
	SCARD_READERSTATE *pXReaderStates = NULL;
	DWORD dwTimeout;
	BOOL bPnpAvailable = FALSE, bPnpChecked = FALSE, bRebuildReadersList = TRUE;
#if defined(UNICODE) || defined(_UNICODE)
	std::wstring szPnpReaderName(L"\\\\?PnP?\\Notification");
#else
	std::string szPnpReaderName("\\\\?PnP?\\Notification");
#endif

	m_bIsRunning = true;

	EnterCriticalSection(&m_lock);
	while (!m_bStop) {

		if (m_hContext == 0) {
			// trying to establish context
			err = m_pcsc.EstablishContext(g_pcscScope, NULL, NULL, &m_hContext);
			if (err != SCARD_S_SUCCESS) {
				m_hContext = 0;
				LeaveCriticalSection(&m_lock);
				Sleep(g_dwPollingPeriod);
				EnterCriticalSection(&m_lock);
				
				if (m_bStop)
					break;
			}
		}

		if (m_hContext) {
			
			// check to see if PNP is supported and set the correct timeout value
			if (!bPnpChecked)
			{
#ifdef DISABLE_PCSC_PNP
				dwTimeout = g_dwPollingPeriod;
				bPnpAvailable = FALSE;
#else
				SCARD_READERSTATE pnpState;
				pnpState.szReader = szPnpReaderName.c_str();
				pnpState.dwCurrentState = SCARD_STATE_UNAWARE;

				err = m_pcsc.GetStatusChange(m_hContext, 0, &pnpState, 1);
				if (pnpState.dwEventState & SCARD_STATE_UNKNOWN)
				{
					dwTimeout = g_dwPollingPeriod;
					bPnpAvailable = FALSE;
				}
				else
				{
					dwTimeout = INFINITE;
					bPnpAvailable = TRUE;
				}
#endif
				bPnpChecked = TRUE;
			}

			// rescan reader list...
			LPTSTR mszNewReaderNames = NULL;
			DWORD cchReaderNames = 0;

			if (SCARD_S_SUCCESS == m_pcsc.IsValidContext(m_hContext))
			{			
				if (bRebuildReadersList)
				{
					do {
						err = m_pcsc.ListReaders(m_hContext, NULL, NULL, &cchReaderNames);
						if (err == SCARD_S_SUCCESS) {
							mszNewReaderNames = new TCHAR[cchReaderNames];
							err = m_pcsc.ListReaders(m_hContext, NULL, mszNewReaderNames, &cchReaderNames);
							if (err != SCARD_S_SUCCESS) {
								delete[] mszNewReaderNames;
								mszNewReaderNames = NULL;
							}
						}
					}
					while ((err == SCARD_E_INSUFFICIENT_BUFFER) && !m_bStop);
				
					if (m_bStop)
						break;

				}
				else
				{
					if (mszXReaderNames)
					{
						mszNewReaderNames = new TCHAR[_tcslen(mszXReaderNames) + 1];
						_tcscpy(mszNewReaderNames, mszXReaderNames);
						err = SCARD_S_SUCCESS;								
					}
					else
						err = SCARD_E_NO_READERS_AVAILABLE;				
				}
			}
			else
			{
				err = SCARD_E_SERVICE_STOPPED;
			}

			// check reader listing status
			DWORD cNewReaderStates = 0;
			SCARD_READERSTATE *pNewReaderStates = NULL;
			switch (err) {
			case SCARD_E_INVALID_HANDLE:
			case SCARD_E_NO_SERVICE:
			case SCARD_E_SERVICE_STOPPED:
				m_pcsc.ReleaseContext(m_hContext);
				m_hContext = 0;
				err = SCARD_S_SUCCESS;
				break;
			case SCARD_E_NO_READERS_AVAILABLE:
				err = SCARD_S_SUCCESS;
				break;
			case SCARD_S_SUCCESS:
				for (LPCTSTR pReader = mszNewReaderNames; *pReader != 0; pReader += _tcslen(pReader) + 1)
					cNewReaderStates++;
				if (bPnpAvailable) // allocate one more element for PNP
					cNewReaderStates++;
				if (cNewReaderStates)
				{
					pNewReaderStates = new SCARD_READERSTATE[cNewReaderStates];
					memset(pNewReaderStates, 0, cNewReaderStates* sizeof(SCARD_READERSTATE));
				}
			default:
				break;
			}
			
			if (err == SCARD_S_SUCCESS)
			{
				// compare readers
				DWORD j;
				for (j = 0; j < cXReaderStates; j++)
					pXReaderStates[j].pvUserData = (LPVOID)1;
				LPCTSTR pReader = mszNewReaderNames;
				DWORD i = 0;
				if (pReader) {
					while (*pReader != 0) {
						pNewReaderStates[i].pvUserData = (LPVOID)1;
						pNewReaderStates[i].dwCurrentState = SCARD_STATE_UNAWARE;
						for (j = 0; j < cXReaderStates; j++) {
							if (_tcscmp(pReader, pXReaderStates[j].szReader) == 0) {
								pXReaderStates[j].pvUserData = NULL;
								pNewReaderStates[i] = pXReaderStates[j];
								break;
							}
						}
						pNewReaderStates[i].szReader = pReader;
						pReader += _tcslen(pReader) + 1;
						i++;
					}
					
					if (bPnpAvailable)
					{
						// we have already allocated one more element for this case
						pNewReaderStates[i].szReader = szPnpReaderName.c_str();
						if (cXReaderStates)
							pNewReaderStates[i].dwCurrentState = pXReaderStates[cXReaderStates - 1].dwCurrentState;
						else
							pNewReaderStates[i].dwCurrentState = SCARD_STATE_UNAWARE;	
					}					
				}				

				// check removed readers
				for (j = 0; j < ((bPnpAvailable && cXReaderStates)? cXReaderStates - 1 : cXReaderStates); j++) {
					if (pXReaderStates[j].pvUserData != NULL) {
						LeaveCriticalSection(&m_lock);
						notifyReaderRemoved(pXReaderStates[j].szReader);
						EnterCriticalSection(&m_lock);
					}
				}

				// update current list
				if (pXReaderStates)
					delete[] pXReaderStates;
				if (mszXReaderNames)
					delete[] mszXReaderNames;
				mszXReaderNames = mszNewReaderNames;
				pXReaderStates = pNewReaderStates;
				cXReaderStates = cNewReaderStates;
			}

			// check reader states
			if (cXReaderStates == 0) {
				LeaveCriticalSection(&m_lock);
				if (bPnpAvailable && m_hContext)
				{
					// wait for reader insertion using PNP
					SCARD_READERSTATE pnpState;
					pnpState.szReader = szPnpReaderName.c_str();
					pnpState.dwCurrentState = SCARD_STATE_UNAWARE;

					m_bBlockingCall = true;
					err = m_pcsc.GetStatusChange(m_hContext, INFINITE, &pnpState, 1);
					m_bBlockingCall = false;
					bRebuildReadersList = TRUE;
				}
				else
					Sleep(g_dwPollingPeriod);
				EnterCriticalSection(&m_lock);
				
				if (m_bStop)
					break;				
			}
			else {
				
				do
				{
					LeaveCriticalSection(&m_lock);
					m_bBlockingCall = true;
					err = m_pcsc.GetStatusChange(m_hContext, dwTimeout, pXReaderStates, cXReaderStates);
					m_bBlockingCall = false;
					EnterCriticalSection(&m_lock);
					
					if (m_bStop || (SCARD_E_CANCELLED == err))
						break;

					if (err == SCARD_S_SUCCESS) {
						// check changed readers...
						for (DWORD i = 0; i < (bPnpAvailable? cXReaderStates - 1 : cXReaderStates); i++) {
							if (pXReaderStates[i].pvUserData != NULL) {
								// new reader
								pXReaderStates[i].pvUserData = NULL;
								pXReaderStates[i].dwEventState &= ~SCARD_STATE_CHANGED;
								LeaveCriticalSection(&m_lock);
								notifyReaderAdded(&pXReaderStates[i]);
								EnterCriticalSection(&m_lock);
								pXReaderStates[i].dwCurrentState = pXReaderStates[i].dwEventState;
							}
							else if (pXReaderStates[i].dwEventState & SCARD_STATE_CHANGED) {
								// existing reader
								pXReaderStates[i].dwEventState &= ~SCARD_STATE_CHANGED;
								if ((pXReaderStates[i].dwEventState & SCARD_STATE_IGNORE) == 0) {
									LeaveCriticalSection(&m_lock);
									notifyStateChange(&pXReaderStates[i]);
									EnterCriticalSection(&m_lock);
									pXReaderStates[i].dwCurrentState = pXReaderStates[i].dwEventState;
								}
							}
						}

						if (bPnpAvailable)
						{
							if (pXReaderStates[cXReaderStates - 1].dwEventState & SCARD_STATE_CHANGED)
							{
								// PNP event
								pXReaderStates[cXReaderStates - 1].dwEventState &= ~SCARD_STATE_CHANGED;
								bRebuildReadersList = TRUE;
								pXReaderStates[cXReaderStates - 1].dwCurrentState = pXReaderStates[cXReaderStates - 1].dwEventState;
								break;	// stop looping to rebuild readers list
							}
							else
								bRebuildReadersList = FALSE;
						}
					}
					else if (bPnpAvailable)
					{					
						bRebuildReadersList = FALSE;
					}								
				}
				while (!m_bStop && bPnpAvailable && ((err == SCARD_S_SUCCESS) || (err == SCARD_E_TIMEOUT)));
				
				if (m_bStop || (SCARD_E_CANCELLED == err))
					break;

				if ((err != SCARD_S_SUCCESS) && (err != SCARD_E_TIMEOUT))
				{
					m_pcsc.ReleaseContext(m_hContext); // this will force removal of all readers
				}
			}
		}
	}

	if (m_hContext)
	{
		m_pcsc.ReleaseContext(m_hContext);
		m_hContext = 0;
	}

	LeaveCriticalSection(&m_lock);

	if (pXReaderStates)
		delete[] pXReaderStates;
	if (mszXReaderNames)
		delete[] mszXReaderNames;

	m_bIsRunning = false;
}

bool PcscTracker::Start(IPcscNotifier* pNotifier)
{
	if (pNotifier && !m_bThreadStarted)
	{
		m_notifier = pNotifier;
		m_bStop = false;
		m_bIsRunning = false;
		m_bBlockingCall = false;

#ifdef _WIN32
		m_hThread = CreateThread(NULL, 0, TrackerThread, this, 0, NULL);
		m_bThreadStarted = (m_hThread != NULL);
#else
		m_bThreadStarted = (pthread_create(&m_hThread, NULL, TrackerThread, this) == 0);
#endif
		return m_bThreadStarted;
		
	}
	else
		return false;
}

void PcscTracker::Stop(void)
{
	if (m_bThreadStarted)
	{
		if (m_bIsRunning)
		{
			EnterCriticalSection(&m_lock);
			m_bStop = true;
			m_notifier = NULL;
			if (m_bBlockingCall && m_hContext)
			{
				m_pcsc.Cancel(m_hContext);
			}
			LeaveCriticalSection(&m_lock);

#ifdef _WIN32
			WaitForSingleObject(m_hThread, INFINITE);
#endif
		}
		
#ifdef _WIN32
		CloseHandle(m_hThread);
		m_hThread = NULL;
#else
		pthread_join(m_hThread, NULL);
#endif
		m_bThreadStarted = false;
	}
}

/****************************************************************/
LPCTSTR GetStateString(DWORD dwState)
{
	static TCHAR g_szVal[1024];
	g_szVal[0] = 0;

	if (dwState == SCARD_STATE_UNAWARE)
		_tcscpy(g_szVal, _T("SCARD_STATE_UNAWARE"));
	else
	{
		if (dwState & SCARD_STATE_IGNORE)
			_tcscat(g_szVal, _T("SCARD_STATE_IGNORE "));
		if (dwState & SCARD_STATE_UNKNOWN)
			_tcscat(g_szVal, _T("SCARD_STATE_UNKNOWN "));
		if (dwState & SCARD_STATE_UNAVAILABLE)
			_tcscat(g_szVal, _T("SCARD_STATE_UNAVAILABLE "));
		if (dwState & SCARD_STATE_EMPTY)
			_tcscat(g_szVal, _T("SCARD_STATE_EMPTY "));
		if (dwState & SCARD_STATE_PRESENT)
			_tcscat(g_szVal, _T("SCARD_STATE_PRESENT "));
		if (dwState & SCARD_STATE_ATRMATCH)
			_tcscat(g_szVal, _T("SCARD_STATE_ATRMATCH "));
		if (dwState & SCARD_STATE_EXCLUSIVE)
			_tcscat(g_szVal, _T("SCARD_STATE_EXCLUSIVE "));
		if (dwState & SCARD_STATE_INUSE)
			_tcscat(g_szVal, _T("SCARD_STATE_INUSE "));
		if (dwState & SCARD_STATE_MUTE)
			_tcscat(g_szVal, _T("SCARD_STATE_MUTE "));
		if (dwState & SCARD_STATE_UNPOWERED)
			_tcscat(g_szVal, _T("SCARD_STATE_UNPOWERED "));
	}

	return g_szVal;
}
