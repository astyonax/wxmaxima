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
  This file defines the class MathCell

  MathCell is the base class for all cell- or list-type elements.
*/

#include "MathCell.h"
#include <wx/regex.h>
#include <wx/sstream.h>

MathCell::MathCell(MathCell *parent, Configuration **config)
{
  m_toolTip = wxEmptyString;
  m_group = parent;
  m_configuration = config;
  m_next = NULL;
  m_previous = NULL;
  m_nextToDraw = NULL;
  m_previousToDraw = NULL;
  m_group = NULL;
  m_fullWidth = -1;
  m_lineWidth = -1;
  m_maxCenter = -1;
  m_maxDrop = -1;
  m_width = -1;
  m_height = -1;
  m_center = -1;
  m_breakLine = false;
  m_breakPage = false;
  m_forceBreakLine = false;
  m_bigSkip = true;
  m_isHidden = false;
  m_isBroken = false;
  m_highlight = false;
  m_type = MC_TYPE_DEFAULT;
  m_textStyle = TS_VARIABLE;
  m_SuppressMultiplicationDot = false;
  m_imageBorderWidth = 0;
  m_currentPoint.x = -1;
  m_currentPoint.y = -1;
  m_toolTip = (*m_configuration)->GetDefaultMathCellToolTip();
}

MathCell::~MathCell()
{
  // Find the last cell in this list of cells
  MathCell *last = this;
  while (last->m_next != NULL)
    last = last->m_next;

  // Delete all cells beginning with the last one
  while ((last != NULL) && (last != this))
  {
    MathCell *tmp = last;
    last = last->m_previous;
    wxDELETE(tmp);
    last->m_next = NULL;
  }
}

void MathCell::SetType(int type)
{
  m_type = type;

  switch (m_type)
  {
    case MC_TYPE_MAIN_PROMPT:
      m_textStyle = TS_MAIN_PROMPT;
      break;
    case MC_TYPE_PROMPT:
      m_textStyle = TS_OTHER_PROMPT;
      break;
    case MC_TYPE_LABEL:
      m_textStyle = TS_LABEL;
      break;
    case MC_TYPE_INPUT:
      m_textStyle = TS_INPUT;
      break;
    case MC_TYPE_ERROR:
      m_textStyle = TS_ERROR;
      break;
    case MC_TYPE_WARNING:
      m_textStyle = TS_WARNING;
      break;
    case MC_TYPE_TEXT:
      m_textStyle = TS_TEXT;
      break;
    case MC_TYPE_SUBSUBSECTION:
      m_textStyle = TS_SUBSUBSECTION;
      break;
    case MC_TYPE_SUBSECTION:
      m_textStyle = TS_SUBSECTION;
      break;
    case MC_TYPE_SECTION:
      m_textStyle = TS_SECTION;
      break;
    case MC_TYPE_TITLE:
      m_textStyle = TS_TITLE;
      break;
    default:
      m_textStyle = TS_DEFAULT;
      break;
  }
  ResetSize();
}

MathCell *MathCell::CopyList()
{
  MathCell *dest = Copy();
  MathCell *src = this->m_next;
  MathCell *ret = dest;

  while (src != NULL)
  {
    dest->AppendCell(src->Copy());
    src = src->m_next;
    dest = dest->m_next;
  }

  return ret;
}

void MathCell::ClearCacheList()
{
  MathCell *tmp = this;

  while (tmp != NULL)
  {
    tmp->ClearCache();
    tmp = tmp->m_next;
  }
}

void MathCell::SetParentList(MathCell *parent)
{
  MathCell *tmp = this;
  while (tmp != NULL)
  {
    tmp->SetParent(parent);
    tmp = tmp->m_next;
  }
}

/***
 * Append new cell to the end of this list.
 */
