///////////////////////////////////////////////////////////////////////////////
// Name:        src/common/rearrangectrl.cpp
// Purpose:     implementation of classes in wx/rearrangectrl.h
// Author:      Vadim Zeitlin
// Created:     2008-12-15
// Copyright:   (c) 2008 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "wx/wxprec.h"

#ifdef __BORLANDC__
    #pragma hdrstop
#endif

#if wxUSE_REARRANGECTRL

#ifndef WX_PRECOMP
    #include "wx/button.h"
    #include "wx/stattext.h"
    #include "wx/sizer.h"
#endif // WX_PRECOMP

#include "wx/rearrangectrl.h"

// ============================================================================
// wxRearrangeList implementation
// ============================================================================

extern
WXDLLIMPEXP_DATA_CORE(const char) wxRearrangeListNameStr[] = "wxRearrangeList";

/*

BEGIN_EVENT_TABLE(wxRearrangeList, wxCheckListBox)
    EVT_CHECKLISTBOX(wxID_ANY, wxRearrangeList::OnCheck)
END_EVENT_TABLE()

//*/

wxArrayString wxRearrangeListBase::orderItems(const wxArrayString& items, const wxArrayInt& order) const
{
    const size_t count = items.size();
    wxArrayString itemsInOrder;
    itemsInOrder.reserve(count);
    for (size_t n = 0; n < count; n++ )
    {
        int idx = order[n];
        if ( idx < 0 )
            idx = -idx - 1;
        itemsInOrder.push_back(items[idx]);
    }
    
    return itemsInOrder;
}

void wxRearrangeListBase::checkOrder(const wxArrayInt& order)
{
    const size_t count = order.size();
    // and now check all the items which should be initially checked
    for (size_t n = 0; n < count; n++ )
    {
        if ( order[n] >= 0 )
        {
            // Be careful to call the base class version here and not our own
            // which would also update m_order itself.
            parentCheck(n);
        }
    }
}

/*
bool wxRearrangeList::Create(wxWindow *parent,
                             wxWindowID id,
                             const wxPoint& pos,
                             const wxSize& size,
                             const wxArrayInt& order,
                             const wxArrayString& items,
                             long style,
                             const wxValidator& validator,
                             const wxString& name)
{
    // construct the array of items in the order in which they should appear in
    // the control
    const size_t count = items.size();
    wxCHECK_MSG( order.size() == count, false, "arrays not in sync" );

    // do create the real control
    if ( !wxCheckListBox::Create(parent, id, pos, size, orderItems(items, order),
                                 style, validator, name) )
        return false;
    
    checkOrder(order);
    
    m_order = order;

    return true;
}
//*/

bool wxRearrangeListBase::CanMoveCurrentUp() const
{
    const int sel = GetRearrangeListContainer()->GetSelection();
    return sel != wxNOT_FOUND && sel != 0;
}

bool wxRearrangeListBase::CanMoveCurrentDown() const
{
    const wxItemContainer *con = GetRearrangeListContainer();
    const int sel = con->GetSelection();
    return sel != wxNOT_FOUND && static_cast<unsigned>(sel) != con->GetCount() - 1;
}

bool wxRearrangeListBase::MoveCurrentUp()
{
    if ( !CanMoveCurrentUp() )
        return false;

    wxItemContainer *con = GetRearrangeListContainer();
    const int sel = con->GetSelection();

    Swap(sel, sel - 1);
    con->SetSelection(sel - 1);

    return true;
}

bool wxRearrangeListBase::MoveCurrentDown()
{
    if ( !CanMoveCurrentDown() )
        return false;

    wxItemContainer *con = GetRearrangeListContainer();
    const int sel = con->GetSelection();

    Swap(sel, sel + 1);
    con->SetSelection(sel + 1);

    return true;
}

