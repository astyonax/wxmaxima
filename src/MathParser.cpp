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
  This file defines the class MathParser that reads wxmx data and math from Maxima.
*/

#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/sstream.h>
#include <wx/regex.h>
#include <wx/intl.h>

#include "MathParser.h"

#include "FracCell.h"
#include "ExptCell.h"
#include "TextCell.h"
#include "SubCell.h"
#include "SqrtCell.h"
#include "LimitCell.h"
#include "MatrCell.h"
#include "ParenCell.h"
#include "AbsCell.h"
#include "ConjugateCell.h"
#include "AtCell.h"
#include "DiffCell.h"
#include "SumCell.h"
#include "IntCell.h"
#include "FunCell.h"
#include "EditorCell.h"
#include "ImgCell.h"
#include "SubSupCell.h"
#include "SlideShowCell.h"
#include "GroupCell.h"

wxXmlNode *MathParser::SkipWhitespaceNode(wxXmlNode *node)
{
  if (node)
  {
    // If this is a text node there is a chance that this is a whitespace we want to skip
    if (node->GetType() == wxXML_TEXT_NODE)
    {
      // This is a text node => Let's see if it is whitespace-only and skip it if it is.
      wxString contents = node->GetContent();
      contents.Trim();
      if (contents.Length() <= 1)
        node = node->GetNext();
    }
  }
  return node;
}

wxXmlNode *MathParser::GetNextTag(wxXmlNode *node)
{
  if (node)
    node = node->GetNext();
  return SkipWhitespaceNode(node);
}

MathParser::MathParser(Configuration **cfg, CellPointers *cellPointers, wxString zipfile)
{
  m_configuration = cfg;
  m_cellPointers = cellPointers;
  m_ParserStyle = MC_TYPE_DEFAULT;
  m_FracStyle = FracCell::FC_NORMAL;
  m_highlight = false;
  if (zipfile.Length() > 0)
  {
    m_fileSystem = new wxFileSystem();
    m_fileSystem->ChangePathTo(zipfile + wxT("#zip:/"), true);
  }
  else
    m_fileSystem = NULL;
}

MathParser::~MathParser()
{
  if (m_fileSystem)
    wxDELETE(m_fileSystem);
}