void MathCell::AppendCell(MathCell *p_next)
{
  if (p_next == NULL)
    return;
  m_maxDrop = -1;
  m_maxCenter = -1;

  // Search the last cell in the list
  MathCell *LastInList = this;
  while (LastInList->m_next != NULL)
    LastInList = LastInList->m_next;

  // Append this p_next to the list
  LastInList->m_next = p_next;
  LastInList->m_next->m_previous = LastInList;

  // Search the last cell in the list that is sorted by the drawing order
  MathCell *LastToDraw = LastInList;
  while (LastToDraw->m_nextToDraw != NULL)
    LastToDraw = LastToDraw->m_nextToDraw;

  // Append p_next to this list.
  LastToDraw->m_nextToDraw = p_next;
  p_next->m_previousToDraw = LastToDraw;
}

MathCell *MathCell::GetParent()
{
  wxASSERT_MSG(m_group != NULL, _("Bug: Math Cell that claims to have no group Cell it belongs to"));
  return m_group;
}

/***
 * Get the maximum drop of the center.
 */
int MathCell::GetMaxCenter()
{
  if (m_maxCenter < 0)
  {
    MathCell *tmp = this;
    while (tmp != NULL)
    {
      m_maxCenter = MAX(m_maxCenter, tmp->m_center);
      if (tmp->m_nextToDraw == NULL)
        break;
      if (tmp->m_nextToDraw->m_breakLine)
        break;
      tmp = tmp->m_nextToDraw;
    }
  }
  return m_maxCenter;
}

/***
 * Get the maximum drop of cell.
 */
int MathCell::GetMaxDrop()
{

  if (m_maxDrop < 0)
  {
    MathCell *tmp = this;
    while (tmp != NULL)
    {
      m_maxDrop = MAX(m_maxDrop, tmp->m_isBroken ? 0 : (tmp->m_height - tmp->m_center));
      if (tmp->m_nextToDraw == NULL)
        break;
      if (tmp->m_nextToDraw->m_breakLine && !tmp->m_nextToDraw->m_isBroken)
        break;
      tmp = tmp->m_nextToDraw;
    }
  }
  return m_maxDrop;
}

//!  Get the maximum hight of cells in line.
int MathCell::GetMaxHeight()
{
  return GetMaxCenter() + GetMaxDrop();
}

/*! Get full width of this group.
 */
int MathCell::GetFullWidth(double scale)
{
  // Recalculate the with of this list of cells only if this has been marked as necessary.
  if (m_fullWidth < 0)
  {
    MathCell *tmp = this;

    // We begin this calculation with a negative offset since the full width of only a single
    // cell doesn't contain the space that separates two cells - that is automatically added
    // to every cell in the next step.
    m_fullWidth = -SCALE_PX(MC_CELL_SKIP, scale);
    while (tmp != NULL)
    {
      m_fullWidth += tmp->m_width;
      tmp = tmp->m_next + SCALE_PX(MC_CELL_SKIP, scale);
    }
  }
  return m_fullWidth;
}

/*! Get the width of this line.
 */
int MathCell::GetLineWidth(double scale)
{
  if (m_lineWidth < 0)
  {
    m_lineWidth = 0;
    int width = m_width;

    MathCell *tmp = this;
    while(tmp != NULL)
    {      
      width += tmp->m_width;
      width += SCALE_PX(MC_CELL_SKIP, scale);
      
      if (width > m_lineWidth)
        m_lineWidth = width;

      tmp = tmp->m_nextToDraw;
      if(tmp != NULL)
      {
        if(tmp->m_isBroken || tmp->m_breakLine || (tmp->m_type == MC_TYPE_MAIN_PROMPT))
          break;
      }
    }
  }
  return m_lineWidth;
}

/*! Draw this cell to dc

 To make this work each derived class must draw the content of the cell
 and then call MathCall::Draw(...).
 */
void MathCell::Draw(wxPoint point, int fontsize)
{
  m_currentPoint = point;
}

void MathCell::DrawList(wxPoint point, int fontsize)
{
  Configuration *configuration = (*m_configuration);
  MathCell *tmp = this;
  while (tmp != NULL)
  {
    tmp->Draw(point, fontsize);
    double scale = configuration->GetScale();
    point.x += tmp->m_width + SCALE_PX(MC_CELL_SKIP, scale);
    tmp = tmp->m_nextToDraw;
  }
}

