// -*- mode: c++; c-file-style: "linux"; c-basic-offset: 2; indent-tabs-mode: nil -*-
//
//  Copyright (C) 2004-2015 Andrej Vodopivec <andrej.vodopivec@gmail.com>
//            (C) 2014-2015 Gunter Königsmann <wxMaxima@physikbuch.de>
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

#ifndef IMGCELL_H
#define IMGCELL_H

#include "MathCell.h"
#include <wx/image.h>
#include "Image.h"

#include <wx/filesys.h>
#include <wx/fs_arc.h>

class ImgCell : public MathCell
{
public:
  ImgCell(MathCell *parent, Configuration **config);
  ImgCell(MathCell *parent, Configuration **config,wxMemoryBuffer image,wxString type);
  ImgCell(MathCell *parent, Configuration **config,wxString image, bool remove = true, wxFileSystem *filesystem = NULL);
  ImgCell(MathCell *parent, Configuration **config,const wxBitmap &bitmap);
  ~ImgCell();
  void LoadImage(wxString image, bool remove = true);
  MathCell* Copy();
  void SelectInner(wxRect& rect, MathCell** first, MathCell** last)
  {
    *first = *last = this;
  }
  friend class SlideShow;
  /*! Writes the image to a file

    The image file that is written is either a bit-per-bit copy of the original
    file loaded into the ImgCell - or in the case that there is no original file
    a losslessly compressed png version of the bitmap.

    See also GetExtension().
   */
  wxSize ToImageFile(wxString filename);
  /*! Removes the cached scaled image from memory

    The scaled version of the image will be recreated automatically once it is 
    needed.
   */
  virtual void ClearCache(){if(m_image)m_image->ClearCache();}
  //! Sets the bitmap that is shown
  void SetBitmap(const wxBitmap &bitmap);
  //! Copies the cell to the system's clipboard
  bool CopyToClipboard();
  // These methods should only be used for saving wxmx files
  // and are shared with SlideShowCell.
  static void WXMXResetCounter() { s_counter = 0; }
  static wxString WXMXGetNewFileName();
  static int WXMXImageCount() { return s_counter; }
  void DrawRectangle(bool draw) { m_drawRectangle = draw; }
  //! Returns the file name extension that matches the image type
  wxString GetExtension(){if(m_image)return m_image->GetExtension(); else return wxEmptyString;}
  //! Returnes the original compressed version of the image
  wxMemoryBuffer GetCompressedImage(){return m_image->m_compressedImage;}
protected:
  Image *m_image;
  void RecalculateHeight(int fontsize);
  void RecalculateWidths(int fontsize);
  void Draw(wxPoint point, int fontsize);
  wxString ToString();
  wxString ToRTF();
  wxString ToTeX();
  wxString ToXML();
	static int s_counter;
	bool m_drawRectangle;
  virtual void DrawBoundingBox(wxDC& dc, bool all = false)
    {
      m_drawBoundingBox = true;
    }
private:
  bool m_drawBoundingBox;
};

#endif // IMGCELL_H
