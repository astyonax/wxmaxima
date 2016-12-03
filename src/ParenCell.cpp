// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
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
  This file defines the class ParenCell

  ParenCell is the MathCell type that represents a math element that is kept
  between parenthesis.
*/

#include "ParenCell.h"
#include "TextCell.h"

#define PAREN_FONT_SIZE 12

/*
  #define PAREN_OPEN "\xB0"
  #define PAREN_CLOSE "\xD1"
  #define PAREN_OPEN_TOP "\x30"
  #define PAREN_OPEN_EXTEND "\x42"
  #define PAREN_OPEN_BOTTOM "\x40"
  #define PAREN_CLOSE_TOP "\x31"
  #define PAREN_CLOSE_EXTEND "\x43"
  #define PAREN_CLOSE_BOTTOM "\x41"
*/

#define PAREN_OPEN "\x27ee"
#define PAREN_CLOSE "\x27ef"
#define PAREN_OPEN_TOP "\x2398"
#define PAREN_OPEN_EXTEND "\x239c"
#define PAREN_OPEN_BOTTOM "\x239d"
#define PAREN_CLOSE_TOP "\x239e"
#define PAREN_CLOSE_EXTEND "\x239f"
#define PAREN_CLOSE_BOTTOM "\x23a0"

#define TRANSFORM_SIZE(type, size)              \
  (type == 0 ? size:                            \
   type == 1 ? 2*size:                          \
   (3*size)/2)

ParenCell::ParenCell() : MathCell()
{
  m_last1 = NULL;
  m_signSize = 50;
  m_signWidth = 18;
  m_signTop = m_signSize / 2;
  m_parenFontSize = 12;
  m_bigParenType = PARENTHESIS_NORMAL;
  m_innerCell = NULL;
  m_print = true;
  m_open = new TextCell(wxT("("));
  m_close = new TextCell(wxT(")"));
}


ParenCell::~ParenCell()
{
  if (m_innerCell != NULL)
    delete m_innerCell;
  if (m_next != NULL)
    delete m_next;
  delete m_open;
  delete m_close;
}

void ParenCell::SetParent(MathCell *parent)
{
  m_group = parent;
  if (m_innerCell != NULL)
    m_innerCell->SetParentList(parent);
  if (m_open != NULL)
    m_open->SetParentList(parent);
  if (m_close != NULL)
    m_close->SetParentList(parent);
}

MathCell* ParenCell::Copy()
{
  ParenCell *tmp = new ParenCell;
  CopyData(this, tmp);
  tmp->SetInner(m_innerCell->CopyList(), m_type);
  tmp->m_isBroken = m_isBroken;

  return tmp;
}

void ParenCell::Destroy()
{
  if (m_innerCell != NULL)
    delete m_innerCell;
  m_innerCell = NULL;
  m_next = NULL;
}

void ParenCell::SetInner(MathCell *inner, int type)
{
  if (inner == NULL)
    return ;
  if (m_innerCell != NULL)
    delete m_innerCell;
  m_innerCell = inner;
  m_type = type;

  // Tell the first of our inter cell not to begin with a multiplication dot.
  m_innerCell->m_SuppressMultiplicationDot=true;

  // Search for the last of the inner cells
  while (inner->m_next != NULL)
    inner = inner->m_next;
  m_last1 = inner;
}