// ParseCellTag
// This function is responsible for creating
// a tree of groupcells when loading XML document.
// Any changes in GroupCell structure or methods
// has to be reflected here in order to ensure proper
// loading of WXMX files.
MathCell *MathParser::ParseCellTag(wxXmlNode *node)
{
  GroupCell *group = NULL;

  // read hide status
  bool hide = (node->GetAttribute(wxT("hide"), wxT("false")) == wxT("true")) ? true : false;
  // read (group)cell type
  wxString type = node->GetAttribute(wxT("type"), wxT("text"));
  wxString sectioning_level = node->GetAttribute(wxT("sectioning_level"), wxT("0"));

  if (type == wxT("code"))
  {
    group = new GroupCell(m_configuration, GC_TYPE_CODE, m_cellPointers);
    wxString isAutoAnswer = node->GetAttribute(wxT("auto_answer"), wxT("no"));
    if(isAutoAnswer == wxT("yes"))
      group->AutoAnswer(true);
    int i = 1; wxString answer;
    while (node->GetAttribute(wxString::Format(wxT("answer%i"),i),&answer))
    {
      group->AddAnswer(answer);
      i++;
    }
    wxXmlNode *children = node->GetChildren();
    children = SkipWhitespaceNode(children);
    while (children)
    {
      if (children->GetName() == wxT("input"))
      {
        MathCell *editor = ParseTag(children->GetChildren());
        if (editor == NULL)
          editor = new EditorCell(group, m_configuration, m_cellPointers, _("Bug: Missing contents"));
        group->SetEditableContent(editor->GetValue());
        wxDELETE(editor);
      }
      if (children->GetName() == wxT("output"))
      {
        group->AppendOutput(HandleNullPointer(ParseTag(children->GetChildren())));
      }
      children = GetNextTag(children);
    }
  }
  else if (type == wxT("image"))
  {
    group = new GroupCell(m_configuration, GC_TYPE_IMAGE, m_cellPointers);
    wxXmlNode *children = node->GetChildren();
    children = SkipWhitespaceNode(children);
    while (children)
    {
      if (children->GetName() == wxT("editor"))
      {
        MathCell *ed = ParseEditorTag(children);
        group->SetEditableContent(ed->GetValue());
        wxDELETE(ed);
      }
      else
        group->AppendOutput(ParseTag(children));
      children = GetNextTag(children);
    }
  }
  else if (type == wxT("pagebreak"))
  {
    group = new GroupCell(m_configuration, GC_TYPE_PAGEBREAK, m_cellPointers);
  }
  else if (type == wxT("text"))
  {
    group = new GroupCell(m_configuration, GC_TYPE_TEXT, m_cellPointers);
    MathCell *editor = ParseTag(node->GetChildren());
    if (editor == NULL)
      editor = new EditorCell(group, m_configuration, m_cellPointers, _("Bug: Missing contents"));
    group->SetEditableContent(editor->GetValue());
    wxDELETE(editor);
  }
  else
  {
    // text types
    if (type == wxT("title"))
      group = new GroupCell(m_configuration, GC_TYPE_TITLE, m_cellPointers);
    else if (type == wxT("section"))
      group = new GroupCell(m_configuration, GC_TYPE_SECTION, m_cellPointers);
    else if (type == wxT("subsection"))
    {
      // We save subsubsections as subsections with a higher sectioning level:
      // This makes them backwards-compatible in the way that they are displayed
      // as subsections on old wxMaxima installations.
      // A sectioning level of the value 0 means that the file is too old to
      // provide a sectioning level.
      if (sectioning_level != wxT("4"))
        group = new GroupCell(m_configuration, GC_TYPE_SUBSECTION, m_cellPointers);
      else
        group = new GroupCell(m_configuration, GC_TYPE_SUBSUBSECTION, m_cellPointers);
    }
    else if (type == wxT("subsubsection"))
    {
      group = new GroupCell(m_configuration, GC_TYPE_SUBSUBSECTION, m_cellPointers);
    }
    else
      return NULL;

    wxXmlNode *children = node->GetChildren();
    children = SkipWhitespaceNode(children);
    while (children)
    {
      if (children->GetName() == wxT("editor"))
      {
        MathCell *ed = ParseEditorTag(children);
        group->SetEditableContent(ed->GetValue());
        wxDELETE(ed);
      }
      else if (children->GetName() == wxT("fold"))
      { // we have folded groupcells
        wxXmlNode *xmlcells = children->GetChildren();
        xmlcells = SkipWhitespaceNode(xmlcells);
        MathCell *tree = NULL;
        MathCell *last = NULL;
        while (xmlcells)
        {
          MathCell *cell = ParseTag(xmlcells, false);

          if (cell == NULL)
            continue;

          if (tree == NULL) tree = cell;

          if (last == NULL) last = cell;
          else
          {
            last->m_next = last->m_nextToDraw = cell;
            last->m_next->m_previous = last->m_next->m_previousToDraw = last;

            last = last->m_next;
          }
          xmlcells = GetNextTag(xmlcells);
        }
        if (tree)
          group->HideTree(dynamic_cast<GroupCell *>(tree));
      }
      children = GetNextTag(children);
    }
  }

  group->SetParent(group);
  group->Hide(hide);
  return group;
}

MathCell *MathParser::HandleNullPointer(MathCell *cell)
{
  if (cell == NULL)
    cell = new TextCell(NULL, m_configuration, _("Bug: Missing contents"));
  return (cell);
}

