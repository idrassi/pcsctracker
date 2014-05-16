///////////////////////////////////////////////////////////////////////////////
// Name:        main.cpp
// Purpose:     main code for pcscTracker application
// Author:      Mounir IDRASSI
// Created:     2014-05-14
// Copyright:   (c) 2014 Mounir IDRASSI <mounir.idrassi@idrix.fr>
// Licence:     GPLv3/MIT
///////////////////////////////////////////////////////////////////////////////

#include <wx/wx.h>
#include <wx/config.h>
#include <wx/log.h>
#include <wx/stdpaths.h>
#include <wx/strconv.h>
#include <wx/event.h>
#include <wx/imaglist.h>
#include <wx/regex.h>

#include <string.h>
#include <list>
#include <vector>
#include <string>
#include "PcscEventsGui.h"
#include "PcscTracker.h"
#include "smartcard_list.h"

#ifndef _WIN32
#include <sys/time.h>
#include "res/pcsceventsicon.xpm"
#include "res/slotsImgList.xpm"
#endif

// ICON IDs
#define ID_SLOTS_ROOT					0
#define	ID_SLOTS_SLOT_EMPTY				1
#define ID_SLOTS_SLOT_STATE_UNKNOWN		2
#define ID_SLOTS_SLOT_FULL				3
#define ID_SLOTS_SLOT_FULL_UNKNWON		4
#define ID_SLOTS_SLOT_FULL_ERROR		5
#define ID_SLOTS_SLOT_PROPERTY			6
#define	ID_SLOTS_TOKEN					7
#define	ID_SLOTS_TOKEN_ERROR			8
#define ID_SLOTS_TOKEN_UNKNOWN			9
#define ID_SLOTS_TOKEN_PROPERTY			ID_SLOTS_SLOT_PROPERTY
#define ID_SLOTS_MECHANISMS_FOLDER		10
#define ID_SLOTS_MECHANISM				11
#define	ID_SLOTS_MECHANISM_PROPERTY		12


DEFINE_EVENT_TYPE(wxEVT_READER_ADDED_EVENT)
DEFINE_EVENT_TYPE(wxEVT_READER_REMOVED_EVENT)
DEFINE_EVENT_TYPE(wxEVT_CARD_STATE_EVENT)


const wxString g_szRootFormat(_("%d Readers"));

class CAutoLock
{
public:
	CRITICAL_SECTION& m_cs;
	CAutoLock(CRITICAL_SECTION& cs) : m_cs(cs) { EnterCriticalSection(&m_cs); }
	~CAutoLock() { LeaveCriticalSection(&m_cs); }
};


class wxReaderStateClientData : public wxClientData
{
public:
	wxReaderStateClientData(SCARD_READERSTATE* pState) :
#ifdef _WIN32
		m_readerName(pState->szReader),  
#else
		m_readerName(pState->szReader, wxConvLibc),  
#endif
		m_currentState(pState->dwCurrentState),
		m_eventState(pState->dwEventState),
		m_cbAtr(pState->cbAtr)
	{ 
		memcpy(m_pbAtr, pState->rgbAtr, m_cbAtr);
		memset(&m_pbAtr[m_cbAtr], 0, 36 - m_cbAtr);
	}

	const wxString& GetReaderName(void) const { return m_readerName; }
	DWORD GetCurrentState(void) const { return m_currentState; }
	DWORD GetEventState(void) const { return m_eventState; }
	DWORD GetAtrLength(void) const { return m_cbAtr; }
	const BYTE* GetAtr(void) const { return m_pbAtr; }

private:
    wxString  m_readerName;
	DWORD	  m_currentState;
	DWORD	  m_eventState;
	BYTE	  m_pbAtr[36];
	DWORD	  m_cbAtr;
};

class wxReaderItemData : public wxTreeItemData
{
public:
	wxReaderItemData(const wxString& text) : wxTreeItemData(), m_szText(text)
	{
	}

	const wxString& GetText() const { return m_szText; }

protected:
	wxString m_szText;
};

wxChar ToHex(BYTE b)
{
	if (b <= 9)
		return wxT('0') + b;
	else if (b <= 15)
		return wxT('A') + b - 10;
	else
		return wxT('.');
}

