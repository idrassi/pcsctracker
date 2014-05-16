///////////////////////////////////////////////////////////////////////////////
// Name:        PcscEvents.cpp
// Purpose:     simple test program for the PcscTracker class
// Author:      Mounir IDRASSI
// Created:     2014-05-14
// Copyright:   (c) 2014 Mounir IDRASSI <mounir.idrassi@idrix.fr>
// Licence:     GPLv3/MIT
///////////////////////////////////////////////////////////////////////////////

#include "PcscTracker.h"

class SimplePcscNotifier : public IPcscNotifier
{
public:
	virtual void readerRemoved(LPCTSTR szReaderName)
	{
		_tprintf(_T("Reader \"%s\" removed.\n\n"), szReaderName);
	}

	virtual void readerAdded(SCARD_READERSTATE* pState)
	{
		_tprintf(_T("Reader \"%s\" : \n\tState = %s\n\n"), pState->szReader, GetStateString(pState->dwEventState));
	}

	virtual void stateChanged(SCARD_READERSTATE* pState)
	{
		_tprintf(_T("Reader \"%s\" : \n\tState = %s\n\n"), pState->szReader, GetStateString(pState->dwEventState));
	}
};

int main(int argc, char** argv)
{
	SimplePcscNotifier notifier;
	PcscTracker tracker;

	if (tracker.Start(&notifier))
	{
		printf("PC/SC tracker started. Press ENTER to quit\n");
		getchar();
		tracker.Stop();
		printf("PC/SC tracker stopped!\n");
	}
	else
		printf("PC/SC tracker failed to start!!\n");


	return 0;
}