void wxRearrangeListBase::Swap(int pos1, int pos2)
{
    wxItemContainer *con = GetRearrangeListContainer();
    // update the internally stored order
    wxSwap(m_order[pos1], m_order[pos2]);


    // and now also swap all the attributes of the items

    // first the label
    const wxString stringTmp = con->GetString(pos1);
    con->SetString(pos1, con->GetString(pos2));
    con->SetString(pos2, stringTmp);

    // then the checked state
    const bool checkedTmp = parentIsChecked(pos1);
    parentCheck(pos1, parentIsChecked(pos2));
    parentCheck(pos2, checkedTmp);

    // and finally the client data, if necessary
    switch ( con->GetClientDataType() )
    {
        case wxClientData_None:
            // nothing to do
            break;

        case wxClientData_Object:
            {
                wxClientData * const dataTmp = con->DetachClientObject(pos1);
                con->SetClientObject(pos1, con->DetachClientObject(pos2));
                con->SetClientObject(pos2, dataTmp);
            }
            break;

        case wxClientData_Void:
            {
                void * const dataTmp = con->GetClientData(pos1);
                con->SetClientData(pos1, con->GetClientData(pos2));
                con->SetClientData(pos2, dataTmp);
            }
            break;
    }
}

void wxRearrangeListBase::Check(unsigned int item, bool check)
{
    if ( check == parentIsChecked(item) )
        return;

    parentCheck(item, check);
    //wxCheckListBox::Check(item, check);

    m_order[item] = ~m_order[item];
}

void wxRearrangeListBase::OnCheck(wxCommandEvent& event)
{
    // update the internal state to match the new item state
    const int n = event.GetInt();

    if ( (m_order[n] >= 0) != parentIsChecked(n) )
        m_order[n] = ~m_order[n];
}

// ============================================================================
// wxRearrangeCtrl implementation
// ============================================================================

/*
BEGIN_EVENT_TABLE(wxRearrangeCtrl, wxPanel)
    EVT_UPDATE_UI(wxID_UP, wxRearrangeCtrl::OnUpdateButtonUI)
    EVT_UPDATE_UI(wxID_DOWN, wxRearrangeCtrl::OnUpdateButtonUI)

    EVT_BUTTON(wxID_UP, wxRearrangeCtrl::OnButton)
    EVT_BUTTON(wxID_DOWN, wxRearrangeCtrl::OnButton)
END_EVENT_TABLE()
//*/

void wxRearrangeCtrlBase::Init()
{
    m_list = NULL;
}

bool
wxRearrangeCtrlBase::Create(wxWindow *parent,
                        wxWindowID id,
                        const wxPoint& pos,
                        const wxSize& size,
                        const wxArrayInt& order,
                        const wxArrayString& items,
                        long style,
                        const wxValidator& validator,
                        const wxString& name)
{
    // create all the windows
    if ( !wxPanel::Create(parent, id, pos, size, wxTAB_TRAVERSAL, name) )
        return false;

    m_list = CreateList(order, items, style, validator);
    
    wxDelegatingItemContainer::Create(m_list->GetRearrangeListContainer());
    
    wxButton * const btnUp = new wxButton(this, wxID_UP);
    wxButton * const btnDown = new wxButton(this, wxID_DOWN);

    // arrange them in a sizer
    wxSizer * const sizerBtns = new wxBoxSizer(wxVERTICAL);
    sizerBtns->Add(btnUp, wxSizerFlags().Centre().Border(wxBOTTOM));
    sizerBtns->Add(btnDown, wxSizerFlags().Centre().Border(wxTOP));

    wxSizer * const sizerTop = new wxBoxSizer(wxHORIZONTAL);
    sizerTop->Add(m_list->GetRearrangeListWindow(), wxSizerFlags(1).Expand().Border(wxRIGHT));
    sizerTop->Add(sizerBtns, wxSizerFlags(0).Centre().Border(wxLEFT));
    SetSizer(sizerTop);

    m_list->GetRearrangeListWindow()->SetFocus();

    Bind(wxEVT_UPDATE_UI, &wxRearrangeCtrl::OnUpdateButtonUI, this, wxID_UP);
    Bind(wxEVT_UPDATE_UI, &wxRearrangeCtrl::OnUpdateButtonUI, this, wxID_DOWN);
    
    Bind(wxEVT_BUTTON, &wxRearrangeCtrl::OnButton, this, wxID_UP);
    Bind(wxEVT_BUTTON, &wxRearrangeCtrl::OnButton, this, wxID_DOWN);

    return true;
}