wxString ToHex(const BYTE* pbData, DWORD dwLen)
{
	wxString szHex;
	if (pbData && dwLen)
	{
		wxStringBuffer hexModifier(szHex, 3*dwLen + 1);
		wxChar* pStr = (wxChar*) hexModifier;

		for (DWORD i = 0; i < dwLen; i++)
		{
			BYTE b = *pbData++;
			*pStr++ = ToHex(b >> 4); 
			*pStr++ = ToHex(b & 0x0F);
			*pStr++ = wxT(' ');
		}
		pStr--; // remove the last space
		*pStr = 0;
	}
	return szHex;
}

wxString PcscStateToString(DWORD dwState)
{
	wxString szResult;
	if (dwState == SCARD_STATE_UNAWARE)
		szResult = wxT("SCARD_STATE_UNAWARE");
	else
	{
		dwState &= 0x0000FFFF;

		if (dwState & SCARD_STATE_IGNORE)
		{
			szResult += wxT("SCARD_STATE_IGNORE, ");
			dwState ^= SCARD_STATE_IGNORE;
		}
		if (dwState & SCARD_STATE_UNKNOWN)
		{
			szResult += wxT("SCARD_STATE_UNKNOWN, ");
			dwState ^= SCARD_STATE_UNKNOWN;
		}
		if (dwState & SCARD_STATE_UNAVAILABLE)
		{
			szResult += wxT("SCARD_STATE_UNAVAILABLE, ");
			dwState ^= SCARD_STATE_UNAVAILABLE;
		}
		if (dwState & SCARD_STATE_EMPTY)
		{
			szResult += wxT("SCARD_STATE_EMPTY, ");
			dwState ^= SCARD_STATE_EMPTY;
		}

		if (dwState & SCARD_STATE_PRESENT)
		{
			szResult += wxT("SCARD_STATE_PRESENT, ");
			dwState ^= SCARD_STATE_PRESENT;
		}
		if (dwState & SCARD_STATE_EXCLUSIVE)
		{
			szResult += wxT("SCARD_STATE_EXCLUSIVE, ");
			dwState ^= SCARD_STATE_EXCLUSIVE;
		}
		if (dwState & SCARD_STATE_INUSE)
		{
			szResult += wxT("SCARD_STATE_INUSE, ");
			dwState ^= SCARD_STATE_INUSE;
		}
		if (dwState & SCARD_STATE_MUTE)
		{
			szResult += wxT("SCARD_STATE_MUTE, ");
			dwState ^= SCARD_STATE_MUTE;
		}
		if (dwState & SCARD_STATE_UNPOWERED)
		{
			szResult += wxT("SCARD_STATE_UNPOWERED, ");
			dwState ^= SCARD_STATE_UNPOWERED;
		}

		if (szResult.length() > 2)
			szResult.RemoveLast(2);
	}

	return szResult;
}


class MyEventsFrame : public EventsFrame, public IPcscNotifier
{
protected:
	PcscTracker& m_tracker;
	wxTreeItemId m_readersRoot;
	CRITICAL_SECTION m_lock;

	virtual void OnCloseEventsFrame( wxCloseEvent& event )
	{ 
		m_tracker.Stop();
		Destroy();
	}

	virtual void HandleTreeItemCollapse( wxTreeEvent& event )
   {
      if (event.GetItem() == m_readersRoot)
         event.Veto();
      else
         event.Skip();
   }
	
   virtual void HandleTreeSelChange( wxTreeEvent& event )
   {
      wxTreeItemId item = event.GetItem();
      wxTreeItemData* data =  m_tree->GetItemData(item);
      if (data)
      {
         m_value->SetValue(((wxReaderItemData*) data)->GetText());
      }
      else
         m_value->SetValue(wxT(""));
   }

public:
	MyEventsFrame(PcscTracker& tracker) : EventsFrame(NULL), m_tracker(tracker)
	{
		InitializeCriticalSection(&m_lock);
		//slots image list
		wxImageList* imgList = new wxImageList(16,16);
		imgList->Add(wxBITMAP(slotsImgList));
		m_tree->AssignImageList(imgList);

		m_readersRoot = m_tree->AddRoot(wxString::Format(g_szRootFormat, 0), ID_SLOTS_ROOT);

		Connect(wxEVT_READER_ADDED_EVENT, wxCommandEventHandler( MyEventsFrame::OnReaderAdded ), NULL, this );
		Connect(wxEVT_READER_REMOVED_EVENT, wxCommandEventHandler( MyEventsFrame::OnReaderRemoved ), NULL, this );
		Connect(wxEVT_CARD_STATE_EVENT, wxCommandEventHandler( MyEventsFrame::OnCardStateChanged ), NULL, this );
	}

