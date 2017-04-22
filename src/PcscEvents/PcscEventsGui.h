///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb  8 2017)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#ifndef __PCSCEVENTSGUI_H__
#define __PCSCEVENTSGUI_H__

#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/intl.h>
#include <wx/treectrl.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/string.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/statbox.h>
#include <wx/panel.h>
#include <wx/frame.h>

///////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
/// Class EventsFrame
///////////////////////////////////////////////////////////////////////////////
class EventsFrame : public wxFrame 
{
	private:
	
	protected:
		wxPanel* m_panel2;
		wxTreeCtrl* m_tree;
		wxPanel* m_panel1;
		wxTextCtrl* m_value;
		
		// Virtual event handlers, overide them in your derived class
		virtual void OnCloseEventsFrame( wxCloseEvent& event ) { event.Skip(); }
		virtual void HandleTreeItemCollapse( wxTreeEvent& event ) { event.Skip(); }
		virtual void HandleTreeSelChange( wxTreeEvent& event ) { event.Skip(); }
		
	
	public:
		
		EventsFrame( wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = _("PC/SC Tracker 1.1     by Mounir IDRASSI (mounir@idrix.fr)"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize( 640,480 ), long style = wxDEFAULT_FRAME_STYLE|wxTAB_TRAVERSAL );
		
		~EventsFrame();
	
};

#endif //__PCSCEVENTSGUI_H__