MathCell *MathParser::ParseEditorTag(wxXmlNode *node)
{
  EditorCell *editor = new EditorCell(NULL, m_configuration, m_cellPointers);
  wxString type = node->GetAttribute(wxT("type"), wxT("input"));
  if (type == wxT("input"))
    editor->SetType(MC_TYPE_INPUT);
  else if (type == wxT("text"))
    editor->SetType(MC_TYPE_TEXT);
  else if (type == wxT("title"))
    editor->SetType(MC_TYPE_TITLE);
  else if (type == wxT("section"))
    editor->SetType(MC_TYPE_SECTION);
  else if (type == wxT("subsection"))
    editor->SetType(MC_TYPE_SUBSECTION);
  else if (type == wxT("subsubsection"))
    editor->SetType(MC_TYPE_SUBSUBSECTION);

  wxString text = wxEmptyString;
  wxXmlNode *line = node->GetChildren();
  while (line)
  {
    if (line->GetName() == wxT("line"))
    {
      if (!text.IsEmpty())
        text += wxT("\n");
#if wxUSE_UNICODE
      text += line->GetNodeContent();
#else
      wxString str = line->GetNodeContent();
      wxString str1(str.wc_str(wxConvUTF8), *wxConvCurrent);
      text += str1;
#endif
    }
    line = line->GetNext();
  } // end while
  editor->SetValue(text);
  return editor;
}

