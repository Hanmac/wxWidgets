///////////////////////////////////////////////////////////////////////////////
// Name:        wx/rearrangectrl.h
// Purpose:     various controls for rearranging the items interactively
// Author:      Vadim Zeitlin
// Created:     2008-12-15
// Copyright:   (c) 2008 Vadim Zeitlin <vadim@wxwidgets.org>
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef _WX_REARRANGECTRL_H_
#define _WX_REARRANGECTRL_H_

#include "wx/checklst.h"

#if wxUSE_REARRANGECTRL

#include "wx/ctrlsub.h"
#include "wx/panel.h"
#include "wx/dialog.h"

#include "wx/arrstr.h"

extern WXDLLIMPEXP_DATA_CORE(const char) wxRearrangeListNameStr[];
extern WXDLLIMPEXP_DATA_CORE(const char) wxRearrangeDialogNameStr[];

// ----------------------------------------------------------------------------
// wxRearrangeList: a (check) list box allowing to move items around
// ----------------------------------------------------------------------------

// This class works allows to change the order of the items shown in it as well
// as to check or uncheck them individually. The data structure used to allow
// this is the order array which contains the items indices indexed by their
// position with an added twist that the unchecked items are represented by the
// bitwise complement of the corresponding index (for any architecture using
// two's complement for negative numbers representation (i.e. just about any at
// all) this means that a checked item N is represented by -N-1 in unchecked
// state).
//
// So, for example, the array order [1 -3 0] used in conjunction with the items
// array ["first", "second", "third"] means that the items are displayed in the
// order "second", "third", "first" and the "third" item is unchecked while the
// other two are checked.

class WXDLLIMPEXP_CORE wxRearrangeListBase
{
public:
    wxRearrangeListBase() { }
    virtual ~wxRearrangeListBase() { }



    // items order
    // -----------

    // get the current items order; the returned array uses the same convention
    // as the one passed to the ctor
    const wxArrayInt& GetCurrentOrder() const { return m_order; }

    // return true if the current item can be moved up or down (i.e. just that
    // it's not the first or the last one)
    bool CanMoveCurrentUp() const;
    bool CanMoveCurrentDown() const;

    // move the current item one position up or down, return true if it was moved
    // or false if the current item was the first/last one and so nothing was done
    bool MoveCurrentUp();
    bool MoveCurrentDown();


    // Override this to keep our m_order array in sync with the real item state.
    virtual void Check(unsigned int item, bool check = true);

    virtual wxItemContainer* GetRearrangeListContainer() = 0;
    virtual const wxItemContainer* GetRearrangeListContainer() const = 0;
    virtual wxWindow* GetRearrangeListWindow() = 0;
    virtual const wxWindow* GetRearrangeListWindow() const = 0;

protected:
    virtual void parentCheck(unsigned int item, bool check = true) = 0;
    virtual bool parentIsChecked(unsigned int item) const = 0;

private:
    // swap two items at the given positions in the listbox
    void Swap(int pos1, int pos2);

public:
    // event handler for item checking/unchecking
    void OnCheck(wxCommandEvent& event);

protected:
    wxArrayString orderItems(const wxArrayString& items, const wxArrayInt& order) const;

    void checkOrder(const wxArrayInt& order);

    // the current order array
    wxArrayInt m_order;

};

template <class W>
class wxRearrangeListTpl : public W, public wxRearrangeListBase
{
public:
    wxRearrangeListTpl() { }

    // ctor creating the control, the arguments are the same as for
    // wxCheckListBox except for the extra order array which defines the
    // (initial) display order of the items as well as their statuses, see the
    // description above
    wxRearrangeListTpl(wxWindow *parent,
                       wxWindowID id,
                       const wxPoint& pos,
                       const wxSize& size,
                       const wxArrayInt& order,
                       const wxArrayString& items,
                       long style = 0,
                       const wxValidator& validator = wxDefaultValidator,
                       const wxString& name = wxRearrangeListNameStr)
    {
        Create(parent, id, pos, size, order, items, style, validator, name);
    }

    bool Create(wxWindow *parent,
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
    if ( !W::Create(parent, id, pos, size, orderItems(items, order),
                                 style, validator, name) )
        return false;

    checkOrder(order);

    bind_events();

    m_order = order;

    return true;
};

    using wxRearrangeListBase::Check;

protected:
    virtual void parentCheck(unsigned int item, bool check = true) { W::Check(item,check); }
    virtual bool parentIsChecked(unsigned int item) const { return W::IsChecked(item); }

    virtual wxItemContainer* GetRearrangeListContainer() { return this; }
    virtual const wxItemContainer* GetRearrangeListContainer() const { return this; }
    virtual wxWindow* GetRearrangeListWindow() { return this; }
    virtual const wxWindow* GetRearrangeListWindow() const { return this; }

    virtual void bind_events() {}
};

//wxEVT_CHECKLISTBOX

template <>
inline void wxRearrangeListTpl<wxCheckListBox>::bind_events()
{
    Bind(wxEVT_CHECKLISTBOX, &wxRearrangeListBase::OnCheck, this);
}



typedef wxRearrangeListTpl<wxCheckListBox> wxRearrangeList;

