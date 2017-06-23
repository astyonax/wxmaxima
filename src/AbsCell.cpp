﻿// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2014-2016 Gunter Königsmann <wxMaxima@physikbuch.de>
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
  This file defines the class AbsCell

  AbsCell is the MathCell type that represents the field that represents the 
  <code>abs()</code> and <code>cabs()</code> commands.
*/


#include "AbsCell.h"
#include "TextCell.h"

AbsCell::AbsCell(MathCell *parent, Configuration **config, CellPointers *cellPointers) : MathCell(parent, config)
{
  m_cellPointers = cellPointers;
  m_innerCell = NULL;
  m_open = new TextCell(parent, config, cellPointers, wxT("abs("));
  m_open->DontEscapeOpeningParenthesis();
  m_close = new TextCell(parent, config, cellPointers, wxT(")"));
  m_last = NULL;
}

void AbsCell::SetParent(MathCell *parent)
{
  m_group = parent;
  if (m_innerCell != NULL)
    m_innerCell->SetParentList(parent);
  if (m_open != NULL)
    m_open->SetParentList(parent);
  if (m_close != NULL)
    m_close->SetParentList(parent);
}

MathCell *AbsCell::Copy()
{
  AbsCell *tmp = new AbsCell(m_group, m_configuration, m_cellPointers);
  CopyData(this, tmp);
  tmp->SetInner(m_innerCell->CopyList());
  tmp->m_isBroken = m_isBroken;
  tmp->m_open->DontEscapeOpeningParenthesis();

  return tmp;
}

AbsCell::~AbsCell()
{
  wxDELETE(m_innerCell);
  wxDELETE(m_open);
  wxDELETE(m_close);
  m_innerCell = NULL;
  m_open = NULL;
  m_close = NULL;
  MarkAsDeleted();
}

void AbsCell::MarkAsDeleted()
{
  MarkAsDeletedList(m_innerCell, m_open, m_close);
  if((this == m_cellPointers->m_selectionStart) || (this == m_cellPointers->m_selectionEnd))
    m_cellPointers->m_selectionStart = m_cellPointers->m_selectionEnd = NULL;
  if(this == m_cellPointers->m_cellUnderPointer)
    m_cellPointers->m_cellUnderPointer = NULL;
}


void AbsCell::SetInner(MathCell *inner)
{
  if (inner == NULL)
    return;
  wxDELETE(m_innerCell);
  m_innerCell = inner;

  m_last = m_innerCell;
  if (m_last != NULL)
    while (m_last->m_next != NULL)
      m_last = m_last->m_next;
}

void AbsCell::RecalculateWidths(int fontsize)
{
  Configuration *configuration = (*m_configuration);
  double scale = configuration->GetScale();
  m_innerCell->RecalculateWidthsList(fontsize);
  m_width = m_innerCell->GetFullWidth(scale) + SCALE_PX(8, scale) + 2 * (*m_configuration)->GetDefaultLineWidth();
  m_open->RecalculateWidthsList(fontsize);
  m_close->RecalculateWidthsList(fontsize);
  ResetData();
}

void AbsCell::RecalculateHeight(int fontsize)
{
  Configuration *configuration = (*m_configuration);
  double scale = configuration->GetScale();
  m_innerCell->RecalculateHeightList(fontsize);
  m_height = m_innerCell->GetMaxHeight() + SCALE_PX(4, scale);
  m_center = m_innerCell->GetMaxCenter() + SCALE_PX(2, scale);
  m_open->RecalculateHeightList(fontsize);
  m_close->RecalculateHeightList(fontsize);

  if (m_isBroken)
  {
    m_height = MAX(m_innerCell->GetMaxHeight(), m_open->GetMaxHeight());
    m_center = MAX(m_innerCell->GetMaxCenter(), m_open->GetMaxCenter());
  }
}

void AbsCell::Draw(wxPoint point, int fontsize)
{
  MathCell::Draw(point, fontsize);

  Configuration *configuration = (*m_configuration);
  double scale = configuration->GetScale();
  wxDC &dc = configuration->GetDC();
  if (DrawThisCell(point) && InUpdateRegion())
  {
    SetPen();
    wxPoint in;
    in.x = point.x + SCALE_PX(4, scale) + (*m_configuration)->GetDefaultLineWidth();
    in.y = point.y;
    m_innerCell->DrawList(in, fontsize);

    dc.DrawLine(point.x + SCALE_PX(2, scale) + (*m_configuration)->GetDefaultLineWidth() / 2,
                point.y - m_center + SCALE_PX(2, scale),
                point.x + SCALE_PX(2, scale) + (*m_configuration)->GetDefaultLineWidth() / 2,
                point.y - m_center + m_height - SCALE_PX(2, scale));
    dc.DrawLine(point.x + m_width - SCALE_PX(2, scale) - 1 - (*m_configuration)->GetDefaultLineWidth() / 2,
                point.y - m_center + SCALE_PX(2, scale),
                point.x + m_width - SCALE_PX(2, scale) - 1 - (*m_configuration)->GetDefaultLineWidth() / 2,
                point.y - m_center + m_height - SCALE_PX(2, scale));
    UnsetPen();
  }
}

wxString AbsCell::ToString()
{
  if (m_isBroken)
    return wxEmptyString;
  wxString s;
  s = wxT("abs(") + m_innerCell->ListToString() + wxT(")");
  return s;
}

wxString AbsCell::ToTeX()
{
  if (m_isBroken)
    return wxEmptyString;
  return wxT("\\left| ") + m_innerCell->ListToTeX() + wxT("\\right| ");
}

wxString AbsCell::ToMathML()
{
  return wxT("<row><mo>|</mo>") +
         m_innerCell->ListToMathML() +
         wxT("<mo>|</mo></row>\n");
//  return wxT("<apply><abs/><ci>") + m_innerCell->ListToMathML() + wxT("</ci></apply>");
}

wxString AbsCell::ToOMML()
{
  return wxT("<m:d><m:dPr m:begChr=\"|\" m:endChr=\"|\"></m:dPr><m:e>") +
         m_innerCell->ListToOMML() + wxT("</m:e></m:d>");
}

wxString AbsCell::ToXML()
{
  return wxT("<a>") + m_innerCell->ListToXML() + wxT("</a>");
}

void AbsCell::SelectInner(wxRect &rect, MathCell **first, MathCell **last)
{
  *first = NULL;
  *last = NULL;

  if (m_innerCell->ContainsRect(rect))
    m_innerCell->SelectRect(rect, first, last);

  if (*first == NULL || *last == NULL)
  {
    *first = this;
    *last = this;
  }
}

bool AbsCell::BreakUp()
{
  if (!m_isBroken)
  {
    m_isBroken = true;
    m_open->m_nextToDraw = m_innerCell;
    m_innerCell->m_previousToDraw = m_open;
    wxASSERT_MSG(m_last != NULL, _("Bug: No last cell in an absCell!"));
    if (m_last != NULL)
    {
      m_last->m_nextToDraw = m_close;
      m_close->m_previousToDraw = m_last;
    }
    m_close->m_nextToDraw = m_nextToDraw;
    if (m_nextToDraw != NULL)
      m_nextToDraw->m_previousToDraw = m_close;
    m_nextToDraw = m_open;

    m_height = MAX(m_innerCell->GetMaxHeight(), m_open->GetMaxHeight());
    m_center = MAX(m_innerCell->GetMaxCenter(), m_open->GetMaxCenter());

    return true;
  }
  return false;
}

void AbsCell::Unbreak()
{
  if (m_isBroken)
    m_innerCell->UnbreakList();
  MathCell::Unbreak();
}