void MathCell::RecalculateList(int fontsize)
{
  MathCell *tmp = this;

  while (tmp != NULL)
  {
    tmp->RecalculateWidths(fontsize);
    tmp->RecalculateHeight(fontsize);
    tmp = tmp->m_next;
  }
}

void MathCell::ResetSizeList()
{
  MathCell *tmp = this;

  while (tmp != NULL)
  {
    tmp->ResetSize();
    tmp = tmp->m_next;
  }
}


void MathCell::RecalculateHeightList(int fontsize)
{
  MathCell *tmp = this;

  while (tmp != NULL)
  {
    tmp->RecalculateHeight(fontsize);
    tmp = tmp->m_next;
  }
}

/*! Recalculate widths of cells. 

  (Used for changing font size since in this case all size information has to be 
  recalculated).
  
  Should set: set m_width.
*/
void MathCell::RecalculateWidthsList(int fontsize)
{
  MathCell *tmp = this;

  while (tmp != NULL)
  {
    tmp->RecalculateWidths(fontsize);
    tmp = tmp->m_next;
  }
}

void MathCell::RecalculateWidths(int fontsize)
{
  ResetData();
}

/*! Is this cell currently visible in the window?.
 */
bool MathCell::DrawThisCell(wxPoint point)
{
  Configuration *configuration = (*m_configuration);
  int top = configuration->GetTop();
  int bottom = configuration->GetBottom();
  if (top == -1 || bottom == -1)
    return true;
  if (point.y - GetMaxCenter() > bottom || point.y + GetMaxDrop() < top)
    return false;
  return true;
}

/*! Get the rectangle around this cell

  \param all
   - true  return the rectangle around the whole line.
   - false return the rectangle around this cell. 
 */
wxRect MathCell::GetRect(bool all)
{
  if (m_isBroken)
    return wxRect(-1, -1, 0, 0);
  if (all)
    return wxRect(m_currentPoint.x, m_currentPoint.y - GetMaxCenter(),
                  GetLineWidth(1.0), GetMaxHeight());
  return wxRect(m_currentPoint.x, m_currentPoint.y - m_center,
                m_width, m_height);
}

bool MathCell::InUpdateRegion(const wxRect &rect)
{
  if (m_clipToDrawRegion) return true;
  if (rect.GetLeft() > m_updateRegion.GetRight()) return false;
  if (rect.GetRight() < m_updateRegion.GetLeft()) return false;
  if (rect.GetBottom() < m_updateRegion.GetTop()) return false;
  if (rect.GetTop() > m_updateRegion.GetBottom()) return false;
  return true;
}

wxRect MathCell::CropToUpdateRegion(const wxRect &rect)
{
  if (m_clipToDrawRegion) return rect;

  int left = rect.GetLeft();
  int top = rect.GetTop();
  int right = rect.GetRight();
  int bottom = rect.GetBottom();
  if (left < m_updateRegion.GetLeft()) left = m_updateRegion.GetLeft();
  if (right > m_updateRegion.GetRight()) right = m_updateRegion.GetRight();
  if (top < m_updateRegion.GetTop()) top = m_updateRegion.GetTop();
  if (bottom > m_updateRegion.GetBottom()) bottom = m_updateRegion.GetBottom();

  // Windows seems to utterly dislike rectangles with the width or height 0.
  if (bottom == top) bottom++;
  if (left == right) right++;
  return wxRect(wxPoint(left, top), wxPoint(right, bottom));
}

void MathCell::DrawBoundingBox(wxDC &dc, bool all)
{
  wxRect rect = GetRect(all);
  if (InUpdateRegion())
    dc.DrawRectangle(CropToUpdateRegion(rect));
}

/***
 * Do we have an operator in this line - draw () in frac...
 */
bool MathCell::IsCompound()
{
  if (IsOperator())
    return true;
  if (m_next == NULL)
    return false;
  return m_next->IsCompound();
}