// ----------------------------------------------------------------------------
// wxRearrangeCtrl: composite control containing a wxRearrangeList and buttons
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxRearrangeCtrlBase : public wxWindowWithItems<wxPanel, wxDelegatingItemContainer>
{


public:
    // ctors/Create function are the same as for wxRearrangeList
    wxRearrangeCtrlBase()
    {
        Init();
    }

    wxRearrangeCtrlBase(wxWindow *parent,
                    wxWindowID id,
                    const wxPoint& pos,
                    const wxSize& size,
                    const wxArrayInt& order,
                    const wxArrayString& items,
                    long style = 0,
                    const wxValidator& validator = wxDefaultValidator,
                    const wxString& name = wxRearrangeListNameStr)
    {
        Init();

        Create(parent, id, pos, size, order, items, style, validator, name);
    }

    bool Create(wxWindow *parent,
                wxWindowID id,
                const wxPoint& pos,
                const wxSize& size,
                const wxArrayInt& order,
                const wxArrayString& items,
                long style = 0,
                const wxValidator& validator = wxDefaultValidator,
                const wxString& name = wxRearrangeListNameStr);

    // get the underlying listbox
    wxRearrangeListBase *GetList() const { return m_list; }

protected:
    virtual wxRearrangeListBase* CreateList(const wxArrayInt& order,
                        const wxArrayString& items,
                        long style,
                        const wxValidator& validator) = 0;

protected:
    // common part of all ctors
    void Init();
private:
    // event handlers for the buttons
    void OnUpdateButtonUI(wxUpdateUIEvent& event);
    void OnButton(wxCommandEvent& event);


    wxRearrangeListBase *m_list;

};

template <class W>
class wxRearrangeCtrlTpl : public wxRearrangeCtrlBase
{
public:
    wxRearrangeCtrlTpl()
    {
        Init();
    }

    wxRearrangeCtrlTpl(wxWindow *parent,
                    wxWindowID id,
                    const wxPoint& pos,
                    const wxSize& size,
                    const wxArrayInt& order,
                    const wxArrayString& items,
                    long style = 0,
                    const wxValidator& validator = wxDefaultValidator,
                    const wxString& name = wxRearrangeListNameStr)
    {
        Init();

        Create(parent, id, pos, size, order, items, style, validator, name);
    }

protected:
    virtual wxRearrangeListBase* CreateList(const wxArrayInt& order,
                                            const wxArrayString& items,
                                            long style,
                                            const wxValidator& validator)
    {
        return new wxRearrangeListTpl<W>(this, wxID_ANY,
                                         wxDefaultPosition, wxDefaultSize,
                                         order, items,
                                         style, validator);

    }

};

typedef wxRearrangeCtrlTpl<wxCheckListBox> wxRearrangeCtrl;

// ----------------------------------------------------------------------------
// wxRearrangeDialog: dialog containing a wxRearrangeCtrl
// ----------------------------------------------------------------------------

class WXDLLIMPEXP_CORE wxRearrangeDialogBase : public wxWindowWithItems<wxDialog, wxDelegatingItemContainer>
{
public:
    wxRearrangeDialogBase() { Init(); }

    bool Create(wxWindow *parent,
                const wxString& message,
                const wxString& title,
                const wxArrayInt& order,
                const wxArrayString& items,
                const wxPoint& pos = wxDefaultPosition,
                const wxString& name = wxRearrangeDialogNameStr);

    // methods for the dialog customization

    // add extra contents to the dialog below the wxRearrangeCtrl part: the
    // given window (usually a wxPanel containing more control inside it) must
    // have the dialog as its parent and will be inserted into it at the right
    // place by this method
    void AddExtraControls(wxWindow *win);

    // return the wxRearrangeList control used by the dialog
    wxRearrangeListBase *GetList() const;


    // get the order of items after it was modified by the user
    wxArrayInt GetOrder() const;

protected:
    // common part of all ctors
    void Init() { m_ctrl = NULL; }

    virtual wxRearrangeCtrlBase* CreateCtrl(const wxArrayInt& order,
                                            const wxArrayString& items) = 0;

private:
    wxRearrangeCtrlBase *m_ctrl;

    wxDECLARE_NO_COPY_CLASS(wxRearrangeDialogBase);
};

template <class W>
class wxRearrangeDialogTpl : public wxRearrangeDialogBase
{
public:
    // default ctor, use Create() later
    wxRearrangeDialogTpl() { Init(); }

    // ctor for the dialog: message is shown inside the dialog itself, order
    // and items are passed to wxRearrangeList used internally
    wxRearrangeDialogTpl(wxWindow *parent,
                      const wxString& message,
                      const wxString& title,
                      const wxArrayInt& order,
                      const wxArrayString& items,
                      const wxPoint& pos = wxDefaultPosition,
                      const wxString& name = wxRearrangeDialogNameStr)
    {
        Init();

        Create(parent, message, title, order, items, pos, name);
    }

protected:

    virtual wxRearrangeCtrlBase* CreateCtrl(const wxArrayInt& order,
                                            const wxArrayString& items)
    {
        return new wxRearrangeCtrlTpl<W>(this, wxID_ANY,
                                         wxDefaultPosition, wxDefaultSize,
                                         order, items);
    }

};

typedef wxRearrangeDialogTpl<wxCheckListBox> wxRearrangeDialog;

#endif // wxUSE_REARRANGECTRL

#endif // _WX_REARRANGECTRL_H_

