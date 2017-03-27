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

#ifndef SQRTCELL_H
#define SQRTCELL_H

#include "MathCell.h"
#include "TextCell.h"

/*! \file

  This file defines the class for the cell type that represents a square root.
 */

/*! This class represents a square root
  
  In the case that this cell is broken into two lines in the order of
  m_nextToDraw this cell is represented by the following individual 
  cells:
  
   - The SqrtCell itself
   - The opening "sqrt("
   - The contents
   - The closing ")".
   
  If it isn't broken into multiple cells m_nextToDraw points to the 
  cell that follows this Cell.
 */
class SqrtCell : public MathCell
{
public:
  SqrtCell(MathCell *parent, Configuration **config);
  ~SqrtCell();
  MathCell* Copy();
  void SetInner(MathCell *inner);
  void SelectInner(wxRect& rect, MathCell** first, MathCell** last);
  void RecalculateHeight(int fontsize);
  void RecalculateWidths(int fontsize);
  void Draw(wxPoint point, int fontsize);
  bool BreakUp();
  void Unbreak();
  wxString ToString();
  wxString ToTeX();
  wxString ToMathML();
  wxString ToOMML();
  wxString ToXML();
  void SetParent(MathCell *parent);
protected:
  MathCell *m_innerCell;
  TextCell *m_open, *m_close;
  MathCell *m_last;
  int m_signWidth, m_signSize, m_signTop;
  int m_signType;
  double m_signFontScale;
};

#endif // SQRTCELL_H