/***
 * Is operator - draw () in frac...
 */
bool MathCell::IsOperator()
{
  return false;
}

/***
 * Return the string representation of cell.
 */
wxString MathCell::ToString()
{
  return wxEmptyString;
}

wxString MathCell::ListToString()
{
  wxString retval;
  MathCell *tmp = this;
  bool firstline = true;

  while (tmp != NULL)
  {
    if ((!firstline) && (tmp->m_forceBreakLine))
    {
      if(!retval.EndsWith(wxT('\n')))
        retval += wxT("\n");
      // if(
      //    (tmp->GetStyle() != TS_LABEL) &&
      //    (tmp->GetStyle() != TS_USERLABEL) &&
      //    (tmp->GetStyle() != TS_MAIN_PROMPT) &&
      //    (tmp->GetStyle() != TS_OTHER_PROMPT))
      //   retval += wxT("\t");        
    }
    // if(firstline)
    // {
    //   if((tmp->GetStyle() != TS_LABEL) &&
    //      (tmp->GetStyle() != TS_USERLABEL) &&
    //      (tmp->GetStyle() != TS_MAIN_PROMPT) &&
    //      (tmp->GetStyle() != TS_OTHER_PROMPT))
    //     retval += wxT("\t");        
    // }
    retval += tmp->ToString();
    
    firstline = false;
    tmp = tmp->m_nextToDraw;
  }

  return retval;
}

wxString MathCell::ToTeX()
{
  return wxEmptyString;
}

wxString MathCell::ListToTeX()
{
  wxString retval;
  MathCell *tmp = this;

  while (tmp != NULL)
  {
    if ((tmp->m_textStyle == TS_LABEL && retval != wxEmptyString) ||
        (tmp->m_breakLine && retval != wxEmptyString))
      retval += wxT("\\]\\[");
    retval += tmp->ToTeX();
    tmp = tmp->m_next;
  }

  // TODO: Things like {a}_{b} make the LaTeX code harder to read. But things like
  // \sqrt{a} need us to use braces from time to time.
  //
  // How far I got was:
  //
  //  wxRegEx removeUnneededBraces1(wxT("{([a-zA-Z0-9])}([{}_a-zA-Z0-9 \\\\^_])"));
  //  removeUnneededBraces1.Replace(&retval,wxT(" \\1\\2"),true);
  return retval;
}

wxString MathCell::ToXML()
{
  return wxEmptyString;
}

wxString MathCell::ToMathML()
{
  return wxEmptyString;
}

wxString MathCell::ListToMathML(bool startofline)
{
  bool highlight = false;

  wxString retval;

  // If the region to export contains linebreaks or labels we put it into a table.
  bool needsTable = false;
  MathCell *temp = this;
  while (temp)
  {
    if (temp->ForceBreakLineHere())
      needsTable = true;

    if (temp->GetType() == MC_TYPE_LABEL)
      needsTable = true;

    temp = temp->m_next;
  }

  temp = this;
  // If the list contains multiple cells we wrap them in a <mrow> in order to
  // group them into a single object.
  bool multiCell = (temp->m_next != NULL);

  // Export all cells
  while (temp != NULL)
  {
    // Do we need to end a highlighting region?
    if ((!temp->m_highlight) && (highlight))
      retval += wxT("</mrow>");

    // Handle linebreaks
    if ((temp != this) && (temp->ForceBreakLineHere()))
      retval += wxT("</mtd></mlabeledtr>\n<mlabeledtr columnalign=\"left\"><mtd>");

    // If a linebreak isn't followed by a label we need to introduce an empty one.
    if ((((temp->ForceBreakLineHere()) || (startofline && (this == temp))) &&
         ((temp->GetStyle() != TS_LABEL) && (temp->GetStyle() != TS_USERLABEL))) && (needsTable))
      retval += wxT("<mtext></mtext></mtd><mtd>");

    // Do we need to start a highlighting region?
    if ((temp->m_highlight) && (!highlight))
      retval += wxT("<mrow mathcolor=\"red\">");
    highlight = temp->m_highlight;


    retval += temp->ToMathML();
    temp = temp->m_next;
  }

  // If the region we converted to MathML ended within a highlighted region
  // we need to close this region now.
  if (highlight)
    retval += wxT("</mrow>");

  // If we grouped multiple cells as a single object we need to cose this group now
  if ((multiCell) && (!needsTable))
    retval = wxT("<mrow>") + retval + wxT("</mrow>\n");

  // If we put the region we exported into a table we need to end this table now
  if (needsTable)
    retval = wxT("<mtable>\n<mlabeledtr columnalign=\"left\"><mtd>") + retval + wxT("</mtd></mlabeledtr>\n</mtable>");
  return retval;
}