	~MyEventsFrame()
	{
		Disconnect( wxEVT_READER_REMOVED_EVENT, wxCommandEventHandler( MyEventsFrame::OnReaderRemoved ) );
		Disconnect(wxEVT_READER_ADDED_EVENT, wxCommandEventHandler( MyEventsFrame::OnReaderAdded ) );
		Disconnect(wxEVT_CARD_STATE_EVENT, wxCommandEventHandler( MyEventsFrame::OnCardStateChanged ));

		DeleteCriticalSection(&m_lock);
	}

   
#define SCARD_PROVIDER_CARD_MODULE 0x80000001

   void GetCardInformation(wxReaderStateClientData* pState, wxString& szCardName, wxString& szCardProvider, wxString& szCardModule)
   {
      szCardName = wxT("");
      szCardProvider = wxT("");
      szCardModule = wxT("");

      if (pState->GetAtrLength())
      {
#ifdef _WIN32
         LPWSTR mszCards = NULL;
         DWORD dwLen = SCARD_AUTOALLOCATE;
         SCARDCONTEXT hContext;
         LONG lRet = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &hContext);
         if (lRet == SCARD_S_SUCCESS)
         {
            lRet = SCardListCards(hContext, pState->GetAtr(), NULL, 0, (LPWSTR) &mszCards, &dwLen);
            if ((lRet == SCARD_S_SUCCESS) && mszCards)
            {
               szCardName = mszCards;

               LPWSTR szProvider = NULL;
               DWORD  chProvider = SCARD_AUTOALLOCATE;
         
               lRet = SCardGetCardTypeProviderName(hContext,
                                                   mszCards,
                                                   SCARD_PROVIDER_CSP,
                                                   (LPTSTR)&szProvider,
                                                   &chProvider);
               if ((SCARD_S_SUCCESS == lRet) && szProvider)
               {
                  szCardProvider = szProvider;

                  if (wcscmp(szProvider, MS_SCARD_PROV_W) == 0)
                  {
                     // Look for the minidriver Dll
                     LPWSTR szModule = NULL;
                     DWORD  chModule = SCARD_AUTOALLOCATE;
         
                     lRet = SCardGetCardTypeProviderName(hContext,
                                                         mszCards,
                                                         SCARD_PROVIDER_CARD_MODULE,
                                                         (LPTSTR)&szModule,
                                                         &chModule);
                     if ((SCARD_S_SUCCESS == lRet) && szModule)
                     {
                        szCardModule = szModule;
                        SCardFreeMemory(hContext, szModule);
                     }
                  }

                  SCardFreeMemory(hContext, szProvider);
               }

               SCardFreeMemory(hContext, mszCards);
            }
            SCardReleaseContext(hContext);
         }
#else
         wxString szATR = ToHex(pState->GetAtr(), pState->GetAtrLength());
         wxRegEx cardNameRegEx;
         size_t i, cardsCount = sizeof(g_cardsList) / sizeof(CardEntry);
         for (i = 0; i < cardsCount; i++)
         {
            wxString exp(g_cardsList[i].atr, wxConvLibc);
            if (  cardNameRegEx.Compile(exp, wxRE_BASIC)
               && cardNameRegEx.Matches(szATR)
               )
            {
               szCardName = wxString(g_cardsList[i].cardName, wxConvUTF8);
               break;
            }
         }
#endif
      }
   }

	void UpdateItem(wxTreeItemId item, wxReaderStateClientData* pState)
	{				
		if (pState->GetEventState() & SCARD_STATE_PRESENT)
		{
			if ( !(pState->GetCurrentState() & SCARD_STATE_PRESENT))
			{
				wxString szAtrValue = ToHex(pState->GetAtr(), pState->GetAtrLength());
				wxString szAtrLabel = wxT("ATR : ") + szAtrValue;
				wxTreeItemId propItem = m_tree->AppendItem(item, szAtrLabel, ID_SLOTS_TOKEN_PROPERTY);
				m_tree->SetItemData(propItem, new wxReaderItemData(szAtrValue));

				wxString szStateValue = PcscStateToString(pState->GetEventState());
				wxString szStateLabel = wxT("State : ") + szStateValue;
				propItem = m_tree->AppendItem(item, szStateLabel, ID_SLOTS_TOKEN_PROPERTY);
				m_tree->SetItemData(propItem, new wxReaderItemData(szStateValue));

            // Add card name and associated provider
            wxString szCardName, szCardProvider, szCardModule;
            GetCardInformation(pState, szCardName, szCardProvider, szCardModule);

            wxString szCardNameLabel = wxT("Card Name : ") + szCardName;
            propItem = m_tree->AppendItem(item, szCardNameLabel, szCardName.Length()? ID_SLOTS_TOKEN : ID_SLOTS_TOKEN_UNKNOWN);
				m_tree->SetItemData(propItem, new wxReaderItemData(szCardName));

            if (szCardProvider.Length())
            {
               wxString szCardProviderLabel = wxT("Card Provider : ") + szCardProvider;
               propItem = m_tree->AppendItem(item, szCardProviderLabel, ID_SLOTS_MECHANISMS_FOLDER);
				   m_tree->SetItemData(propItem, new wxReaderItemData(szCardProvider));

               if (szCardModule.Length())
               {
                  // minidriver case
                  wxString szCardModuleLabel = wxT("Minidriver : ") + szCardModule;
                  wxTreeItemId minidriverItem = m_tree->AppendItem(propItem, szCardModuleLabel, ID_SLOTS_MECHANISM);
				      m_tree->SetItemData(minidriverItem, new wxReaderItemData(szCardModule));
                  m_tree->Expand(propItem);
               }
            }
			}
			else
			{
				wxTreeItemIdValue cookie;
				wxTreeItemId propItem = m_tree->GetFirstChild(item, cookie);
				if (propItem.IsOk())
				{
					// ATR
               bool bAtrChanged = false;
					wxString szAtrValue = ToHex(pState->GetAtr(), pState->GetAtrLength());
					wxString szAtrLabel = wxT("ATR : ") + szAtrValue;
               bAtrChanged = (m_tree->GetItemText(propItem) != szAtrLabel);
					m_tree->SetItemText(propItem, szAtrLabel);
					delete m_tree->GetItemData(propItem);
					m_tree->SetItemData(propItem, new wxReaderItemData(szAtrValue));

					propItem = m_tree->GetNextChild(propItem, cookie);
					// State
					wxString szStateValue = PcscStateToString(pState->GetEventState());
					wxString szStateLabel = wxT("State : ") + szStateValue;
					m_tree->SetItemText(propItem, szStateLabel);
					delete m_tree->GetItemData(propItem);
					m_tree->SetItemData(propItem, new wxReaderItemData(szStateValue));

               if (bAtrChanged)
               {
                  // Update card name and associated provider
                  wxString szCardName, szCardProvider, szCardModule;
                  GetCardInformation(pState, szCardName, szCardProvider, szCardModule);

                  propItem = m_tree->GetNextChild(propItem, cookie);

                  wxString szCardNameLabel = wxT("Card Name : ") + szCardName;
				      m_tree->SetItemText(propItem, szCardNameLabel);
                  m_tree->SetItemImage(propItem, szCardName.Length()? ID_SLOTS_TOKEN : ID_SLOTS_TOKEN_UNKNOWN);
                  delete m_tree->GetItemData(propItem);
				      m_tree->SetItemData(propItem, new wxReaderItemData(szCardName));

                  propItem = m_tree->GetNextChild(propItem, cookie);

                  if (szCardProvider.Length())
                  {
                     wxString szCardProviderLabel = wxT("Card Provider : ") + szCardProvider;
                     if (propItem.IsOk())
                     {
				            m_tree->SetItemText(propItem, szCardProviderLabel);
                        delete m_tree->GetItemData(propItem);
				            m_tree->SetItemData(propItem, new wxReaderItemData(szCardProvider));
                     }
                     else
                     {
                        propItem = m_tree->AppendItem(item, szCardProviderLabel, ID_SLOTS_MECHANISMS_FOLDER);
				            m_tree->SetItemData(propItem, new wxReaderItemData(szCardProvider));
                     }

                     if (szCardModule.Length())
                     {
                        // minidriver case
                        wxString szCardModuleLabel = wxT("Minidriver : ") + szCardModule;
                        wxTreeItemId minidriverItem = m_tree->GetFirstChild(propItem, cookie);
                        if (minidriverItem.IsOk())
                        {
                           m_tree->SetItemText(minidriverItem, szCardModuleLabel);
                           delete m_tree->GetItemData(minidriverItem);
                        }
                        else
                           minidriverItem = m_tree->AppendItem(propItem, szCardModuleLabel, ID_SLOTS_MECHANISM);

				            m_tree->SetItemData(minidriverItem, new wxReaderItemData(szCardModule));
                        m_tree->Expand(propItem);
                     }
                  }
                  else
                  {
                     if (propItem.IsOk())
                        m_tree->Delete(propItem);
                  }
				   }
            }
			}
		}
		
		if (!(pState->GetEventState() & SCARD_STATE_PRESENT) && (pState->GetCurrentState() & SCARD_STATE_PRESENT))
		{
			m_tree->DeleteChildren(item);
		}
	}

	void UpdateRootLabel()
	{
		m_tree->SetItemText(m_readersRoot, wxString::Format(g_szRootFormat, m_tree->GetChildrenCount(m_readersRoot, false)));
	}

	virtual void OnReaderAdded( wxCommandEvent& event )
	{
		CAutoLock lock(m_lock);
		wxReaderStateClientData* pState = (wxReaderStateClientData*) event.GetClientObject();
		const wxString& szName = pState->GetReaderName();
		DWORD dwEventState = pState->GetEventState();
		int imageIndex = -1;

		if (dwEventState & SCARD_STATE_PRESENT)
		{
			if (dwEventState & SCARD_STATE_MUTE)
				imageIndex = ID_SLOTS_SLOT_FULL_UNKNWON;
			else
				imageIndex = ID_SLOTS_SLOT_FULL;
		}
		else if (dwEventState & SCARD_STATE_EMPTY)
			imageIndex = ID_SLOTS_SLOT_EMPTY;
		else
			imageIndex = ID_SLOTS_SLOT_STATE_UNKNOWN;

		wxTreeItemId item = m_tree->AppendItem(m_readersRoot, szName, imageIndex);
		m_tree->SetItemData(item, new wxReaderItemData(szName));
		UpdateItem(item, pState);

		UpdateRootLabel();
		m_tree->Expand(m_readersRoot);

		delete pState;
	}

	virtual void OnReaderRemoved( wxCommandEvent& event )
	{
		CAutoLock lock(m_lock);
		wxString szName = event.GetString();

		wxTreeItemIdValue cookie;
		wxTreeItemId item = m_tree->GetFirstChild(m_readersRoot, cookie);
		while (item.IsOk())
		{
			if (szName == m_tree->GetItemText(item))
			{
				m_tree->Delete(item);
				UpdateRootLabel();
				break;
			}
			item = m_tree->GetNextChild(m_readersRoot, cookie);
		}
	}

	virtual void OnCardStateChanged( wxCommandEvent& event )
	{
		CAutoLock lock(m_lock);
		wxReaderStateClientData* pState = (wxReaderStateClientData*) event.GetClientObject();
		const wxString& szName = pState->GetReaderName();
		DWORD dwEventState = pState->GetEventState();
		int imageIndex = -1;

		if (dwEventState & SCARD_STATE_PRESENT)
		{
			if (dwEventState & SCARD_STATE_MUTE)
				imageIndex = ID_SLOTS_SLOT_FULL_UNKNWON;
			else
				imageIndex = ID_SLOTS_SLOT_FULL;
		}
		else if (dwEventState & SCARD_STATE_EMPTY)
			imageIndex = ID_SLOTS_SLOT_EMPTY;
		else
			imageIndex = ID_SLOTS_SLOT_STATE_UNKNOWN;

		wxTreeItemIdValue cookie;
		wxTreeItemId item = m_tree->GetFirstChild(m_readersRoot, cookie);
		while (item.IsOk())
		{
			if (szName == m_tree->GetItemText(item))
			{
				m_tree->SetItemImage(item, imageIndex);
				UpdateItem(item, pState);
				break;
			}
			item = m_tree->GetNextChild(m_readersRoot, cookie);
		}


		delete pState;
	}

	virtual void readerRemoved(LPCTSTR szReaderName)
	{
		wxCommandEvent myEvent( wxEVT_READER_REMOVED_EVENT );
#ifdef _WIN32
		myEvent.SetString(szReaderName);
#else
		wxString name(szReaderName, wxConvLibc);
		myEvent.SetString(name);
#endif
		// Send it
		wxPostEvent(this,myEvent);
	}
	virtual void readerAdded(SCARD_READERSTATE* pState)
	{
		wxCommandEvent myEvent( wxEVT_READER_ADDED_EVENT );
		myEvent.SetClientObject(new wxReaderStateClientData(pState));
		// Send it
		wxPostEvent(this,myEvent);
	}

	virtual void stateChanged(SCARD_READERSTATE* pState)
	{
		wxCommandEvent myEvent( wxEVT_CARD_STATE_EVENT );
		myEvent.SetClientObject(new wxReaderStateClientData(pState));
		// Send it
		wxPostEvent(this,myEvent);
	}

};

