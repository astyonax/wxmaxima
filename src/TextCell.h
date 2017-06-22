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

#ifndef TEXTCELL_H
#define TEXTCELL_H

#include "wx/regex.h"
#include "MathCell.h"
#include "CellPointers.h"

/*! A Text cell

  Everything on the worksheet that is composed of characters with the eception
  of input cells: Input cells are handled by EditorCell instead.
 */
class TextCell : public MathCell
{
private:
  //! Is an ending "(" of a function name the opening parenthesis of the function?
  bool m_dontEscapeOpeningParenthesis;
public:
  TextCell(MathCell *parent, Configuration **config, CellPointers *cellPointers);

  TextCell(MathCell *parent, Configuration **config, CellPointers *cellPointers, wxString text);

  void MarkAsDeleted();
  
  CellPointers *m_cellPointers;

  ~TextCell();
  
  MathCell *Copy();

  //! Set the text contained in this cell
  void SetValue(const wxString &text);

  //! Set the automatic label maxima has assigned the current equation
  void SetUserDefinedLabel(wxString userDefinedLabel){m_userDefinedLabel = userDefinedLabel;}

  void RecalculateWidths(int fontsize);

  void Draw(wxPoint point, int fontsize);

  void SetFont(int fontsize);

  /*! Calling this function signals that the "(" this cell ends in isn't part of the function name

    The "(" is the opening parenthesis of a function instead.
   */
  void DontEscapeOpeningParenthesis()
  { m_dontEscapeOpeningParenthesis = true; }

  wxString ToString();

  wxString ToTeX();

  wxString ToMathML();

  wxString ToOMML();

  wxString ToRTF();

  wxString ToXML();

  wxString GetDiffPart();

  bool IsOperator();

  wxString GetValue()
  { return m_text; }

  wxString GetGreekStringTeX();

  wxString GetSymbolTeX();

#if wxUSE_UNICODE

  wxString GetGreekStringUnicode();

  wxString GetSymbolUnicode(bool keepPercent);

#elif defined __WXMSW__
  wxString GetGreekStringSymbol();
  wxString GetSymbolSymbol(bool keepPercent);
#endif

  bool IsShortNum();

protected:
  void SetAltText();
  
  //! Resets the font size to label size
  void SetFontSizeForLabel(wxDC &dc, double scale);

  wxRegEx m_unescapeRegEx;

  //! The text we keep inside this cell
  wxString m_text;
  //! The text we keep inside this cell
  wxString m_userDefinedLabel;
  //! The text we display: m_text might be a number that is longer than we want to display
  wxString m_displayedText;
  //! How many maximum digits did we display the last time this cell was recalculated?
  int m_displayedDigits_old;
  wxString m_altText, m_altJsText;
  wxString m_fontname, m_texFontname;

  bool m_alt, m_altJs;
  int m_realCenter;
  /*! The font size we had the last time we were recalculating this cell

    If a fraction or similar is broken into two lines this changes \f$ \frac{a}{b}\f$ to 
    \f$ a/b\f$. \f$ \Longrightarrow\f$ we need a mechanism that tells us that the font 
    size has changed and we need to re-calculate the text width.
   */
  int m_lastCalculationFontSize;
  //! The line height
  int m_fontSize;
  //! The actual font size for labels (that have a fixed width)
  int m_fontSizeLabel;
  int m_labelWidth, m_labelHeight;
private:
  //! Produces a text sample that determines the label width
  wxString LabelWidthText();
  wxString m_initialToolTip;
};

#endif // TEXTCELL_H