void ParenCell::RecalculateWidths(int fontsize)
{
  Configuration *configuration = Configuration::Get();
  double scale = configuration->GetScale();
  if (m_innerCell == NULL)
    m_innerCell = new TextCell;

  m_innerCell->RecalculateWidthsList(fontsize);

  wxDC& dc = configuration->GetDC();
  m_innerCell->RecalculateHeightList(fontsize);
  int size = m_innerCell->GetMaxHeight();
  /// BUG 2897415: Exporting equations to HTML locks up on Mac
  ///  there is something wrong with what dc.GetTextExtent returns,
  ///  make sure there is no infinite loop!
  // Avoid a possible infinite loop.
  if(size < 2) size = 12;
    
  int fontsize1 = (int) ((fontsize * scale + 0.5));

  if (size <= 2*fontsize1)
  {
    m_bigParenType = PARENTHESIS_NORMAL;
    dc.SetFont(
      wxFont(fontsize1, wxFONTFAMILY_MODERN,
             wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
             configuration->GetTeXCMRI())
      );
    dc.GetTextExtent( wxT("("),&m_signWidth, &m_signSize);
  }
  else
  {
    if (size <= 4.1*fontsize1)
    {
      m_bigParenType = PARENTHESIS_BIG;
      dc.SetFont( wxFont(fontsize1, wxFONTFAMILY_MODERN,
                         wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
                         configuration->GetTeXCMEX()));
      dc.GetTextExtent(wxT(PAREN_OPEN),&m_signWidth, &m_signSize);
    }
    else
    {
      m_bigParenType = PARENTHESIS_ASSEMBLED;
      dc.GetTextExtent(wxT(PAREN_OPEN_TOP),
                       &m_signWidth, &m_signSize);
    }
  }
    
  if (m_bigParenType != PARENTHESIS_ASSEMBLED)
  {
    m_parenFontSize = fontsize;
    fontsize1 = (int) ((m_parenFontSize * scale + 0.5));
      
    int i=0;
      
      
    if(m_signSize > 0)
      while (m_signSize < TRANSFORM_SIZE(m_bigParenType, size) && i<1000)
      {
        int fontsize1 = (int) ((++m_parenFontSize * scale + 0.5));
        dc.SetFont(wxFont(fontsize1, wxFONTFAMILY_MODERN,
                          wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
                          m_bigParenType == 0 ?
                          configuration->GetTeXCMRI() :
                          configuration->GetTeXCMEX()));
        dc.GetTextExtent(m_bigParenType == 0 ? wxT("(") :
                         m_bigParenType == 1 ? wxT(PAREN_OPEN) :
                         wxT(PAREN_OPEN_TOP),
                         &m_signWidth, &m_signSize);
        // Avoid an infinite loop.
        if(m_signSize < 2) m_signSize = 2;
        i++;
      }
  }
  else
  {
    m_parenFontSize = fontsize;
    fontsize1 = (int) ((m_parenFontSize * scale + 0.5));
    dc.SetFont(wxFont(fontsize1, wxFONTFAMILY_MODERN,
                      wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
                      m_bigParenType < 1 ?
                      configuration->GetTeXCMRI() :
                      configuration->GetTeXCMEX()));
    dc.GetTextExtent(wxT(PAREN_OPEN), &m_signWidth, &m_signSize);
  }

  m_signTop = m_signSize / 5;
  m_width = m_innerCell->GetFullWidth(scale) + 2*m_signWidth;

  m_open->RecalculateWidthsList(fontsize);
  m_close->RecalculateWidthsList(fontsize);
  ResetData();
}

void ParenCell::RecalculateHeight(int fontsize)
{
  Configuration *configuration = Configuration::Get();
  double scale = configuration->GetScale();
  m_innerCell->RecalculateHeightList(fontsize);
  m_height = m_innerCell->GetMaxHeight() + SCALE_PX(2, scale);
  m_center = m_innerCell->GetMaxCenter() + SCALE_PX(1, scale);

  m_open->RecalculateHeightList(fontsize);
  m_close->RecalculateHeightList(fontsize);
}

void ParenCell::Draw(wxPoint point, int fontsize)
{
  Configuration *configuration = Configuration::Get();
  MathCell::Draw(point, fontsize);
  if (DrawThisCell(point)&&(InUpdateRegion()))
  {
    double scale = configuration->GetScale();
    wxDC& dc = configuration->GetDC();
    wxPoint in(point);

    in.x = point.x + m_signWidth;
    SetForeground();
    int fontsize1 = (int) ((m_parenFontSize * scale + 0.5));
    dc.SetFont(wxFont(fontsize1, wxFONTFAMILY_MODERN,
                      wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false,
                      m_bigParenType < 1 ?
                      configuration->GetTeXCMRI() :
                      configuration->GetTeXCMEX()));
    if (m_bigParenType < 2)
    {
      dc.DrawText(m_bigParenType == 0 ? wxT("(") :
                  wxT(PAREN_OPEN),
                  point.x,
                  point.y - m_center + SCALE_PX(MC_TEXT_PADDING, scale) -
                  (m_bigParenType > 0 ? m_signTop : 0));
      dc.DrawText(m_bigParenType == 0 ? wxT(")") :
                  wxT(PAREN_CLOSE),
                  point.x + m_signWidth + m_innerCell->GetFullWidth(scale),
                  point.y - m_center + SCALE_PX(MC_TEXT_PADDING, scale) -
                  (m_bigParenType > 0 ? m_signTop : 0));
    }
    else
    {
      int top =    point.y - m_center - m_signTop;
      int bottom = top + m_height - m_signSize / 2;
      dc.DrawText(wxT(PAREN_OPEN_TOP),
                  point.x,
                  top);
      dc.DrawText(wxT(PAREN_CLOSE_TOP),
                  point.x + m_signWidth + m_innerCell->GetFullWidth(scale),
                  top);
      dc.DrawText(wxT(PAREN_OPEN_BOTTOM),
                  point.x,
                  bottom);
      dc.DrawText(wxT(PAREN_CLOSE_BOTTOM),
                  point.x + m_signWidth + m_innerCell->GetFullWidth(scale),
                  bottom);
      top = top + m_signSize / 2;

      wxASSERT_MSG(m_signSize>=10,_("Font issue: The Parenthesis sign is too small!"));
      if(m_signSize <= 10)
        m_signSize = 10;

      if (top <= bottom)
      {
        while (top < bottom)
        {
          dc.DrawText(wxT(PAREN_OPEN_EXTEND),
                      point.x,
                      top-1);
          dc.DrawText(wxT(PAREN_CLOSE_EXTEND),
                      point.x + m_width - m_signWidth,
                      top-1);
          top += m_signSize / 10;
        }
      }
    }
    m_innerCell->DrawList(in, fontsize);
  }
}

