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

#ifndef EXPTCELL_H
#define EXPTCELL_H

#include "MathCell.h"
#include "TextCell.h"
#include "CellPointers.h"

/*!\file

  This file declares the class ExptCell which represents a exp() or %e^x-construct.
 */

/*! This cell represents a exp() or %e^x-construct.

  In the case that this cell is broken into two lines in the order of
  m_nextToDraw this cell is represented by the following individual 
  cells:
  
   - The ExptCell itself
   - The opening "exp("
   - The contents
   - The closing ")".
   
  If it isn't broken into multiple cells m_nextToDraw points to the 
  cell that follows this Cell. 
 */
class ExptCell : public MathCell
{
public:
  ExptCell(MathCell *parent, Configuration **config, CellPointers *cellpointers);

  ~ExptCell();

  void MarkAsDeleted();

  virtual wxString GetToolTip(const wxPoint &point){
    if(ContainsPoint(point))
      {
        // Default assumption: will be overwritten by the next command,
        // if there is a more accurate solution.
        m_cellPointers->m_cellUnderPointer = this;
      }
      return GetToolTipList(point, m_baseCell, m_powCell);
  }

  MathCell *Copy();

  //! Set the mantissa
  void SetBase(MathCell *base);

  //! Set the exponent
  void SetPower(MathCell *power);

  void RecalculateHeight(int fontsize);

  void RecalculateWidths(int fontsize);

  void Draw(wxPoint point, int fontsize);

  wxString ToString();

  wxString ToTeX();

  wxString ToXML();

  wxString ToOMML();

  wxString ToMathML();

  wxString GetDiffPart();

  void SelectInner(wxRect &rect, MathCell **first, MathCell **last);

  void IsMatrix(bool isMatrix)
  {
    m_isMatrix = isMatrix;
  }

  bool BreakUp();

  void Unbreak();

  void SetParent(MathCell *parent);

protected:
  MathCell *m_baseCell, *m_powCell;
  TextCell *m_open, *m_close;
  MathCell *m_last2;
  MathCell *m_exp, *m_last1;
  bool m_isMatrix;

private:
  CellPointers *m_cellPointers;
};


#endif // EXPTCELL_H
