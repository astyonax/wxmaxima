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
  This file defines the class IntCell

  IntCell is the MathCell type that represents maxima's <code>integrate()</code> command.
*/

#include "IntCell.h"
#include "TextCell.h"

#define INTEGRAL_TOP wxT("\x2320")
#define INTEGRAL_BOTTOM wxT("\x2321")
#define INTEGRAL_EXTEND wxT("\x23AE")

IntCell::IntCell(MathCell *parent, Configuration **config, CellPointers *cellPointers) : MathCell(parent, config)
{
  m_base = NULL;
  m_under = NULL;
  m_over = NULL;
  m_var = NULL;
  m_useUnicode = no;
  m_signHeight = 50;
  m_extendHeight = 0;
  m_signTopHeight = 0;
  m_signBotHeight = 0;
  m_extendNum = 1;
  m_signWidth = 18;
  m_signTop = m_signHeight / 2;
  m_intStyle = INT_IDEF;
  m_charWidth = 12;
  m_charHeight = 12;
  m_cellPointers = cellPointers;
}

void IntCell::SetParent(MathCell *parent)
{
  m_group = parent;
  if (m_base != NULL)
    m_base->SetParentList(parent);
  if (m_under != NULL)
    m_under->SetParentList(parent);
  if (m_over != NULL)
    m_over->SetParentList(parent);
  if (m_var != NULL)
    m_var->SetParentList(parent);
}

MathCell *IntCell::Copy()
{
  IntCell *tmp = new IntCell(m_group, m_configuration, m_cellPointers);
  CopyData(this, tmp);
  tmp->SetBase(m_base->CopyList());
  tmp->SetUnder(m_under->CopyList());
  tmp->SetOver(m_over->CopyList());
  tmp->SetVar(m_var->CopyList());
  tmp->m_intStyle = m_intStyle;

  return tmp;
}

IntCell::~IntCell()
{
  wxDELETE(m_base);
  m_base = NULL;
  wxDELETE(m_under);
  m_under = NULL;
  wxDELETE(m_over);
  m_over = NULL;
  wxDELETE(m_var);
  m_var = NULL;
  MarkAsDeleted();
}

void IntCell::MarkAsDeleted()
{
  MarkAsDeletedList(m_base, m_under, m_over, m_var);
  if((this == m_cellPointers->m_selectionStart) || (this == m_cellPointers->m_selectionEnd))
    m_cellPointers->m_selectionStart = m_cellPointers->m_selectionEnd = NULL;
  if(this == m_cellPointers->m_cellUnderPointer)
    m_cellPointers->m_cellUnderPointer = NULL;
}

void IntCell::SetOver(MathCell *over)
{
  if (over == NULL)
    return;
  wxDELETE(m_over);
  m_over = over;
}

void IntCell::SetBase(MathCell *base)
{
  if (base == NULL)
    return;
  wxDELETE(m_base);
  m_base = base;
}

void IntCell::SetUnder(MathCell *under)
{
  if (under == NULL)
    return;
  wxDELETE(m_under);
  m_under = under;
}

void IntCell::SetVar(MathCell *var)
{
  if (var == NULL)
    return;
  wxDELETE(m_var);
  m_var = var;
}