wxString MathCell::OMML2RTF(wxXmlNode *node)
{
  wxString result;

  while (node != NULL)
  {
    if (node->GetType() == wxXML_ELEMENT_NODE)
    {
      wxString ommlname = node->GetName();
      result += wxT("{\\m") + ommlname.Right(ommlname.Length() - 2);

      // Convert the attributes
      wxXmlAttribute *attributes = node->GetAttributes();
      while (attributes != NULL)
      {
        wxString ommlatt = attributes->GetName();
        result += wxT("{\\m") + ommlatt.Right(ommlatt.Length() - 2) +
                  wxT(" ") + attributes->GetValue() + wxT("}");
        attributes = attributes->GetNext();
      }

      // Convert all child nodes
      if (node->GetChildren() != NULL)
      {
        result += OMML2RTF(node->GetChildren());
      }
      result += wxT("}");
    }
    else
      result += wxT(" ") + RTFescape(node->GetContent());

    node = node->GetNext();
  }
  return result;
}

wxString MathCell::OMML2RTF(wxString ommltext)
{
  if (ommltext == wxEmptyString)
    return wxEmptyString;

  wxString result;
  wxXmlDocument ommldoc;
  ommltext = wxT("<m:r>") + ommltext + wxT("</m:r>");

  wxStringInputStream ommlStream(ommltext);

  ommldoc.Load(ommlStream, wxT("UTF-8"));

  wxXmlNode *node = ommldoc.GetRoot();
  result += OMML2RTF(node);

  if ((result != wxEmptyString) && (result != wxT("\\mr")))
  {
    result = wxT("{\\mmath {\\*\\moMath") + result + wxT("}}");
  }
  return result;
}

wxString MathCell::XMLescape(wxString input)
{
  input.Replace(wxT("&"), wxT("&amp;"));
  input.Replace(wxT("<"), wxT("&lt;"));
  input.Replace(wxT(">"), wxT("&gt;"));
  input.Replace(wxT("'"), wxT("&apos;"));
  input.Replace(wxT("\""), wxT("&quot;"));
  return input;
}

wxString MathCell::RTFescape(wxString input, bool MarkDown)
{
  // Characters with a special meaning in RTF
  input.Replace("\\", "\\\\");
  input.Replace("{", "\\{");
  input.Replace("}", "\\}");

  // The Character we will use as a soft line break
  input.Replace("\r", wxEmptyString);

  // Encode unicode characters in a rather mind-boggling way
  wxString output;
  for (size_t i = 0; i < input.Length(); i++)
  {
    wxChar ch = input[i];
    if (ch == wxT('\n'))
    {
      if (((i > 0) && (input[i - 1] == wxT('\n'))) || !MarkDown)
        output += wxT("\\par}\n{\\pard ");
      else
        output += wxT("\n");
    }
    else
    {
      if ((ch < 128) && (ch > 0))
      {
        output += ch;
      }
      else
      {
        if (ch < 32768)
        {
          output += wxString::Format("\\u%i?", int(ch));
        }
        else
        {
          output += wxString::Format("\\u%i?", int(ch) - 65536);
        }
      }
    }
  }
  return (output);
}

