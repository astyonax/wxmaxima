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

#define PAREN_OPEN_TOP_DINGBATS "\xE6"
#define PAREN_OPEN_BOTTOM_DINGBATS "\xE8"
#define PAREN_CLOSE_TOP_DINGBATS "\xF6"
#define PAREN_CLOSE_BOTTOM_DINGBATS "\xF8"
#define PAREN_OPEN_EXTEND_DINGBATS "\xE7"
#define PAREN_CLOSE_EXTEND_DINGBATS "\xF7"

#define PAREN_OPEN_BIG_TEXFONT "\xB0"
#define PAREN_CLOSE_BIG_TEXFONT "\xD1"

#define PAREN_OPEN_TOP_TEXFONT     "\x30"
#define PAREN_OPEN_EXTEND_TEXFONT  "\x42"
#define PAREN_OPEN_BOTTOM_TEXFONT  "\x40"
#define PAREN_CLOSE_TOP_TEXFONT    "\x31"
#define PAREN_CLOSE_EXTEND_TEXFONT "\x43"
#define PAREN_CLOSE_BOTTOM_TEXFONT "\x41"

#define PAREN_OPEN_TOP_UNICODE     "\x239b"
#define PAREN_OPEN_EXTEND_UNICODE  "\x239c"
#define PAREN_OPEN_BOTTOM_UNICODE  "\x239d"
#define PAREN_CLOSE_TOP_UNICODE    "\x239e"
#define PAREN_CLOSE_EXTEND_UNICODE "\x239f"
#define PAREN_CLOSE_BOTTOM_UNICODE "\x23a0"

#define TRANSFORM_SIZE(type, size)            \
  (type == small_texfont ? size:              \
  type  == big_texfont ? 2*size:              \
   (3*size)/2)