void IntCell::RecalculateWidths(int fontsize)
{
  Configuration *configuration = (*m_configuration);
  wxDC &dc = configuration->GetDC();
  double scale = configuration->GetScale();
  
  int signWidth1,signWidth2,signWidth3,descent,leading;
  m_useUnicode = currentFont;
  SetFont(fontsize);
  dc.GetTextExtent(INTEGRAL_TOP,    &signWidth1, &m_signTopHeight, &descent, &leading);
  m_signTopHeight -= descent + 1;
  dc.GetTextExtent(INTEGRAL_EXTEND, &signWidth2, &m_extendHeight, &descent, &leading);
  m_extendHeight -= descent + 1;
  dc.GetTextExtent(INTEGRAL_BOTTOM, &signWidth3, &m_signBotHeight, &descent, &leading);
  m_signBotHeight -= descent + 1;
  
  if(
    (signWidth1 < 1 ) ||
    (signWidth2 < 1 ) ||
    (signWidth3 < 1 ) ||
    (m_signTopHeight < 1) ||
    (m_extendHeight < 1) ||
    (m_signBotHeight < 1)
    )
  {
    m_useUnicode = fallbackFont;
    SetFont(fontsize);
    dc.GetTextExtent(INTEGRAL_TOP,    &signWidth1, &m_signTopHeight, &descent, &leading);
    m_signTopHeight -= descent + 1;
    dc.GetTextExtent(INTEGRAL_EXTEND, &signWidth2, &m_extendHeight, &descent, &leading);
    m_extendHeight -= descent + 1;
    dc.GetTextExtent(INTEGRAL_BOTTOM, &signWidth3, &m_signBotHeight, &descent, &leading);
    m_signBotHeight -= descent + 1;
  
    if(
      (signWidth1 < 1 ) ||
      (signWidth2 < 1 ) ||
      (signWidth3 < 1 ) ||
      (m_signTopHeight < 1) ||
      (m_extendHeight < 1) ||
      (m_signBotHeight < 1)
      )
    {
      m_useUnicode = fallbackFont2;
      SetFont(fontsize);
      dc.GetTextExtent(INTEGRAL_TOP,    &signWidth1, &m_signTopHeight, &descent, &leading);
      m_signTopHeight -= descent + 1;
      dc.GetTextExtent(INTEGRAL_EXTEND, &signWidth2, &m_extendHeight, &descent, &leading);
      m_extendHeight -= descent + 1;
      dc.GetTextExtent(INTEGRAL_BOTTOM, &signWidth3, &m_signBotHeight, &descent, &leading);
      m_signBotHeight -= descent + 1;
  
      if(
        (signWidth1 < 1 ) ||
        (signWidth2 < 1 ) ||
        (signWidth3 < 1 ) ||
        (m_signTopHeight < 1) ||
        (m_extendHeight < 1) ||
        (m_signBotHeight < 1)
        )
      {
        m_useUnicode = no;
      }
    }
  }

  if(m_useUnicode == no)
  {
    m_signHeight = SCALE_PX(50, scale);
    m_signWidth = SCALE_PX(18, scale);
  }
  else
  {
    m_signHeight = m_signTopHeight + m_extendHeight + m_signBotHeight;
    m_signWidth = signWidth1;
    if(m_signWidth < signWidth2)
      m_signWidth = signWidth2;
    if(m_signWidth < signWidth3)
      m_signWidth = signWidth3;  
  }
  
  m_base->RecalculateWidthsList(fontsize);
  m_var->RecalculateWidthsList(fontsize);
  if (m_under == NULL)
    m_under = new TextCell(m_group, m_configuration, m_cellPointers);
  m_under->RecalculateWidthsList(MAX(MC_MIN_SIZE, fontsize - 5));
  if (m_over == NULL)
    m_over = new TextCell(m_group, m_configuration, m_cellPointers);
  m_over->RecalculateWidthsList(MAX(MC_MIN_SIZE, fontsize - 5));
  
  m_width = m_signWidth +
    MAX(m_over->GetFullWidth(scale) + m_signWidth, m_under->GetFullWidth(scale)) +
    m_base->GetFullWidth(scale) +
    m_var->GetFullWidth(scale) +
    SCALE_PX(4, scale);
  ResetData();
}

void IntCell::RecalculateHeight(int fontsize)
{
  Configuration *configuration = (*m_configuration);
  double scale = configuration->GetScale();

  m_under->RecalculateHeightList(MAX(MC_MIN_SIZE, fontsize - 5));
  m_over->RecalculateHeightList(MAX(MC_MIN_SIZE, fontsize - 5));
  m_base->RecalculateHeightList(fontsize);
  m_var->RecalculateHeightList(fontsize);

  if (m_intStyle == INT_DEF)
  {
      m_center = MAX(m_over->GetMaxHeight() + SCALE_PX(4, scale) + m_signHeight / 2 - m_signHeight / 3,
                     m_base->GetMaxCenter());
      m_height = m_center +
                 MAX(m_under->GetMaxHeight() + SCALE_PX(4, scale) + m_signHeight / 2 - m_signHeight / 3,
                     m_base->GetMaxDrop());
  }
  else
  {
    m_center = MAX(m_signHeight / 2, m_base->GetMaxCenter());
    m_height = m_center +
      MAX(m_signHeight / 2, m_base->GetMaxDrop());
  }
}