wxString MathCell::ListToOMML(bool startofline)
{
  bool multiCell = (m_next != NULL);

  wxString retval;

  // If the region to export contains linebreaks or labels we put it into a table.
  // Export all cells

  MathCell *tmp = this;
  while (tmp != NULL)
  {
    wxString token = tmp->ToOMML();

    // End exporting the equation if we reached the end of the equation.
    if (token == wxEmptyString)
      break;

    retval += token;

    // Hard linebreaks aren't supported by OMML and therefore need a new equation object
    if (tmp->ForceBreakLineHere())
      break;

    tmp = tmp->m_next;
  }

  if ((multiCell) && (retval != wxEmptyString))
    return wxT("<m:r>") + retval + wxT("</m:r>");
  else
    return retval;
}

wxString MathCell::ListToRTF(bool startofline)
{
  wxString retval;
  MathCell *tmp = this;

  while (tmp != NULL)
  {
    wxString rtf = tmp->ToRTF();
    if (rtf != wxEmptyString)
    {
      if ((GetStyle() == TS_LABEL) || ((GetStyle() == TS_USERLABEL)))
      {
        retval += wxT("\\par}\n{\\pard\\s22\\li1105\\lin1105\\fi-1105\\f0\\fs24 ") + rtf + wxT("\\tab");
        startofline = false;
      }
      else
      {
        if (startofline)
          retval += wxT("\\par}\n{\\pard\\s21\\li1105\\lin1105\\f0\\fs24 ") + rtf + wxT("\\n");
        startofline = true;
      }
      tmp = tmp->m_next;
    }
    else
    {
      if (tmp->ListToOMML() != wxEmptyString)
      {
        // Math!

        // set the style for this line.
        if (startofline)
          retval += wxT("\\pard\\s21\\li1105\\lin1105\\f0\\fs24 ");

        retval += OMML2RTF(tmp->ListToOMML());

        startofline = true;

        // Skip the rest of this equation
        while (tmp != NULL)
        {
          // A non-equation item starts a new rtf item
          if (tmp->ToOMML() == wxEmptyString)
            break;

          // A newline starts a new equation
          if (tmp->ForceBreakLineHere())
          {
            tmp = tmp->m_next;
            break;
          }

          tmp = tmp->m_next;
        }
      }
      else
      {
        tmp = tmp->m_next;
      }
    }
  }
  return retval;
}

wxString MathCell::ListToXML()
{
  bool highlight = false;

  wxString retval;
  MathCell *tmp = this;

  while (tmp != NULL)
  {
    if ((tmp->GetHighlight()) && (!highlight))
    {
      retval += wxT("<hl>\n");
      highlight = true;
    }

    if ((!tmp->GetHighlight()) && (highlight))
    {
      retval += wxT("</hl>\n");
      highlight = false;
    }

    retval += tmp->ToXML();
    tmp = tmp->m_next;
  }

  if (highlight)
  {
    retval += wxT("</hl>\n");
  }

  return retval;
}

/***
 * Get the part for diff tag support - only ExpTag overvrides this.
 */
wxString MathCell::GetDiffPart()
{
  return wxEmptyString;
}

/***
 * Find the first and last cell in rectangle rect in this line.
 */
void MathCell::SelectRect(wxRect &rect, MathCell **first, MathCell **last)
{
  SelectFirst(rect, first);
  if (*first != NULL)
  {
    *last = *first;
    (*first)->SelectLast(rect, last);
    if (*last == *first)
      (*first)->SelectInner(rect, first, last);
  }
  else
    *last = NULL;
}

/***
 * Find the first cell in rectangle rect in this line.
 */
void MathCell::SelectFirst(wxRect &rect, MathCell **first)
{
  if (rect.Intersects(GetRect(false)))
    *first = this;
  else if (m_nextToDraw != NULL)
    m_nextToDraw->SelectFirst(rect, first);
  else
    *first = NULL;
}