MathCell *MathParser::ParseFracTag(wxXmlNode *node)
{
  FracCell *frac = new FracCell(NULL, m_configuration);
  frac->SetFracStyle(m_FracStyle);
  frac->SetHighlight(m_highlight);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  frac->SetNum(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  frac->SetDenom(HandleNullPointer(ParseTag(child, false)));
  
  if (node->GetAttribute(wxT("line")) == wxT("no"))
    frac->SetFracStyle(FracCell::FC_CHOOSE);
  if (node->GetAttribute(wxT("diffstyle")) == wxT("yes"))
    frac->SetFracStyle(FracCell::FC_DIFF);
  frac->SetType(m_ParserStyle);
  frac->SetStyle(TS_VARIABLE);
  frac->SetupBreakUps();
  return frac;
}

MathCell *MathParser::ParseDiffTag(wxXmlNode *node)
{
  DiffCell *diff = new DiffCell(NULL, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  if (child)
  {
    int fc = m_FracStyle;
    m_FracStyle = FracCell::FC_DIFF;

    diff->SetDiff(HandleNullPointer(ParseTag(child, false)));
    m_FracStyle = fc;
    child = GetNextTag(child);

    diff->SetBase(HandleNullPointer(ParseTag(child, true)));
    diff->SetType(m_ParserStyle);
    diff->SetStyle(TS_VARIABLE);
  }
  return diff;
}

MathCell *MathParser::ParseSupTag(wxXmlNode *node)
{
  ExptCell *expt = new ExptCell(NULL, m_configuration);
  if (node->GetAttributes() != NULL)
    expt->IsMatrix(true);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);

  expt->SetBase(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);

  MathCell *power = HandleNullPointer(ParseTag(child, false));
  power->SetExponentFlag();
  expt->SetPower(power);
  expt->SetType(m_ParserStyle);
  expt->SetStyle(TS_VARIABLE);
  return expt;
}

MathCell *MathParser::ParseSubSupTag(wxXmlNode *node)
{
  SubSupCell *subsup = new SubSupCell(NULL, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  subsup->SetBase(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  MathCell *index = HandleNullPointer(ParseTag(child, false));
  index->SetExponentFlag();
  subsup->SetIndex(index);
  child = GetNextTag(child);
  MathCell *power = HandleNullPointer(ParseTag(child, false));
  power->SetExponentFlag();
  subsup->SetExponent(power);
  subsup->SetType(m_ParserStyle);
  subsup->SetStyle(TS_VARIABLE);
  return subsup;
}

MathCell *MathParser::ParseSubTag(wxXmlNode *node)
{
  SubCell *sub = new SubCell(NULL, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  sub->SetBase(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  MathCell *index = HandleNullPointer(ParseTag(child, false));
  sub->SetIndex(index);
  index->SetExponentFlag();
  sub->SetType(m_ParserStyle);
  sub->SetStyle(TS_VARIABLE);

  return sub;
}

MathCell *MathParser::ParseAtTag(wxXmlNode *node)
{
  AtCell *at = new AtCell(NULL, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);

  at->SetBase(HandleNullPointer(ParseTag(child, false)));
  at->SetHighlight(m_highlight);
  child = GetNextTag(child);
  at->SetIndex(HandleNullPointer(ParseTag(child, false)));
  at->SetType(m_ParserStyle);
  at->SetStyle(TS_VARIABLE);
  return at;
}

MathCell *MathParser::ParseFunTag(wxXmlNode *node)
{
  FunCell *fun = new FunCell(NULL, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);

  fun->SetName(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  fun->SetType(m_ParserStyle);
  fun->SetStyle(TS_VARIABLE);
  fun->SetArg(HandleNullPointer(ParseTag(child, false)));
  return fun;
}

MathCell *MathParser::ParseText(wxXmlNode *node, int style)
{
  wxString str;
  TextCell *retval = NULL;
  if ((node != NULL) && ((str = node->GetContent()) != wxEmptyString))
  {
#if !wxUSE_UNICODE
    wxString str1(str.wc_str(wxConvUTF8), *wxConvCurrent);
    str = str1;
#endif
#if wxUSE_UNICODE
    str.Replace(wxT("-"), wxT("\x2212")); // unicode minus sign
#endif

    wxStringTokenizer lines(str, wxT('\n'));
    while (lines.HasMoreTokens())
    {
      TextCell *cell = new TextCell(NULL, m_configuration);
      if (style != TS_ERROR)
        cell->SetType(m_ParserStyle);
      else
        cell->SetType(MC_TYPE_ERROR);
      cell->SetStyle(style);
      cell->SetHighlight(m_highlight);
      cell->SetValue(lines.GetNextToken());
      if (retval == NULL)
        retval = cell;
      else
      {
        cell->ForceBreakLine(true);
        retval->AppendCell(cell);
      };
    }
  }

  if (retval == NULL)
    retval = new TextCell(NULL, m_configuration);

  wxString breaklineattrib;
  if (node != NULL)
    breaklineattrib = node->GetAttribute(wxT("breakline"), wxT("false"));

  if (breaklineattrib == wxT("true"))
  {
    if (retval)
    {
      retval->ForceBreakLine(true);
    }
  }
  return retval;
}

MathCell *MathParser::ParseCharCode(wxXmlNode *node, int style)
{
  TextCell *cell = new TextCell(NULL, m_configuration);
  wxString str;
  if ((node != NULL) && ((str = node->GetContent()) != wxEmptyString))
  {
    long code;
    if (str.ToLong(&code))
      str = wxString::Format(wxT("%c"), code);
#if !wxUSE_UNICODE
    wxString str1(str.wc_str(wxConvUTF8), *wxConvCurrent);
    str = str1;
#endif
    cell->SetValue(str);
    cell->SetType(m_ParserStyle);
    cell->SetStyle(style);
    cell->SetHighlight(m_highlight);
  }
  return cell;
}

MathCell *MathParser::ParseSqrtTag(wxXmlNode *node)
{
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);

  SqrtCell *cell = new SqrtCell(NULL, m_configuration);

  cell->SetInner(HandleNullPointer(ParseTag(child, true)));
  cell->SetType(m_ParserStyle);
  cell->SetStyle(TS_VARIABLE);
  cell->SetHighlight(m_highlight);
  return cell;
}

MathCell *MathParser::ParseAbsTag(wxXmlNode *node)
{
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  AbsCell *cell = new AbsCell(NULL, m_configuration);
  cell->SetInner(HandleNullPointer(ParseTag(child, true)));
  cell->SetType(m_ParserStyle);
  cell->SetStyle(TS_VARIABLE);
  cell->SetHighlight(m_highlight);
  return cell;
}

MathCell *MathParser::ParseConjugateTag(wxXmlNode *node)
{
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  ConjugateCell *cell = new ConjugateCell(NULL, m_configuration);
  cell->SetInner(HandleNullPointer(ParseTag(child, true)));
  cell->SetType(m_ParserStyle);
  cell->SetStyle(TS_VARIABLE);
  cell->SetHighlight(m_highlight);
  return cell;
}

MathCell *MathParser::ParseParenTag(wxXmlNode *node)
{
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  ParenCell *cell = new ParenCell(NULL, m_configuration);
  cell->SetInner(HandleNullPointer(ParseTag(child, true)), m_ParserStyle);
  cell->SetHighlight(m_highlight);
  cell->SetStyle(TS_VARIABLE);
  if (node->GetAttributes() != NULL)
    cell->SetPrint(false);
  return cell;
}

MathCell *MathParser::ParseLimitTag(wxXmlNode *node)
{
  LimitCell *limit = new LimitCell(NULL, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  limit->SetName(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  limit->SetUnder(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  limit->SetBase(HandleNullPointer(ParseTag(child, false)));
  limit->SetType(m_ParserStyle);
  limit->SetStyle(TS_VARIABLE);
  return limit;
}

MathCell *MathParser::ParseSumTag(wxXmlNode *node)
{
  SumCell *sum = new SumCell(NULL, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  wxString type = node->GetAttribute(wxT("type"), wxT("sum"));

  if (type == wxT("prod"))
    sum->SetSumStyle(SM_PROD);
  sum->SetHighlight(m_highlight);
  sum->SetUnder(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  if (type != wxT("lsum"))
    sum->SetOver(HandleNullPointer(ParseTag(child, false)));
  child = GetNextTag(child);
  sum->SetBase(HandleNullPointer(ParseTag(child, false)));
  sum->SetType(m_ParserStyle);
  sum->SetStyle(TS_VARIABLE);
  return sum;
}

MathCell *MathParser::ParseIntTag(wxXmlNode *node)
{
  IntCell *in = new IntCell(NULL, m_configuration);
  wxXmlNode *child = node->GetChildren();
  child = SkipWhitespaceNode(child);
  in->SetHighlight(m_highlight);
  wxString definiteAtt = node->GetAttribute(wxT("def"), wxT("true"));
  if (definiteAtt != wxT("true"))
  {
    in->SetBase(HandleNullPointer(ParseTag(child, false)));
    child = GetNextTag(child);
    in->SetVar(HandleNullPointer(ParseTag(child, true)));
    in->SetType(m_ParserStyle);
    in->SetStyle(TS_VARIABLE);
  }
  else
  {
    // A Definite integral
    in->SetIntStyle(IntCell::INT_DEF);
    in->SetUnder(HandleNullPointer(ParseTag(child, false)));
    child = GetNextTag(child);
    in->SetOver(HandleNullPointer(ParseTag(child, false)));
    child = GetNextTag(child);
    in->SetBase(HandleNullPointer(ParseTag(child, false)));
    child = GetNextTag(child);
    in->SetVar(HandleNullPointer(ParseTag(child, true)));
    in->SetType(m_ParserStyle);
    in->SetStyle(TS_VARIABLE);
  }
  return in;
}

MathCell *MathParser::ParseTableTag(wxXmlNode *node)
{
  MatrCell *matrix = new MatrCell(NULL, m_configuration);
  matrix->SetHighlight(m_highlight);

  if (node->GetAttribute(wxT("special"), wxT("false")) == wxT("true"))
    matrix->SetSpecialFlag(true);
  if (node->GetAttribute(wxT("inference"), wxT("false")) == wxT("true"))
  {
    matrix->SetInferenceFlag(true);
    matrix->SetSpecialFlag(true);
  }
  if (node->GetAttribute(wxT("colnames"), wxT("false")) == wxT("true"))
    matrix->ColNames(true);
  if (node->GetAttribute(wxT("rownames"), wxT("false")) == wxT("true"))
    matrix->RowNames(true);

  wxXmlNode *rows = SkipWhitespaceNode(node->GetChildren());
  while (rows)
  {
    matrix->NewRow();
    wxXmlNode *cells = SkipWhitespaceNode(rows->GetChildren());
    while (cells)
    {
      matrix->NewColumn();
      matrix->AddNewCell(HandleNullPointer(ParseTag(cells, false)));
      cells = GetNextTag(cells);
    }
    rows = rows->GetNext();
  }
  matrix->SetType(m_ParserStyle);
  matrix->SetStyle(TS_VARIABLE);
  matrix->SetDimension();
  return matrix;
}

MathCell *MathParser::ParseTag(wxXmlNode *node, bool all)
{
  //  wxYield();
  MathCell *retval = NULL;
  MathCell *cell = NULL;
  bool warning = all;
  wxString altCopy;

  node = SkipWhitespaceNode(node);

  while (node)
  {
    if (node->GetType() == wxXML_ELEMENT_NODE)
    {
      // Parse XML tags. The only other type of element we recognize are text
      // nodes.
      wxString tagName(node->GetName());

      MathCell *tmp = NULL;
      if (tagName == wxT("v"))
      {               // Variables (atoms)
        tmp = ParseText(node->GetChildren(), TS_VARIABLE);
      }
      else if (tagName == wxT("t"))
      {          // Other text
        TextStyle style = TS_DEFAULT;
        if (node->GetAttribute(wxT("type")) == wxT("error"))
          style = TS_ERROR;
        tmp = ParseText(node->GetChildren(), style);
      }
      else if (tagName == wxT("n"))
      {          // Numbers
        tmp = ParseText(node->GetChildren(), TS_NUMBER);
      }
      else if (tagName == wxT("h"))
      {          // Hidden cells (*)
        tmp = ParseText(node->GetChildren());
        tmp->m_isHidden = true;
      }
      else if (tagName == wxT("p"))
      {          // Parenthesis
        tmp = ParseParenTag(node);
      }
      else if (tagName == wxT("f"))
      {               // Fractions
        tmp = ParseFracTag(node);
      }
      else if (tagName == wxT("e"))
      {          // Exponentials
        tmp = ParseSupTag(node);
      }
      else if (tagName == wxT("i"))
      {          // Subscripts
        tmp = ParseSubTag(node);
      }
      else if (tagName == wxT("fn"))
      {         // Functions
        tmp = ParseFunTag(node);
      }
      else if (tagName == wxT("g"))
      {          // Greek constants
        tmp = ParseText(node->GetChildren(), TS_GREEK_CONSTANT);
      }
      else if (tagName == wxT("s"))
      {          // Special constants %e,...
        tmp = ParseText(node->GetChildren(), TS_SPECIAL_CONSTANT);
      }
      else if (tagName == wxT("fnm"))
      {         // Function names
        tmp = ParseText(node->GetChildren(), TS_FUNCTION);
      }
      else if (tagName == wxT("q"))
      {          // Square roots
        tmp = ParseSqrtTag(node);
      }
      else if (tagName == wxT("d"))
      {          // Differentials
        tmp = ParseDiffTag(node);
      }
      else if (tagName == wxT("sm"))
      {         // Sums
        tmp = ParseSumTag(node);
      }
      else if (tagName == wxT("in"))
      {         // integrals
        tmp = ParseIntTag(node);
      }
      else if (tagName == wxT("mspace"))
      {
        tmp = new TextCell(NULL, m_configuration, wxT(" "));
      }
      else if (tagName == wxT("at"))
      {
        tmp = ParseAtTag(node);
      }
      else if (tagName == wxT("a"))
      {
        tmp = ParseAbsTag(node);
      }
      else if (tagName == wxT("cj"))
      {
        tmp = ParseConjugateTag(node);
      }
      else if (tagName == wxT("ie"))
      {
        tmp = ParseSubSupTag(node);
      }
      else if (tagName == wxT("lm"))
      { // A limit tag
        tmp = ParseLimitTag(node);
      }
      else if (tagName == wxT("r"))
      { // A group of tags
        tmp = ParseTag(node->GetChildren());
      }
      else if (tagName == wxT("tb"))
      {
        tmp = ParseTableTag(node);
      }
      else if ((tagName == wxT("mth")) || (tagName == wxT("line")))
      {
        tmp = ParseTag(node->GetChildren());
        if (tmp != NULL)
          tmp->ForceBreakLine(true);
        else
          tmp = new TextCell(NULL, m_configuration, wxT(" "));
      }
      else if (tagName == wxT("lbl"))
      {
        wxString user_lbl = node->GetAttribute(wxT("userdefinedlabel"), m_userDefinedLabel);
        wxString userdefined = node->GetAttribute(wxT("userdefined"), wxT("no"));
        
        if ( userdefined != wxT("yes"))
        {
          tmp = ParseText(node->GetChildren(), TS_LABEL);
        }
        else
        {
          tmp = ParseText(node->GetChildren(), TS_USERLABEL);

          // Backwards compatibility to 17.04/17.12:
          // If we cannot find the user-defined label's text but still know that there
          // is one it's value has been saved as "automatic label" instead.
          if(user_lbl == wxEmptyString)
          {
            user_lbl = dynamic_cast<TextCell *>(tmp)->GetValue();
            user_lbl = user_lbl.substr(1,user_lbl.Length() - 2);
          }
        }

        dynamic_cast<TextCell *>(tmp)->SetUserDefinedLabel(user_lbl);
        tmp->ForceBreakLine(true);
      }
      else if (tagName == wxT("st"))
      {
        tmp = ParseText(node->GetChildren(), TS_STRING);
      }
      else if (tagName == wxT("hl"))
      {
        bool highlight = m_highlight;
        m_highlight = true;
        tmp = ParseTag(node->GetChildren());
        m_highlight = highlight;
      }
      else if (tagName == wxT("img"))
      {
        ImgCell *imageCell;
        wxString filename(node->GetChildren()->GetContent());
#if !wxUSE_UNICODE
        wxString filename1(filename.wc_str(wxConvUTF8), *wxConvCurrent);
        filename = filename1;
#endif

        if (m_fileSystem) // loading from zip
          imageCell = new ImgCell(NULL, m_configuration, filename, false, m_fileSystem);
        else
        {
          if (node->GetAttribute(wxT("del"), wxT("yes")) != wxT("no"))
            imageCell = new ImgCell(NULL, m_configuration, filename, true, NULL);
          else
          {
            // This is the only case show_image() produces ergo this is the only
            // case we might get a local path

            if (
                    (!wxFileExists(filename)) &&
                    (wxFileExists((*m_configuration)->GetWorkingDirectory() + wxT("/") + filename))
                    )
              filename = (*m_configuration)->GetWorkingDirectory() + wxT("/") + filename;

            imageCell = new ImgCell(NULL, m_configuration, filename, false, NULL);
          }
        }

        if (node->GetAttribute(wxT("rect"), wxT("true")) == wxT("false"))
          imageCell->DrawRectangle(false);

        tmp = imageCell;
      }
      else if (tagName == wxT("slide"))
      {
        SlideShow *slideShow = new SlideShow(NULL, m_configuration, m_fileSystem);
        wxString str(node->GetChildren()->GetContent());
        wxArrayString images;
        wxString framerate;
        wxStringTokenizer tokens(str, wxT(";"));
        if (node->GetAttribute(wxT("fr"), &framerate))
        {
          long fr;
          if (framerate.ToLong(&fr))
            slideShow->SetFrameRate(fr);
        }
        while (tokens.HasMoreTokens())
        {
          wxString token = tokens.GetNextToken();
          if (token.Length())
          {
#if !wxUSE_UNICODE
            wxString token1(token.wc_str(wxConvUTF8), *wxConvCurrent);
            token = token1;
#endif
            images.Add(token);
          }
        }
        if (slideShow)
          slideShow->LoadImages(images);
        tmp = slideShow;
      }
      else if (tagName == wxT("editor"))
      {
        tmp = ParseEditorTag(node);
      }
      else if (tagName == wxT("cell"))
      {
        tmp = ParseCellTag(node);
      }
      else if (tagName == wxT("ascii"))
      {
        tmp = ParseCharCode(node->GetChildren());
      }
      else if (node->GetChildren())
      {
        tmp = ParseTag(node->GetChildren());
      }

      // The new cell may needing being equipped with a "altCopy" tag.
      if ((tmp != NULL) && (node->GetAttribute(wxT("altCopy"), &altCopy)))
        tmp->SetAltCopyText(altCopy);

      // Append the cell we found (tmp) to the list of cells we parsed so far (cell).
      if (tmp != NULL)
      {
        if (node->GetAttribute(wxT("breakline"), wxT("false")) == wxT("true"))
          tmp->ForceBreakLine(true);
        if (cell == NULL)
          cell = tmp;
        else
          cell->AppendCell(tmp);
      }
    }
    else
    {
      // We didn't get a tag but got a text cell => Parse the text.
      if (cell == NULL)
        cell = ParseText(node);
      else
        cell->AppendCell(ParseText(node));
    }

    if (cell != NULL)
    {
      // Append the new cell to the return value
      if (retval == NULL)
        retval = cell;
      else
        cell = cell->m_next;
    }
    else if ((warning) && (!all))
    {
      // Tell the user we ran into problems.
      wxString name;
      name.Trim(true);
      name.Trim(false);
      if (cell != NULL) name = cell->ToString();
      if (name.Length() != 0)
      {
        wxMessageBox(_("Parts of the document will not be loaded correctly:\nFound unknown XML Tag name " + name),
                     _("Warning"),
                     wxOK | wxICON_WARNING);
        warning = false;
      }
    }

    if (!all)
      break;

    node = GetNextTag(node);
  }

  return retval;
}

/***
 * Parse the string s, which is (correct) xml fragment.
 * Put the result in line.
 */
MathCell *MathParser::ParseLine(wxString s, int style)
{
  m_ParserStyle = style;
  m_FracStyle = FracCell::FC_NORMAL;
  m_highlight = false;
  MathCell *cell = NULL;

  wxConfigBase *config = wxConfig::Get();
  int showLength = 0;
  config->Read(wxT("showLength"), &showLength);

  switch (showLength)
  {
    case 0:
      showLength = 50000;
      break;
    case 1:
      showLength = 500000;
      break;
    case 2:
      showLength = 5000000;
      break;
    case 3:
      showLength = 0;
      break;
  }
  wxRegEx graph(wxT("[[:cntrl:]]"));

#if wxUSE_UNICODE
  graph.Replace(&s, wxT("\xFFFD"));
#else
  graph.Replace(&s, wxT("?"));
#endif

  if (((long) s.Length() < showLength) || (showLength == 0))
  {

    wxXmlDocument xml;

#if wxUSE_UNICODE
    wxStringInputStream xmlStream(s);
#else
    wxString su(s.wc_str(*wxConvCurrent), wxConvUTF8);
    wxStringInputStream xmlStream(su);
#endif

    xml.Load(xmlStream, wxT("UTF-8"), wxXMLDOC_KEEP_WHITESPACE_NODES);

    wxXmlNode *doc = xml.GetRoot();

    if (doc != NULL)
      cell = ParseTag(doc->GetChildren());
  }
  else
  {
    cell = new TextCell(NULL, m_configuration,
                        _(" << Expression longer than allowed by the configuration setting! >>"));
    cell->ForceBreakLine(true);
  }
  return cell;
}