class MyApp: public wxApp {
public:
    bool OnInit();
	int OnExit();
	int InitLanguages();
	wxConfig*				m_config;
	wxLocale*				m_locale; // locale we'll be using
	PcscTracker m_tracker;

};

IMPLEMENT_APP(MyApp)

int MyApp::InitLanguages()
{
	if (m_locale) delete m_locale;
	m_locale = new wxLocale;

	int lng = wxLocale::GetSystemLanguage();
	
	if ( !m_locale->Init(lng, 0) )
	{		
		if(m_locale->Init(wxLANGUAGE_ENGLISH_US, 0) )
		{
			lng = wxLANGUAGE_ENGLISH_US;
		}
	}

#ifndef __APPLE__   
    wxLocale::AddCatalogLookupPathPrefix(wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFileName::GetPathSeparator() +  wxT("Languages"));
#else    
    wxLocale::AddCatalogLookupPathPrefix(wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFileName::GetPathSeparator() +  wxT("../Resources/Languages"));
#endif   
    m_locale->AddCatalog(wxT("pcscevents"));

	return lng;
}

bool MyApp::OnInit()
{
	wxApp::OnInit();
	SetAppName(_("PCSCEVENTS"));
	

#ifndef __APPLE__ 	
	wxSetWorkingDirectory(wxPathOnly(wxStandardPaths::Get().GetExecutablePath()));
#else
	wxSetWorkingDirectory(wxPathOnly(wxStandardPaths::Get().GetExecutablePath()) + wxFileName::GetPathSeparator() + wxT("../Resources"));
#endif
	
	wxLog::EnableLogging(false);
	wxLog::SetComponentLevel(wxT("wx"), wxLOG_Error);

	m_locale = NULL;
	InitLanguages();

    wxInitAllImageHandlers();

	if (m_tracker.Initialize())
	{
		MyEventsFrame* frame_1 = new MyEventsFrame(m_tracker);
		wxIcon _icon;
		_icon.CopyFromBitmap(wxBitmap(wxICON(pcsceventsicon)));
		frame_1->SetIcon(_icon);
		SetTopWindow(frame_1);

		frame_1->Show();

		m_tracker.Start(frame_1);
		return true;
	}
	else
	{
#ifdef _WIN32
		wxMessageBox(_("Failed to load PC/SC functions from Winscard.\nPlease check your machine configuration"), _("Unexpected Error"), wxOK | wxICON_ERROR);
#else
		wxMessageBox(_("PC/SC Lite doesn't seem to be installed on your machine.\nPlease install first (e.g. \"sudo apt-get install pcscd\" under debian/Ubuntu or \"sudo yum install pcsc-lite\" under RedHat/CentOS"),
			_("PC/SC Lite missing"), wxOK | wxICON_ERROR);

#endif
		return false;
	}
}

int MyApp::OnExit()
{
	int status = wxApp::OnExit();
	if (m_locale) delete m_locale;

	return status;
}