wxString ParenCell::ToString()
{
  wxString s;
  if (!m_isBroken)
  {
    if (m_print)
      s = wxT("(") + m_innerCell->ListToString() + wxT(")");
    else
      s = m_innerCell->ListToString();
  }
  return s;
}

wxString ParenCell::ToTeX()
{
  wxString s;
  if (!m_isBroken)
  {
    wxString innerCell = m_innerCell->ListToTeX();

    // Let's see if the cell contains anything potentially higher than a normal
    // character.
    bool needsLeftRight = false;
    for(size_t i=0;i<innerCell.Length();i++)
      if(!wxIsalnum(innerCell[i]))
      {
        needsLeftRight = true;
        break;
      }
    
    if (m_print)
    {
      if(needsLeftRight)
        s = wxT("\\left( ") + m_innerCell->ListToTeX()  + wxT("\\right) ");
      else
        s = wxT("(") + m_innerCell->ListToTeX()  + wxT(")");
    }
    else
      s = m_innerCell->ListToTeX();
  }
  return s;
}

wxString ParenCell::ToOMML()
{
  return wxT("<m:d><m:dPr m:begChr=\"") + XMLescape(m_open->ToString()) + wxT("\" m:endChr=\"") +
    XMLescape(m_close->ToString()) + wxT("\"></m:dPr><m:e>") +
    m_innerCell->ListToOMML()+wxT("</m:e></m:d>");
}

wxString ParenCell::ToMathML()
{
  if(!m_print) return m_innerCell->ListToMathML();

  wxString open   = m_open->ToString();
  wxString close  = m_close->ToString();
  return(
    wxT("<mrow><mo>") + XMLescape(open) + wxT("</mo>") +
    m_innerCell->ListToMathML() +
    wxT("<mo>") + XMLescape(close) +  wxT("</mo></mrow>\n")
    );
}

wxString ParenCell::ToXML()
{
//  if( m_isBroken )
//    return wxEmptyString;
  wxString s = m_innerCell->ListToXML();
  return ( ( m_print )? _T("<r><p>") + s + _T("</p></r>") : s );
}

void ParenCell::SelectInner(wxRect& rect, MathCell **first, MathCell **last)
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

bool ParenCell::BreakUp()
{
  if (!m_isBroken)
  {
    m_isBroken = true;
    m_open->m_nextToDraw = m_innerCell;
    m_innerCell->m_previousToDraw = m_open;
    wxASSERT_MSG(m_last1 != NULL,_("Bug: No last cell inside a parenthesis!"));
    if(m_last1 != NULL)
    {
      m_last1->m_nextToDraw = m_close;
      m_close->m_previousToDraw = m_last1;
    }
    m_close->m_nextToDraw = m_nextToDraw;
    if (m_nextToDraw != NULL)
      m_nextToDraw->m_previousToDraw = m_close;
    m_nextToDraw = m_open;
    return true;
  }
  return false;
}

void ParenCell::Unbreak()
{
  if (m_isBroken)
    m_innerCell->UnbreakList();
  MathCell::Unbreak();
}