void IntCell::Draw(wxPoint point, int fontsize)
{
  MathCell::Draw(point, fontsize);
  Configuration *configuration = (*m_configuration);

  if (DrawThisCell(point) && InUpdateRegion())
  {
    wxDC &dc = configuration->GetDC();
    double scale = configuration->GetScale();
    
    wxPoint base, under, over, var;
    if(m_useUnicode == no)
    {
      SetPen();
      // top decoration
      int m_signWCenter = m_signWidth / 2;
      dc.DrawLine(point.x + m_signWCenter,
                  point.y - (m_signHeight + 1) / 2 + SCALE_PX(12, scale) - 1,
                  point.x + m_signWCenter + SCALE_PX(3, scale),
                  point.y - (m_signHeight + 1) / 2 + SCALE_PX(3, scale));
      dc.DrawLine(point.x + m_signWCenter + SCALE_PX(3, scale),
                  point.y - (m_signHeight + 1) / 2 + SCALE_PX(3, scale),
                  point.x + m_signWCenter + SCALE_PX(6, scale),
                  point.y - (m_signHeight + 1) / 2);
      dc.DrawLine(point.x + m_signWCenter + SCALE_PX(6, scale),
                  point.y - (m_signHeight + 1) / 2,
                  point.x + m_signWCenter + SCALE_PX(9, scale),
                  point.y - (m_signHeight + 1) / 2 + SCALE_PX(3, scale));
      // bottom decoration
      dc.DrawLine(point.x + m_signWCenter,
                  point.y + (m_signHeight + 1) / 2 - SCALE_PX(12, scale) + 1,
                  point.x + m_signWCenter - SCALE_PX(3, scale),
                  point.y + (m_signHeight + 1) / 2 - SCALE_PX(3, scale));
      dc.DrawLine(point.x + m_signWCenter - SCALE_PX(3, scale),
                  point.y + (m_signHeight + 1) / 2 - SCALE_PX(3, scale),
                  point.x + m_signWCenter - SCALE_PX(6, scale),
                  point.y + (m_signHeight + 1) / 2);
      dc.DrawLine(point.x + m_signWCenter - SCALE_PX(6, scale),
                  point.y + (m_signHeight + 1) / 2,
                  point.x + m_signWCenter - SCALE_PX(9, scale),
                  point.y + (m_signHeight + 1) / 2 - SCALE_PX(3, scale));
      // line
      dc.DrawLine(point.x + m_signWCenter,
                  point.y - (m_signHeight + 1) / 2 + SCALE_PX(12, scale) - 1,
                  point.x + m_signWCenter,
                  point.y + (m_signHeight + 1) / 2 - SCALE_PX(12, scale) + 1);
      UnsetPen();
    }
    else
    {
      SetForeground();
      SetFont(fontsize);
      dc.DrawText(INTEGRAL_TOP,
                  point.x,
                  m_extendHeight/4+point.y - (m_signHeight)/2 + SCALE_PX(1,scale));
      for(int i=0;i<m_extendNum;i++)
        dc.DrawText(INTEGRAL_EXTEND,
                    point.x,
                    m_extendHeight/4+point.y - m_extendHeight*(m_extendNum+1)/2 + m_extendNum*i + SCALE_PX(1,scale));
      dc.DrawText(INTEGRAL_BOTTOM,
                  point.x,
                  m_extendHeight/4+point.y + m_extendHeight*(m_extendNum-1)/2 +SCALE_PX(1,scale));
    }
    
    if (m_intStyle == INT_DEF)
    {
      under.x = point.x + m_signWidth;
      under.y = point.y + m_signHeight / 2 + m_under->GetMaxCenter() + SCALE_PX(2, scale) -
        m_signHeight / 3;
      m_under->DrawList(under, MAX(MC_MIN_SIZE, fontsize - 5));
      
      over.x = point.x + m_signWidth;
      over.y = point.y - m_signHeight / 2 - m_over->GetMaxDrop() - SCALE_PX(2, scale) +
        m_signHeight / 3;
      m_over->DrawList(over, MAX(MC_MIN_SIZE, fontsize - 5));
      
      base.x = point.x + m_signWidth +
        MAX(m_over->GetFullWidth(scale), m_under->GetFullWidth(scale));
    }
    else
      base.x += point.x + m_signWidth;
    base.y = point.y;
    
    m_base->DrawList(base, fontsize);
    
    var.x = base.x + m_base->GetFullWidth(scale);
    var.y = point.y;
    m_var->DrawList(var, fontsize);
  }
}

wxString IntCell::ToString()
{
  wxString s = wxT("integrate(");

  s += m_base->ListToString();

  MathCell *tmp = m_var;
  wxString var;
  tmp = tmp->m_next;
  if (tmp != NULL)
  {
    var = tmp->ListToString();
  }

  wxString to = m_over->ListToString();
  wxString from = m_under->ListToString();

  s += wxT(",") + var;
  if (m_intStyle == INT_DEF)
    s += wxT(",") + from + wxT(",") + to;

  s += wxT(")");
  return s;
}

wxString IntCell::ToTeX()
{
  wxString s = wxT("\\int");

  wxString to = m_over->ListToTeX();
  wxString from = m_under->ListToTeX();

  if (m_intStyle == INT_DEF)
    s += wxT("_{") + from + wxT("}^{") + to + wxT("}");
  else
    s += wxT(" ");

  s += wxT("{\\left. ");
  s += m_base->ListToTeX();
  s += m_var->ListToTeX();
  s += wxT("\\right.}");

  return s;
}