void wxRearrangeCtrlBase::OnUpdateButtonUI(wxUpdateUIEvent& event)
{
    event.Enable( event.GetId() == wxID_UP ? m_list->CanMoveCurrentUp()
                                           : m_list->CanMoveCurrentDown() );
}

void wxRearrangeCtrlBase::OnButton(wxCommandEvent& event)
{
    if ( event.GetId() == wxID_UP )
        m_list->MoveCurrentUp();
    else
        m_list->MoveCurrentDown();
}

// ============================================================================
// wxRearrangeDialog implementation
// ============================================================================

extern
WXDLLIMPEXP_DATA_CORE(const char) wxRearrangeDialogNameStr[] = "wxRearrangeDlg";

namespace
{

enum wxRearrangeDialogSizerPositions
{
    Pos_Label,
    Pos_Ctrl,
    Pos_Buttons,
    Pos_Max
};

} // anonymous namespace

bool wxRearrangeDialogBase::Create(wxWindow *parent,
                               const wxString& message,
                               const wxString& title,
                               const wxArrayInt& order,
                               const wxArrayString& items,
                               const wxPoint& pos,
                               const wxString& name)
{
    if ( !wxDialog::Create(parent, wxID_ANY, title,
                           pos, wxDefaultSize,
                           wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER,
                           name) )
        return false;

    m_ctrl = CreateCtrl(order, items);

    wxDelegatingItemContainer::Create(m_ctrl);

    // notice that the items in this sizer should be inserted accordingly to
    // wxRearrangeDialogSizerPositions order
    wxSizer * const sizerTop = new wxBoxSizer(wxVERTICAL);

    if ( !message.empty() )
    {
        sizerTop->Add(new wxStaticText(this, wxID_ANY, message),
                      wxSizerFlags().Border());
    }
    else
    {
        // for convenience of other wxRearrangeDialog code that depends on
        // positions of sizer items, insert a dummy zero-sized item
        sizerTop->AddSpacer(0);
    }

    sizerTop->Add(m_ctrl,
                  wxSizerFlags(1).Expand().Border());
    sizerTop->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                  wxSizerFlags().Expand().Border());
    SetSizerAndFit(sizerTop);

    return true;
}

void wxRearrangeDialogBase::AddExtraControls(wxWindow *win)
{
    wxSizer * const sizer = GetSizer();
    wxCHECK_RET( sizer, "the dialog must be created first" );

    wxASSERT_MSG( sizer->GetChildren().GetCount() == Pos_Max,
                  "calling AddExtraControls() twice?" );

    sizer->Insert(Pos_Buttons, win, wxSizerFlags().Expand().Border());

    win->MoveAfterInTabOrder(m_ctrl);

    // we need to update the initial/minimal window size
    sizer->SetSizeHints(this);
}

wxRearrangeListBase *wxRearrangeDialogBase::GetList() const
{
    wxCHECK_MSG( m_ctrl, NULL, "the dialog must be created first" );

    return m_ctrl->GetList();
}

wxArrayInt wxRearrangeDialogBase::GetOrder() const
{
    wxCHECK_MSG( m_ctrl, wxArrayInt(), "the dialog must be created first" );

    return m_ctrl->GetList()->GetCurrentOrder();
}

#endif // wxUSE_REARRANGECTRL
