/*
 *  Copyright (C) 2004-2005 Andrej Vodopivec <andrejv@users.sourceforge.net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 * 
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */



#ifndef _WXCOMMANDLINE_H_
#define _WXCOMMANDLINE_H_

#include <wx/wx.h>

#include <vector>

class CommandLine : public wxTextCtrl
{
 public:
  CommandLine(wxWindow *parent,
              wxWindowID id,
              const wxString& value,
              const wxPoint& pos,
              const wxSize& size,
              long style);
  ~CommandLine();
  int addToHistory(wxString s);
  wxString previous();
  wxString next();
  wxString complete(wxString s);
  void setMatchParens(bool match) { m_matchParens = match; }
 protected:
  std::vector<wxString> history;
  int history_index;
  void filterLine(wxKeyEvent& event);
  void highligth(wxKeyEvent& event);
  long marked;
  bool m_matchParens;
  DECLARE_EVENT_TABLE()
};


#endif	//_WXCOMMANDLINE_H_