void MathCell::MarkAsDeletedList(MathCell *list1,
                                 MathCell *list2,
                                 MathCell *list3,
                                 MathCell *list4,
                                 MathCell *list5,
                                 MathCell *list6,
                                 MathCell *list7
  )
{
  while(list1 != NULL)
  {
    list1->MarkAsDeleted();
    list1 = list1->m_next;
  }
  while(list2 != NULL)
  {
    list2->MarkAsDeleted();
    list2 = list2->m_next;
  }
  while(list3 != NULL)
  {
    list3->MarkAsDeleted();
    list3 = list3->m_next;
  }
  while(list4 != NULL)
  {
    list4->MarkAsDeleted();
    list4 = list4->m_next;
  }
  while(list5 != NULL)
  {
    list5->MarkAsDeleted();
    list5 = list5->m_next;
  }
  while(list6 != NULL)
  {
    list6->MarkAsDeleted();
    list6 = list6->m_next;
  }
  while(list7 != NULL)
  {
    list7->MarkAsDeleted();
    list7 = list7->m_next;
  }
}

wxString MathCell::GetToolTipList(const wxPoint &point,
                                  MathCell *list1,
                                  MathCell *list2,
                                  MathCell *list3,
                                  MathCell *list4,
                                  MathCell *list5,
                                  MathCell *list6,
                                  MathCell *list7
  )
{
  wxString toolTip;

  while(list1 != NULL)
  {
    toolTip = list1->GetToolTip(point);
    if(toolTip != wxEmptyString)
      return toolTip;
    list1 = list1->m_next;
  }
  while(list2 != NULL)
  {
    toolTip = list2->GetToolTip(point);
    if(toolTip != wxEmptyString)
      return toolTip;
    list2 = list2->m_next;
  }
  while(list3 != NULL)
  {
    toolTip = list3->GetToolTip(point);
    if(toolTip != wxEmptyString)
      return toolTip;
    list3 = list3->m_next;
  }
  while(list4 != NULL)
  {
    toolTip = list4->GetToolTip(point);
    if(toolTip != wxEmptyString)
      return toolTip;
    list4 = list4->m_next;
  }
  while(list5 != NULL)
  {
    toolTip = list5->GetToolTip(point);
    if(toolTip != wxEmptyString)
      return toolTip;
    list5 = list5->m_next;
  }
  while(list6 != NULL)
  {
    toolTip = list6->GetToolTip(point);
    if(toolTip != wxEmptyString)
      return toolTip;
    list6 = list6->m_next;
  }
  while(list7 != NULL)
  {
    toolTip = list7->GetToolTip(point);
    if(toolTip != wxEmptyString)
      return toolTip;
    list7 = list7->m_next;
  }
  return m_toolTip;
}


/***
 * Find the last cell in rectangle rect in this line.
 */
void MathCell::SelectLast(wxRect &rect, MathCell **last)
{
  if (rect.Intersects(GetRect(false)))
    *last = this;
  if (m_nextToDraw != NULL)
    m_nextToDraw->SelectLast(rect, last);
}

/***
 * Select rectangle in deeper cell - derived classes should override this
 */
void MathCell::SelectInner(wxRect &rect, MathCell **first, MathCell **last)
{
  *first = this;
  *last = this;
}

bool MathCell::BreakLineHere()
{
  return (((!m_isBroken) && m_breakLine) || m_forceBreakLine);
}

bool MathCell::ContainsRect(const wxRect &sm, bool all)
{
  wxRect big = GetRect(all);
  if (big.x <= sm.x &&
      big.y <= sm.y &&
      big.x + big.width >= sm.x + sm.width &&
      big.y + big.height >= sm.y + sm.height)
    return true;
  return false;
}

/*!
 Resets remembered data.

 Resets cached data like width and the height of the current cell
 as well as the vertical position of the center. Temporarily unbreaks all
 lines until the widths are recalculated if there aren't any hard line 
 breaks.
 */
void MathCell::ResetData()
{
  m_fullWidth = -1;
  m_lineWidth = -1;
  m_maxCenter = -1;
  m_maxDrop = -1;
  m_breakLine = m_forceBreakLine;
}

