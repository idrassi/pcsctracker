///////////////////////////////////////////////////////////////////////////
// C++ code generated with wxFormBuilder (version Feb 26 2014)
// http://www.wxformbuilder.org/
//
// PLEASE DO "NOT" EDIT THIS FILE!
///////////////////////////////////////////////////////////////////////////

#include "PcscEventsGui.h"

///////////////////////////////////////////////////////////////////////////

EventsFrame::EventsFrame( wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style ) : wxFrame( parent, id, title, pos, size, style )
{
	this->SetSizeHints( wxSize( 640,480 ), wxDefaultSize );
	
	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer( wxVERTICAL );
	
	m_panel2 = new wxPanel( this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer( wxVERTICAL );
	
	m_tree = new wxTreeCtrl( m_panel2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS );
	bSizer2->Add( m_tree, 1, wxALL|wxEXPAND, 5 );
	
	m_panel1 = new wxPanel( m_panel2, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTAB_TRAVERSAL );
	m_panel1->SetMinSize( wxSize( 630,100 ) );
	
	wxStaticBoxSizer* sbSizer1;
	sbSizer1 = new wxStaticBoxSizer( new wxStaticBox( m_panel1, wxID_ANY, _("Display : ") ), wxVERTICAL );
	
	m_value = new wxTextCtrl( m_panel1, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE|wxTE_READONLY );
	sbSizer1->Add( m_value, 1, wxEXPAND, 5 );
	
	
	m_panel1->SetSizer( sbSizer1 );
	m_panel1->Layout();
	sbSizer1->Fit( m_panel1 );
	bSizer2->Add( m_panel1, 0, wxALL|wxEXPAND, 5 );
	
	
	m_panel2->SetSizer( bSizer2 );
	m_panel2->Layout();
	bSizer2->Fit( m_panel2 );
	bSizer4->Add( m_panel2, 1, wxEXPAND, 5 );
	
	
	this->SetSizer( bSizer4 );
	this->Layout();
	
	this->Centre( wxBOTH );
	
	// Connect Events
	this->Connect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( EventsFrame::OnCloseEventsFrame ) );
	m_tree->Connect( wxEVT_COMMAND_TREE_ITEM_COLLAPSING, wxTreeEventHandler( EventsFrame::HandleTreeItemCollapse ), NULL, this );
	m_tree->Connect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( EventsFrame::HandleTreeSelChange ), NULL, this );
}

EventsFrame::~EventsFrame()
{
	// Disconnect Events
	this->Disconnect( wxEVT_CLOSE_WINDOW, wxCloseEventHandler( EventsFrame::OnCloseEventsFrame ) );
	m_tree->Disconnect( wxEVT_COMMAND_TREE_ITEM_COLLAPSING, wxTreeEventHandler( EventsFrame::HandleTreeItemCollapse ), NULL, this );
	m_tree->Disconnect( wxEVT_COMMAND_TREE_SEL_CHANGED, wxTreeEventHandler( EventsFrame::HandleTreeSelChange ), NULL, this );
	
}
