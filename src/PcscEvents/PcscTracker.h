///////////////////////////////////////////////////////////////////////////////
// Name:        PcscTracker.h
// Purpose:     declaration of PC/SC helper functions and classes
// Author:      Mounir IDRASSI
// Created:     2014-05-14
// Copyright:   (c) 2014 Mounir IDRASSI <mounir.idrassi@idrix.fr>
// Licence:     GPLv3/MIT
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#ifdef _WIN32
#include <Windows.h>
#include <WinSCard.h>
#include <tchar.h>
#else
#if defined(__APPLE__)
#define LPTSTR __dummtTSTR
#define LPCTSTR __dummtCTSTR
#define LPCWSTR __dummtCWSTR
#include <PCSC/wintypes.h>
#include <PCSC/winscard.h>
#undef LPTSTR
#undef LPCTSTR
#undef LPCWSTR
#else
#include <winscard.h>
#include <reader.h>
#endif

#include <pthread.h>
#include <dlfcn.h>

#define WINAPI
#define VOID	void
typedef void* 	HMODULE;

VOID Sleep(DWORD dwMilliseconds);

HMODULE LoadLibrary(const char* lpFileName);
BOOL FreeLibrary(HMODULE hModule);
void* GetProcAddress(HMODULE hModule, const char* lpProcName);

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef SCARD_E_NO_READERS_AVAILABLE
#define SCARD_E_NO_READERS_AVAILABLE	((LONG) 0x8010002E)
#endif


/*
 * string manipulation
 */
#define TCHAR char
#define PTCHAR char*

#ifdef __APPLE__
#define LPTSTR	TCHAR*
#define LPCTSTR const TCHAR*
#endif

#define  TEXT(x)		x
#ifndef  _T
#define  _T(x)			x
#endif
#define  _tcscpy	    strcpy	 
#define  _tcslen	    strlen
#define  _tcscat	    strcat
#define _tcscmp		 strcmp
#define _tprintf		 printf
/*
 * Critical sections
 */
typedef pthread_mutex_t CRITICAL_SECTION;
typedef CRITICAL_SECTION* LPCRITICAL_SECTION;

void InitializeCriticalSection(
  LPCRITICAL_SECTION lpCriticalSection
);

void DeleteCriticalSection(
  LPCRITICAL_SECTION lpCriticalSection
);

void EnterCriticalSection(
  LPCRITICAL_SECTION lpCriticalSection
);


void LeaveCriticalSection(
  LPCRITICAL_SECTION lpCriticalSection
);

#endif

typedef LONG (WINAPI *SCardEstablishContextPtr)( DWORD dwScope, LPCVOID pvReserved1, LPCVOID pvReserved2, LPSCARDCONTEXT phContext);
typedef LONG (WINAPI *SCardReleaseContextPtr)(SCARDCONTEXT hContext);
typedef LONG (WINAPI *SCardIsValidContextPtr)(SCARDCONTEXT hContext);

typedef LONG (WINAPI *SCardListReadersPtr)(SCARDCONTEXT hContext, LPCTSTR mszGroups, LPTSTR mszReaders, LPDWORD pcchReaders);
typedef LONG (WINAPI *SCardGetStatusChangePtr)(SCARDCONTEXT hContext, DWORD dwTimeout, SCARD_READERSTATE* rgReaderStates, DWORD cReaders);
typedef LONG (WINAPI *SCardCancelPtr)(SCARDCONTEXT hContext);


class Pcsc
{
protected:
	HMODULE m_hDll;
	SCardEstablishContextPtr m_SCardEstablishContext;
	SCardReleaseContextPtr m_SCardReleaseContext;
	SCardIsValidContextPtr m_SCardIsValidContext;
	SCardListReadersPtr m_SCardListReaders;
	SCardGetStatusChangePtr m_SCardGetStatusChange;
	SCardCancelPtr m_SCardCancel;

public:
	Pcsc() : m_hDll(NULL)
	{
		// Initialize();
	}

	~Pcsc()
	{
		Finalize();
	}

	bool Initialize(void);
	void Finalize(void);

	LONG EstablishContext(DWORD dwScope, LPCVOID pvReserved1, LPCVOID pvReserved2, LPSCARDCONTEXT phContext);
	LONG ReleaseContext(SCARDCONTEXT hContext);
	LONG IsValidContext(SCARDCONTEXT hContext);
	LONG ListReaders(SCARDCONTEXT hContext, LPCTSTR mszGroups, LPTSTR mszReaders, LPDWORD pcchReaders);
	LONG GetStatusChange(SCARDCONTEXT hContext, DWORD dwTimeout, SCARD_READERSTATE* rgReaderStates, DWORD cReaders);
	LONG Cancel(SCARDCONTEXT hContext);
};

class IPcscNotifier
{
public:
	virtual void readerRemoved(LPCTSTR szReaderName) = 0;
	virtual void readerAdded(SCARD_READERSTATE* pState) = 0;
	virtual void stateChanged(SCARD_READERSTATE* pState) = 0;
};


class PcscTracker
{
protected:
	IPcscNotifier* m_notifier;
	volatile bool m_bStop;
	volatile bool m_bIsRunning;
	volatile bool m_bBlockingCall;
#ifdef _WIN32
	HANDLE m_hThread;
#else
	pthread_t m_hThread;
#endif
	bool m_bThreadStarted;
	CRITICAL_SECTION m_lock;
	SCARDCONTEXT m_hContext;
	Pcsc m_pcsc;

#ifdef _WIN32
	static DWORD WINAPI TrackerThread(LPVOID lpParameter)
#else
	static void* TrackerThread(LPVOID lpParameter)
#endif
	{
		PcscTracker* pTracker = (PcscTracker*) lpParameter;

		pTracker->TrackerProc();
		return 0;
	}

	void TrackerProc(void);

	void notifyReaderAdded(SCARD_READERSTATE* pState)
	{
		if (m_notifier)
			m_notifier->readerAdded(pState);
	}

	void notifyReaderRemoved(LPCTSTR szReaderName)
	{
		if (m_notifier)
			m_notifier->readerRemoved(szReaderName);
	}

	void notifyStateChange(SCARD_READERSTATE* pState)
	{
		if (m_notifier)
			m_notifier->stateChanged (pState);
	}
	

public:
	static DWORD g_dwPollingPeriod;
	static DWORD g_pcscScope;

	PcscTracker() : m_notifier(NULL), m_bStop(false), m_bIsRunning(false), m_bBlockingCall(false), m_hContext(0)
	{
#ifdef _WIN32
		m_hThread = NULL;
#endif
		m_bThreadStarted = false;

		InitializeCriticalSection(&m_lock);
	}

	~PcscTracker()
	{
		Stop();
	}

	bool Initialize(void) { return m_pcsc.Initialize(); }

	bool Start(IPcscNotifier* pNotifier);
	void Stop(void);
};


LPCTSTR GetStateString(DWORD dwState);