MathCell *MathCell::first()
{
  MathCell *tmp = this;
  while (tmp->m_previous)
    tmp = tmp->m_previous;

  return tmp;
}

MathCell *MathCell::last()
{
  MathCell *tmp = this;
  while (tmp->m_next)
    tmp = tmp->m_next;

  return tmp;
}

void MathCell::Unbreak()
{
  ResetData();
  m_isBroken = false;
  m_nextToDraw = m_next;
  if (m_nextToDraw != NULL)
    m_nextToDraw->m_previousToDraw = this;
}

void MathCell::UnbreakList()
{
  MathCell *tmp = this;
  while (tmp != NULL)
  {
    tmp->Unbreak();
    tmp = tmp->m_next;
  }
}

/*!
  Set the pen in device context according to the style of the cell.
*/
void MathCell::SetPen()
{
  Configuration *configuration = (*m_configuration);
  wxDC &dc = configuration->GetDC();

  if (m_highlight)
    dc.SetPen(*(wxThePenList->FindOrCreatePen(configuration->GetColor(TS_HIGHLIGHT),
                                              configuration->GetDefaultLineWidth(), wxPENSTYLE_SOLID)));
  else if (m_type == MC_TYPE_PROMPT)
    dc.SetPen(*(wxThePenList->FindOrCreatePen(configuration->GetColor(TS_OTHER_PROMPT),
                                              configuration->GetDefaultLineWidth(), wxPENSTYLE_SOLID)));
  else if (m_type == MC_TYPE_INPUT)
    dc.SetPen(*(wxThePenList->FindOrCreatePen(configuration->GetColor(TS_INPUT),
                                              configuration->GetDefaultLineWidth(), wxPENSTYLE_SOLID)));
  else
    dc.SetPen(*(wxThePenList->FindOrCreatePen(configuration->GetColor(TS_DEFAULT),
                                              configuration->GetDefaultLineWidth(), wxPENSTYLE_SOLID)));
}

/***
 * Reset the pen in the device context.
 */
void MathCell::UnsetPen()
{
  Configuration *configuration = (*m_configuration);
  wxDC &dc = configuration->GetDC();
  if (m_type == MC_TYPE_PROMPT || m_type == MC_TYPE_INPUT || m_highlight)
    dc.SetPen(*(wxThePenList->FindOrCreatePen(configuration->GetColor(TS_DEFAULT),
                                              1, wxPENSTYLE_SOLID)));
}

/***
 * Copy all important data from s to t
 */
void MathCell::CopyData(MathCell *s, MathCell *t)
{
  t->m_altCopyText = s->m_altCopyText;
  t->m_forceBreakLine = s->m_forceBreakLine;
  t->m_type = s->m_type;
  t->m_textStyle = s->m_textStyle;
}

void MathCell::SetForeground()
{
  Configuration *configuration = (*m_configuration);
  wxColour color;
  wxDC &dc = configuration->GetDC();
  if (m_highlight)
  {
    color = configuration->GetColor(TS_HIGHLIGHT);
  }
  else
  {
    switch (m_type)
    {
      case MC_TYPE_PROMPT:
        color = configuration->GetColor(TS_OTHER_PROMPT);
        break;
      case MC_TYPE_MAIN_PROMPT:
        color = configuration->GetColor(TS_MAIN_PROMPT);
        break;
      case MC_TYPE_ERROR:
        color = wxColour(wxT("red"));
        break;
      case MC_TYPE_WARNING:
        color = configuration->GetColor(TS_WARNING);
        break;
      case MC_TYPE_LABEL:
        color = configuration->GetColor(TS_LABEL);
        break;
      default:
        color = configuration->GetColor(m_textStyle);
        break;
    }
  }

  dc.SetTextForeground(color);
}

bool MathCell::IsMath()
{
  return !(m_textStyle == TS_DEFAULT ||
           m_textStyle == TS_LABEL ||
           m_textStyle == TS_INPUT);
}

wxRect MathCell::m_updateRegion;
bool   MathCell::m_clipToDrawRegion = true;
