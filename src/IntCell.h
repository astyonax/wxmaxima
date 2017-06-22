﻿// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//  Copyright (C) 2014-2016 Gunter Königsmann <wxMaxima@physikbuch.de>
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
  This file declares the class IntCell

  IntCell is the MathCell type that represents maxima's <code>integrate()</code> command.
*/


#ifndef INTCELL_H
#define INTCELL_H

#include "MathCell.h"
#include "Setup.h"
#include "CellPointers.h"

/*! This class represents an integral

  This class represents an integral including the integral sign and its contents.
 */
class IntCell : public MathCell
{
public:
  IntCell(MathCell *parent, Configuration **config, CellPointers *cellPointers);

  ~IntCell();

  void MarkAsDeleted();

  virtual wxString GetToolTip(const wxPoint &point){
    return GetToolTipList(point, m_base, m_under, m_over, m_var);
  }

  MathCell *Copy();

  void RecalculateHeight(int fontsize);

  void RecalculateWidths(int fontsize);

  void Draw(wxPoint point, int fontsize);

  void SetBase(MathCell *base);

  //! Set the lower limit of the integral
  void SetUnder(MathCell *under);

  //! Set the higher limit of the integral
  void SetOver(MathCell *name);

  //! Set the integration variable
  void SetVar(MathCell *var);

  enum IntegralType
  {
    INT_DEF, //!< An definite integral, meaning an integral with limits.
    INT_IDEF //!> An indefinite integral, meaning an integral without limits
  };

  //! Choose between definite and indefinite integrals
  void SetIntStyle(IntegralType style)
  {
    m_intStyle = style;
  }

  wxString ToString();

  wxString ToTeX();

  wxString ToMathML();

  wxString ToOMML();

  wxString ToXML();

  void SelectInner(wxRect &rect, MathCell **first, MathCell **last);

  void SetParent(MathCell *parent);

protected:
  //! The part of the formula that is to be integrated.
  MathCell *m_base;
  //! The lower limit of the integral
  MathCell *m_under;
  //! The upper limit of the integral
  MathCell *m_over;
  //! The integration variable
  MathCell *m_var;
  //! The height of the integral sign
  int m_signSize;
  //! The width of the integral sign
  int m_signWidth;
  //! Is this integral definitive?
  IntegralType m_intStyle;
  //! How far is the integral sign's center from the top of this cell?
  int m_signTop;
  int m_charHeight, m_charWidth;
private:
  CellPointers *m_cellPointers;
};

#endif  // INTCELL_H