ParenCell::ParenCell(MathCell *parent, Configuration **config, CellPointers *cellPointers) : MathCell(parent, config)
{
  m_cellPointers = cellPointers;
  m_charWidth = 12;
  m_charWidth1 = 12;
  m_charHeight = 12;
  m_charHeight1 = 12;
  m_fontSize = 10;
  m_last1 = NULL;
  m_signHeight = 50;
  m_signWidth = 18;
  m_signTop = m_signHeight / 2;
  m_parenFontSize = 12;
  m_bigParenType = ascii;
  m_innerCell = NULL;
  m_print = true;
  m_open = new TextCell(parent, config, cellPointers, wxT("("));
  m_close = new TextCell(parent, config, cellPointers, wxT(")"));
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

MathCell *ParenCell::Copy()
{
  ParenCell *tmp = new ParenCell(m_group, m_configuration, m_cellPointers);
  CopyData(this, tmp);
  tmp->SetInner(m_innerCell->CopyList(), m_type);
  tmp->m_isBroken = m_isBroken;

  return tmp;
}

ParenCell::~ParenCell()
{
  wxDELETE(m_innerCell);
  wxDELETE(m_open);
  wxDELETE(m_close);
  m_innerCell = m_open = m_close = NULL;
  MarkAsDeleted();
}

void ParenCell::MarkAsDeleted()
{
  MarkAsDeletedList(m_innerCell, m_open, m_close);
  if((this == m_cellPointers->m_selectionStart) || (this == m_cellPointers->m_selectionEnd))
    m_cellPointers->m_selectionStart = m_cellPointers->m_selectionEnd = NULL;
  if(this == m_cellPointers->m_cellUnderPointer)
    m_cellPointers->m_cellUnderPointer = NULL;
}

void ParenCell::SetInner(MathCell *inner, int type)
{
  if (inner == NULL)
    return;
  wxDELETE(m_innerCell);
  m_innerCell = inner;
  m_type = type;

  // Tell the first of our inter cell not to begin with a multiplication dot.
  m_innerCell->m_SuppressMultiplicationDot = true;

  // Search for the last of the inner cells
  while (inner->m_next != NULL)
    inner = inner->m_next;
  m_last1 = inner;
  ResetSize();
}

void ParenCell::SetFont(int fontsize)
{
  Configuration *configuration = (*m_configuration);
  wxDC &dc = configuration->GetDC();
  double scale = configuration->GetScale();

  wxString fontName;
  wxFontStyle fontStyle;
  wxFontWeight fontWeight;
  wxFontEncoding fontEncoding;
  bool underlined = configuration->IsUnderlined(TS_DEFAULT);

  // Ensure a sane minimum font size
  if (fontsize < 4)
    fontsize = 4;
  m_parenFontSize = fontsize;
  
  // The font size scales with the worksheet
  int fontsize1 = (int) (((double) fontsize) * scale + 0.5);

  fontEncoding = configuration->GetFontEncoding();

  switch(m_bigParenType)
  {
  case big_texfont:
  case assembled_texfont:
    fontName = configuration->GetTeXCMEX();
    break;

  case assembled_dingbats: 
    fontName = configuration->GetSymbolFontName();
    fontEncoding = wxFONTENCODING_CP1250;
    break;
    
  case small_texfont:
    fontName = configuration->GetTeXCMRI();
    break;
    
  default:
    fontName = configuration->GetFontName(TS_DEFAULT);
  }
  fontStyle = configuration->IsItalic(TS_DEFAULT);
  fontWeight = configuration->IsBold(TS_DEFAULT);
  fontName = configuration->GetFontName(TS_DEFAULT);

  wxFont font;
  font.SetFamily(wxFONTFAMILY_MODERN);
  font.SetFaceName(fontName);
  font.SetEncoding(fontEncoding);
  font.SetStyle(fontStyle);
  font.SetWeight(fontWeight);
  font.SetUnderlined(underlined);
  font.SetEncoding(fontEncoding);
  if (!font.IsOk())
  {
    font.SetFamily(wxFONTFAMILY_MODERN);
    font.SetEncoding(fontEncoding);
    font.SetStyle(fontStyle);
    font.SetWeight(fontWeight);
    font.SetUnderlined(underlined);
  }

  if (!font.IsOk())
    font = *wxNORMAL_FONT;

  font.SetPointSize(fontsize1);
  wxASSERT_MSG(font.IsOk(),
               _("Seems like something is broken with a font. Installing http://www.math.union.edu/~dpvc/jsmath/download/jsMath-fonts.html and checking \"Use JSmath fonts\" in the configuration dialogue should fix it."));
  // A fallback if we have been completely unable to set a working font
  if (!dc.GetFont().IsOk())
    m_bigParenType = handdrawn;

  if(m_bigParenType != handdrawn)
    dc.SetFont(font);
}

void ParenCell::RecalculateWidths(int fontsize)
{
  Configuration *configuration = (*m_configuration);
  double scale = configuration->GetScale();

  // Add a dummy contents to empty parenthesis
  if (m_innerCell == NULL)
    m_innerCell = new TextCell(m_group, m_configuration, m_cellPointers);

  m_innerCell->RecalculateWidthsList(fontsize);
  m_innerCell->RecalculateHeightList(fontsize);

  wxDC &dc = configuration->GetDC();
  int size = m_innerCell->GetMaxHeight();
  if (size < 2) size = 12;
  if (fontsize < 2) fontsize = 12;
  int fontsize1 = (int) ((fontsize * scale + 0.5));

  // If our font provides all the unicode chars we need we don't need
  // to bother which exotic method we need to use for drawing nice parenthesis.

  m_bigParenType = assembled_unicode;
  SetFont(fontsize);
  dc.GetTextExtent(wxT("\xf0000"), &m_signWidth, &m_extendHeight);
  dc.GetTextExtent(wxT(PAREN_OPEN_TOP_UNICODE), &m_signWidth, &m_signHeight);
  std::cerr<<m_extendHeight << " " << m_signHeight<<"\n";

  if((m_extendHeight > 0) && (m_signHeight > 0))
  {
    if (size <= 2 * fontsize1)
    {
      m_bigParenType = ascii;
      dc.GetTextExtent(wxT("("), &m_signWidth, &m_signHeight);
      std::cerr<<"Ascii\n";
    }
    else
    {
      m_bigParenType = assembled_unicode;
      std::cerr<<"Assembled\n";
    } 
  }
  {
    // No font with nice unicode chars.
    // 
    // Do we want to use TeX fonts? This is useful on windows if our normal font
    // lacks the unicode characters we need for drawing big parenthesis and we don't
    // want to use ugly hand-drawn parenthesis.
    if (configuration->CheckTeXFonts())
    {
      /// BUG 2897415: Exporting equations to HTML locks up on Mac
      ///  there is something wrong with what dc.GetTextExtent returns,
      ///  make sure there is no infinite loop!
      // Avoid a possible infinite loop.
      if (size <= 2 * fontsize1)
      {
        m_bigParenType = small_texfont;
        SetFont(fontsize);
        dc.GetTextExtent(wxT("("), &m_signWidth, &m_signHeight);
      }
      else
      {
        if (size <= 4.1 * fontsize1)
        {
          m_bigParenType = big_texfont;
          SetFont(fontsize);
          dc.GetTextExtent(wxT(PAREN_OPEN_BIG_TEXFONT), &m_signWidth, &m_signHeight);
        }
        else
        {
          m_bigParenType = assembled_texfont;
          SetFont(fontsize);
          dc.GetTextExtent(wxT(PAREN_OPEN_TOP_DINGBATS),
                           &m_signWidth, &m_signHeight);
        }
      }

      // Now determine the character sizes we need.
      if (m_bigParenType == assembled_texfont)
      {
        SetFont(fontsize);
        dc.GetTextExtent(wxT(PAREN_OPEN_BIG_TEXFONT), &m_signWidth, &m_signHeight);
      }
      else
      {
        m_parenFontSize = fontsize;
        fontsize1 = (int) ((m_parenFontSize * scale + 0.5));

        if (m_signHeight < 2)
          m_signHeight = 2;

        if (m_signHeight > 0)
        {
          // The i avoids an infinite loop.
          int i = 0;
          while (m_signHeight < TRANSFORM_SIZE(m_bigParenType, size) && i < 40)
          {
            int fontsize1 = (int) ((++m_parenFontSize * scale + 0.5));

            wxString openParentChar;
            switch(m_bigParenType)
            {            
            case big_texfont:
              openParentChar = PAREN_OPEN_BIG_TEXFONT;
              break;
            
            case assembled_texfont:
              openParentChar = PAREN_OPEN_TOP_TEXFONT;
              break;

            default:
              openParentChar = wxT("(");
              break;
            }
            SetFont(fontsize);
            dc.GetTextExtent(openParentChar,&m_signWidth, &m_signHeight);
            i++;
          }
        }
      }

      m_signTop = m_signHeight / 5;
      m_width = m_innerCell->GetFullWidth(scale) + 2 * m_signWidth;
    }
    else
    {
      // No TeX fonts
#if defined __WXMSW__
      if (size <= 2 * fontsize1)
      {
        dc.GetTextExtent(PAREN_OPEN_TOP_DINGBATS, &m_charWidth, &m_charHeight);
        m_bigParenType = ascii;
        SetFont(fontsize);
        dc.GetTextExtent(wxt("("), &m_charWidth, &m_charHeight);
      }
      else
      {
        m_bigParenType = assembled_dingbats;
        SetFont(fontsize);
        dc.GetTextExtent(PAREN_OPEN_TOP_DINGBATS, &m_charWidth, &m_charHeight);
      }
    
      if(m_charHeight < 2)
        m_charHeight = 2;
      m_width = m_innerCell->GetFullWidth(scale) + 2*m_charWidth;
#else
      m_bigParenType = handdrawn;
      m_width = m_innerCell->GetFullWidth(scale) + SCALE_PX(12, configuration->GetScale())
        + 2 * (*m_configuration)->GetDefaultLineWidth();
#endif
    }
  }
  m_open->RecalculateWidthsList(fontsize);
  m_close->RecalculateWidthsList(fontsize);
  ResetData();
}

void ParenCell::RecalculateHeight(int fontsize)
{
  Configuration *configuration = (*m_configuration);
  double scale = configuration->GetScale();
  m_innerCell->RecalculateHeightList(fontsize);
  m_height = m_innerCell->GetMaxHeight() + SCALE_PX(2, scale);
  m_center = m_innerCell->GetMaxCenter() + SCALE_PX(1, scale);

  SetFont(fontsize);
  wxDC& dc = configuration->GetDC();
  dc.GetTextExtent(wxT("("), &m_charWidth1, &m_charHeight1);
  if(m_charHeight1 < 2)
    m_charHeight1 = 2;

  m_open->RecalculateHeightList(fontsize);
  m_close->RecalculateHeightList(fontsize);

  if (m_isBroken)
  {
    m_height = MAX(m_innerCell->GetMaxHeight(), m_open->GetMaxHeight());
    m_center = MAX(m_innerCell->GetMaxCenter(), m_open->GetMaxCenter());
  }
}

void ParenCell::Draw(wxPoint point, int fontsize)
{
  Configuration *configuration = (*m_configuration);
  MathCell::Draw(point, fontsize);
  if (DrawThisCell(point) && (InUpdateRegion()))
  {
    double scale = configuration->GetScale();
    wxDC &dc = configuration->GetDC();
    wxPoint in(point);

    in.x = point.x + m_signWidth;
    SetForeground();
    SetFont(m_parenFontSize);
    
    switch(m_bigParenType)
    {            
    case ascii:
    case small_texfont:
      in.x += m_charWidth;
      dc.DrawText(wxT("("),
                  point.x,
                  point.y - m_center + SCALE_PX(MC_TEXT_PADDING, scale));      
      dc.DrawText(wxT(")"),
                  point.x + m_signWidth + m_innerCell->GetFullWidth(scale),
                  point.y - m_center + SCALE_PX(MC_TEXT_PADDING, scale));
      break;
    case big_texfont:
      in.x += m_charWidth;
      dc.DrawText(wxT(PAREN_OPEN_BIG_TEXFONT),
                  point.x,
                  point.y - m_center + SCALE_PX(MC_TEXT_PADDING, scale) -
                  m_signTop);
      dc.DrawText(wxT(PAREN_CLOSE_BIG_TEXFONT),
                  point.x + m_signWidth + m_innerCell->GetFullWidth(scale),
                  point.y - m_center + SCALE_PX(MC_TEXT_PADDING, scale) -
                  m_signTop);
      break;
    case assembled_texfont:
      in.x += m_charWidth;
      {
        int top = point.y - m_center - m_signTop;
        int bottom = top + m_height - m_signHeight / 2;
        dc.DrawText(wxT(PAREN_OPEN_TOP_TEXFONT),
                    point.x,
                    top);
        dc.DrawText(wxT(PAREN_CLOSE_TOP_TEXFONT),
                    point.x + m_signWidth + m_innerCell->GetFullWidth(scale),
                    top);
        dc.DrawText(wxT(PAREN_OPEN_BOTTOM_TEXFONT),
                    point.x,
                    bottom);
        dc.DrawText(wxT(PAREN_CLOSE_BOTTOM_TEXFONT),
                    point.x + m_signWidth + m_innerCell->GetFullWidth(scale),
                    bottom);
        top = top + m_signHeight / 2;
        
        wxASSERT_MSG(m_signHeight >= 10, _("Font issue: The Parenthesis sign is too small!"));
        if (m_signHeight <= 10)
          m_signHeight = 10;
        
        if (top <= bottom)
        {
          while (top < bottom)
          {
            dc.DrawText(wxT(PAREN_OPEN_EXTEND_TEXFONT),
                        point.x,
                        top - 1);
            dc.DrawText(wxT(PAREN_CLOSE_EXTEND_TEXFONT),
                        point.x + m_width - m_signWidth,
                        top - 1);
            top += m_signHeight / 10;
          }
        }
      }
      break;
    case assembled_unicode:
      in.x += m_charWidth;
      break;
    case assembled_dingbats:
      in.x += m_charWidth;
      SetForeground();
      SetFont(fontsize);
      dc.DrawText(PAREN_OPEN_TOP_DINGBATS,
                  point.x,
                  point.y - m_center);
      dc.DrawText(PAREN_OPEN_BOTTOM_DINGBATS,
                  point.x,
                  point.y + m_height - m_center - m_charHeight);
      dc.DrawText(PAREN_CLOSE_TOP_DINGBATS,
                  point.x + m_width - m_charWidth,
                  point.y - m_center);
      dc.DrawText(PAREN_CLOSE_BOTTOM_DINGBATS,
                  point.x + m_width - m_charWidth,
                  point.y + m_height - m_center - m_charHeight);
      int top, bottom;
      top = point.y - m_center + m_charHeight/2;
      bottom = point.y + m_height - m_center - (4*m_charHeight)/3;

      wxASSERT_MSG(m_charHeight>=2,_("Font issue: The char height is too small! Installing http://www.math.union.edu/~dpvc/jsmath/download/jsMath-fonts.html and checking \"Use JSmath fonts\" in the configuration dialogue should be a workaround."));
      if(m_charHeight <= 2)
        m_charHeight = 2;

      if (top <= bottom)
      {
        while (top < bottom)
        {
          dc.DrawText(PAREN_OPEN_EXTEND_DINGBATS,
                      point.x,
                      top);
          dc.DrawText(PAREN_CLOSE_EXTEND_DINGBATS,
                      point.x + m_width - m_charWidth,
                      top);
          top += (2*m_charHeight)/3;
        }
        dc.DrawText(PAREN_OPEN_EXTEND_DINGBATS,
                    point.x,
                    point.y + m_height - m_center - (3*m_charHeight)/2);
        dc.DrawText(PAREN_CLOSE_EXTEND_DINGBATS,
                    point.x + m_width - m_charWidth,
                    point.y + m_height - m_center - (3*m_charHeight)/2);
      }
      break;
    case handdrawn:
      in.x = point.x + SCALE_PX(6, scale) + (*m_configuration)->GetDefaultLineWidth();
      SetPen();
      // left
      dc.DrawLine(point.x + SCALE_PX(5, scale) + (*m_configuration)->GetDefaultLineWidth() / 2,
                  point.y - m_innerCell->GetMaxCenter() + SCALE_PX(1, scale),
                  point.x + SCALE_PX(2, scale) + (*m_configuration)->GetDefaultLineWidth() / 2,
                  point.y - m_innerCell->GetMaxCenter() + SCALE_PX(7, scale));
      dc.DrawLine(point.x + SCALE_PX(2, scale) + (*m_configuration)->GetDefaultLineWidth() / 2,
                  point.y - m_innerCell->GetMaxCenter() + SCALE_PX(7, scale),
                  point.x + SCALE_PX(2, scale) + (*m_configuration)->GetDefaultLineWidth() / 2,
                  point.y + m_innerCell->GetMaxDrop() - SCALE_PX(7, scale));
      dc.DrawLine(point.x + SCALE_PX(2, scale) + (*m_configuration)->GetDefaultLineWidth() / 2,
                  point.y + m_innerCell->GetMaxDrop() - SCALE_PX(7, scale),
                  point.x + SCALE_PX(5, scale) + (*m_configuration)->GetDefaultLineWidth() / 2,
                  point.y + m_innerCell->GetMaxDrop() - SCALE_PX(1, scale));
      // right
      dc.DrawLine(point.x + m_width - SCALE_PX(5, scale) - 1 - (*m_configuration)->GetDefaultLineWidth() / 2,
                  point.y - m_innerCell->GetMaxCenter() + SCALE_PX(1, scale),
                  point.x + m_width - SCALE_PX(2, scale) - 1 - (*m_configuration)->GetDefaultLineWidth() / 2,
                  point.y - m_innerCell->GetMaxCenter() + SCALE_PX(7, scale));
      dc.DrawLine(point.x + m_width - SCALE_PX(2, scale) - 1 - (*m_configuration)->GetDefaultLineWidth() / 2,
                  point.y - m_innerCell->GetMaxCenter() + SCALE_PX(7, scale),
                  point.x + m_width - SCALE_PX(2, scale) - 1 - (*m_configuration)->GetDefaultLineWidth() / 2,
                  point.y + m_innerCell->GetMaxDrop() - SCALE_PX(7, scale));
      dc.DrawLine(point.x + m_width - SCALE_PX(2, scale) - 1 - (*m_configuration)->GetDefaultLineWidth() / 2,
                  point.y + m_innerCell->GetMaxDrop() - SCALE_PX(7, scale),
                  point.x + m_width - SCALE_PX(5, scale) - 1 - (*m_configuration)->GetDefaultLineWidth() / 2,
                  point.y + m_innerCell->GetMaxDrop() - SCALE_PX(1, scale));
      break;
    }
    
    UnsetPen();
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
    for (size_t i = 0; i < innerCell.Length(); i++)
      if (!wxIsalnum(innerCell[i]))
      {
        needsLeftRight = true;
        break;
      }

    if (m_print)
    {
      if (needsLeftRight)
        s = wxT("\\left( ") + m_innerCell->ListToTeX() + wxT("\\right) ");
      else
        s = wxT("(") + m_innerCell->ListToTeX() + wxT(")");
    }
    else
      s = m_innerCell->ListToTeX();
  }
  return s;
}

wxString ParenCell::ToOMML()
{
  return wxT("<m:d><m:dPr m:begChr=\"") + XMLescape(m_open->ToString()) + wxT("\" m:endChr=\"") +
         XMLescape(m_close->ToString()) + wxT("\" m:grow=\"1\"></m:dPr><m:e>") +
         m_innerCell->ListToOMML() + wxT("</m:e></m:d>");
}

wxString ParenCell::ToMathML()
{
  if (!m_print) return m_innerCell->ListToMathML();

  wxString open = m_open->ToString();
  wxString close = m_close->ToString();
  return (
          wxT("<mrow><mo>") + XMLescape(open) + wxT("</mo>") +
          m_innerCell->ListToMathML() +
          wxT("<mo>") + XMLescape(close) + wxT("</mo></mrow>\n")
  );
}

wxString ParenCell::ToXML()
{
//  if( m_isBroken )
//    return wxEmptyString;
  wxString s = m_innerCell->ListToXML();
  return ((m_print) ? _T("<r><p>") + s + _T("</p></r>") : s);
}

void ParenCell::SelectInner(wxRect &rect, MathCell **first, MathCell **last)
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
    wxASSERT_MSG(m_last1 != NULL, _("Bug: No last cell inside a parenthesis!"));
    if (m_last1 != NULL)
    {
      m_last1->m_nextToDraw = m_close;
      m_close->m_previousToDraw = m_last1;
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

void ParenCell::Unbreak()
{
  if (m_isBroken)
    m_innerCell->UnbreakList();
  MathCell::Unbreak();
}
