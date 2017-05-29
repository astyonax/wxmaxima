// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2008-2009 Ziga Lenarcic <zigalenarcic@users.sourceforge.net>
//            (C) 2012-2013 Doug Ilijev <doug.ilijev@gmail.com>
//            (C) 2015-2016 Gunter Königsmann <wxMaxima@physikbuch.de>
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//

/*! \file
  This file defines the class Notification

  It represents a non-disturbing message to the user.
 */

#include "Notification.h"
Notification::Notification():wxNotificationMessage()
{
  m_shown = false;
  #if wxCHECK_VERSION(3, 1, 0)
  AddAction(wxID_ANY,_("Focus window"));
  #endif
}

Notification::Notification(const wxString &title,
			   const wxString &message,
			   wxWindow *parent,
			   int flags): wxNotificationMessage (title, message, parent, flags)
{
  m_shown = false;
  #if wxCHECK_VERSION(3, 1, 0)
  AddAction(wxID_ANY,_("Focus window"));
  #endif
}

void Notification::Show()
{
  wxNotificationMessage::Show();
  m_shown = true;
}

bool Notification::Close()
{
  m_shown = false;
  return wxNotificationMessage::Close();
}

void Notification::OnClick(wxCommandEvent &event)
{
  if(GetParent() != NULL)
    GetParent()->Raise();
  m_shown = false;
  Close();
}

void Notification::OnDismissed(wxCommandEvent &event)
{
  m_shown = false;
  Close();
}

BEGIN_EVENT_TABLE(Notification, wxNotificationMessage)
  #if wxCHECK_VERSION(3, 1, 0)
  EVT_NOTIFICATION_MESSAGE_CLICK(Notification::OnClick)
  EVT_NOTIFICATION_MESSAGE_DISMISSED(Notification::OnDismissed)
  EVT_NOTIFICATION_MESSAGE_ACTION(Notification::OnDismissed)
  #endif
END_EVENT_TABLE()