wxString IntCell::ToMathML()
{
  wxString base = m_base->ListToMathML();

  wxString var;
  if (m_var) var = m_var->ListToMathML();

  wxString from;
  if (m_under) from = m_under->ListToMathML();

  wxString to;
  if (m_over) to = m_over->ListToMathML();

  wxString retval;
  if (from.IsEmpty() && to.IsEmpty())
    retval = wxT("<mo>&#x222B;</mo>") + base;
  if (from.IsEmpty() && !to.IsEmpty())
    retval = wxT("<mover><mo>&#x222B;</mo>") + to + wxT("</mover>") + base;
  if (!from.IsEmpty() && to.IsEmpty())
    retval = wxT("<munder><mo>&#x222B;</mo>") + from + wxT("</munder>") + base;
  if (!from.IsEmpty() && !to.IsEmpty())
    retval = wxT("<munderover><mo>&#x222B;</mo>") + from + to + wxT("</munderover>\n") + base;
  if (!var.IsEmpty())
    retval = retval + var;

  return (wxT("<mrow>") + retval + wxT("</mrow>"));
}

wxString IntCell::ToOMML()
{
  wxString base = m_base->ListToOMML();

  wxString var;
  if (m_var) var = m_var->ListToOMML();

  wxString from;
  if (m_under) from = m_under->ListToOMML();

  wxString to;
  if (m_over) to = m_over->ListToOMML();

  wxString retval;

  retval = wxT("<m:nary><m:naryPr><m:chr>\x222b</m:chr></m:naryPr>");
  if (from != wxEmptyString)
    retval += wxT("<m:sub>") + from + wxT("</m:sub>");
  if (to != wxEmptyString)
    retval += wxT("<m:sup>") + to + wxT("</m:sup>");
  retval += wxT("<m:e><m:r>") + base + var + wxT("</m:r></m:e></m:nary>");

  return retval;
}

wxString IntCell::ToXML()
{
  wxString from;
  if (m_under != NULL)
    from = m_under->ListToXML();
  from = wxT("<r>") + from + wxT("</r>");

  wxString to;
  if (m_over != NULL)
    to = m_over->ListToXML();
  to = wxT("<r>") + to + wxT("</r>");

  wxString base;
  if (m_base != NULL)
    base = m_base->ListToXML();
  base = wxT("<r>") + base + wxT("</r>");

  wxString var;
  if (m_var != NULL)
    var = m_var->ListToXML();
  var = wxT("<r>") + var + wxT("</r>");

  if (m_intStyle == INT_DEF)
    return wxT("<in>") + from + to + base + var + wxT("</in>");
  else
    return wxT("<in def=\"false\">") + base + var + wxT("</in>");
}

void IntCell::SelectInner(wxRect &rect, MathCell **first, MathCell **last)
{
  *first = NULL;
  *last = NULL;
  if (m_over->ContainsRect(rect))
    m_over->SelectRect(rect, first, last);
  else if (m_under->ContainsRect(rect))
    m_under->SelectRect(rect, first, last);
  else if (m_base->ContainsRect(rect))
    m_base->SelectRect(rect, first, last);
  if (*first == NULL || *last == NULL)
  {
    *first = this;
    *last = this;
  }
}

void IntCell::SetFont(int fontsize)
{
  Configuration *configuration = (*m_configuration);
  wxDC &dc = configuration->GetDC();
  double scale = configuration->GetScale();
  wxString fontName;
  wxFontStyle fontStyle;
  wxFontWeight fontWeight;
  wxFontEncoding fontEncoding;
  bool underlined = false;

  // Ensure a sane minimum font size
  if (fontsize < 4)
    fontsize = 4;
  
  // The font size scales with the worksheet
  int fontsize1 = (int) (((double) fontsize) * scale + 0.5);
  
  wxFont font;
  font.SetFamily(wxFONTFAMILY_MODERN);
  switch(m_useUnicode)
  {
  case no:
    // In this case we don't need to set a font
    return;
  case currentFont:
    fontName = configuration->GetFontName(TS_DEFAULT);
    break;
  case fallbackFont:
    fontName = wxT("Linux Libertine");
    break;
  case fallbackFont2:
    fontName = wxT("Linux Libertine O");
    break;
  }
  fontEncoding = configuration->GetFontEncoding();
  fontName = configuration->GetFontName(TS_DEFAULT);
  fontStyle = configuration->IsItalic(TS_DEFAULT);
  fontWeight = configuration->IsBold(TS_DEFAULT);
  fontName = configuration->GetFontName(TS_DEFAULT);
  font.SetFaceName(fontName);
  font.SetEncoding(fontEncoding);
  font.SetStyle(fontStyle);
  font.SetWeight(fontWeight);
  font.SetUnderlined(underlined);
  font.SetPointSize(fontsize1);
  if (!font.IsOk())
    m_useUnicode = no;
  else
    dc.SetFont(font);
}
