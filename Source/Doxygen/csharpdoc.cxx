/* -----------------------------------------------------------------------------
 * This file is part of SWIG, which is licensed as a whole under version 3
 * (or any later version) of the GNU General Public License. Some additional
 * terms also apply to certain portions of SWIG. The full details of the SWIG
 * license and copyrights can be found in the LICENSE and COPYRIGHT files
 * included with the SWIG source code as distributed by the SWIG developers
 * and at http://www.swig.org/legal.html.
 *
 * csharpdoc.cxx
 * ----------------------------------------------------------------------------- */

#include "csharpdoc.h"
#include "doxyparser.h"
#include <iostream>
#include <vector>
#include <list>
#include "swigmod.h"
#define APPROX_LINE_LENGTH 64   // characters per line allowed
#define TAB_SIZE 8              // current tab size in spaces
//TODO {@link} {@linkplain} {@docRoot}, and other useful doxy commands that are not a javadoc tag

// define static tables, they are filled in CSharpDocConverter's constructor
std::map<std::string, std::pair<CSharpDocConverter::tagHandler, std::string> > CSharpDocConverter::tagHandlers;

using std::string;
using std::list;
using std::vector;

void CSharpDocConverter::fillStaticTables() {
  if (tagHandlers.size()) // fill only once
    return;

  /*
   * Some translation rules:
   *
   * @ and \ must be escaped for both Java and Python to appear on output: \@, \\,
   *         while Doxygen produces output in both cases.
   * Rule:   @ and \ with space on the right should get to output.
   *
   * :: remains intact, even in class::method(). But you can use class#method also
   *    in C++ comment and it is properly translated to C++ output (changed by doxygen to ::)
   *    and Java output (remains #).
   * Rule: SWIG type system can't be used to convert C::m to C#m, because in Java it is C.m
   *       Use string replacement :: --> # in tag see and links.
   *
   * HTML tags must be translated - remain in Java, to markdown in Python
   *
   * Unknown HTML tags, for example <x> is translated to &lt;x&gt; by doxygen, while
   *     Java src is <x> and therefore invisible on output - browser ignores unknown command.
   *     This is handy in syntax descriptions, for example: more <fileName>.
   *
   * Standalone < and > need not be translated, they are rendered properly in
   *      all three outputs.
   *
   * ., %, and " need not to be translated
   *
   * entities must be translated - remain in Java, something meaningful in Python (&lt, ...)
   *
   * - Python
   * - add comments also to auto-generated methods like equals(), delete() in Java,
   *   and methods for std::vector(), ...
   *   Commenting methods of std types is simple - add comment to std_*.i file.
   */

  // these commands insert HTML tags
  tagHandlers["a"] = make_pair(&CSharpDocConverter::handleTagHtml, "i");
  tagHandlers["arg"] = make_pair(&CSharpDocConverter::handleTagHtml, "li");
  tagHandlers["b"] = make_pair(&CSharpDocConverter::handleTagHtml, "b");
  tagHandlers["c"] = make_pair(&CSharpDocConverter::handleTagHtml, "code");
  tagHandlers["cite"] = make_pair(&CSharpDocConverter::handleTagHtml, "i");
  tagHandlers["e"] = make_pair(&CSharpDocConverter::handleTagHtml, "i");
  tagHandlers["em"] = make_pair(&CSharpDocConverter::handleTagHtml, "i");
  tagHandlers["li"] = make_pair(&CSharpDocConverter::handleTagHtml, "li");
  tagHandlers["p"] = make_pair(&CSharpDocConverter::handleTagHtml, "code");
  // these commands insert just a single char, some of them need to be escaped
  tagHandlers["$"] = make_pair(&CSharpDocConverter::handleTagChar, "");
  tagHandlers["@"] = make_pair(&CSharpDocConverter::handleTagChar, "");
  tagHandlers["\\"] = make_pair(&CSharpDocConverter::handleTagChar, "");
  tagHandlers["<"] = make_pair(&CSharpDocConverter::handleTagChar, "&lt;");
  tagHandlers[">"] = make_pair(&CSharpDocConverter::handleTagChar, "&gt;");
  tagHandlers["&"] = make_pair(&CSharpDocConverter::handleTagChar, "&amp;");
  tagHandlers["#"] = make_pair(&CSharpDocConverter::handleTagChar, "");
  tagHandlers["%"] = make_pair(&CSharpDocConverter::handleTagChar, "");
  tagHandlers["~"] = make_pair(&CSharpDocConverter::handleTagChar, "");
  tagHandlers["\""] = make_pair(&CSharpDocConverter::handleTagChar, "&quot;");
  tagHandlers["."] = make_pair(&CSharpDocConverter::handleTagChar, "");
  // :: is used in C++ to separate namespaces
  tagHandlers["::"] = make_pair(&CSharpDocConverter::handleTagChar, ".");
  //// these commands are stripped out
  //tagHandlers["attention"] = make_pair(&CSharpDocConverter::handleParagraph, "");
  //tagHandlers["anchor"] = make_pair(&CSharpDocConverter::handleTagAnchor, "");
  //tagHandlers["bug"] = make_pair(&CSharpDocConverter::handleParagraph, "");
  //tagHandlers["date"] = make_pair(&CSharpDocConverter::handleParagraph, "");
  //tagHandlers["details"] = make_pair(&CSharpDocConverter::handleParagraph, "");
  //// this command is inserts text accumulated after cmd htmlonly -
  //// see DoxygenParser - CMD_HTML_ONLY.
  //tagHandlers["htmlonly"] = make_pair(&CSharpDocConverter::handleParagraph, "");
  //tagHandlers["invariant"] = make_pair(&CSharpDocConverter::handleParagraph, "");
  //tagHandlers["latexonly"] = make_pair(&CSharpDocConverter::handleParagraph, "");
  //tagHandlers["manonly"] = make_pair(&CSharpDocConverter::handleParagraph, "");

  //tagHandlers["rtfonly"] = make_pair(&CSharpDocConverter::handleParagraph, "");
  //tagHandlers["short"] = make_pair(&CSharpDocConverter::handleParagraph, "");
  //tagHandlers["xmlonly"] = make_pair(&CSharpDocConverter::handleParagraph, "");
  //// these commands are kept as-is, they are supported by CSharpDoc
  tagHandlers["author"] = make_pair(&CSharpDocConverter::handleTagSame, "");
  tagHandlers["authors"] = make_pair(&CSharpDocConverter::handleTagSame, "author");
  tagHandlers["deprecated"] = make_pair(&CSharpDocConverter::handleTagSame, "");
  //tagHandlers["exception"] = make_pair(&CSharpDocConverter::handleTagSame, "");
  //tagHandlers["package"] = make_pair(&CSharpDocConverter::handleTagSame, "");
  tagHandlers["param"] = make_pair(&CSharpDocConverter::handleTagParam, "");
  //tagHandlers["tparam"] = make_pair(&CSharpDocConverter::handleTagParam, "");
  tagHandlers["ref"] = make_pair(&CSharpDocConverter::handleTagRef, "");
  tagHandlers["result"] = make_pair(&CSharpDocConverter::handleTagSame, "return");
  tagHandlers["return"] = make_pair(&CSharpDocConverter::handleTagSame, "returns");
  tagHandlers["returns"] = make_pair(&CSharpDocConverter::handleTagSame, "");
  tagHandlers["see"] = make_pair(&CSharpDocConverter::handleTagCref, "see");
  tagHandlers["sa"] = make_pair(&CSharpDocConverter::handleTagCref, "seealso");
  tagHandlers["since"] = make_pair(&CSharpDocConverter::handleTagSame, "");
  tagHandlers["throws"] = make_pair(&CSharpDocConverter::handleTagSame, "");
  tagHandlers["throw"] = make_pair(&CSharpDocConverter::handleTagSame, "throws");
  tagHandlers["version"] = make_pair(&CSharpDocConverter::handleTagSame, "");
  // these commands have special handlers
  // special handling of brief + partofdescription: both are combined to summary
  //tagHandlers["brief"] = make_pair(&CSharpDocConverter::handleParagraph, "");
  //tagHandlers["partofdescription"] = make_pair(&CSharpDocConverter::handleParagraph, "");
  tagHandlers["summary"] = make_pair(&CSharpDocConverter::handleSummary, "");
  //tagHandlers["code"] = make_pair(&CSharpDocConverter::handleTagExtended, "code");
  //tagHandlers["cond"] = make_pair(&CSharpDocConverter::handleTagMessage, "Conditional comment: ");
  //tagHandlers["copyright"] = make_pair(&CSharpDocConverter::handleTagMessage, "Copyright: ");
  //tagHandlers["else"] = make_pair(&CSharpDocConverter::handleTagIf, "Else: ");
  //tagHandlers["elseif"] = make_pair(&CSharpDocConverter::handleTagIf, "Else if: ");
  //tagHandlers["endcond"] = make_pair(&CSharpDocConverter::handleTagMessage, "End of conditional comment.");
  //// space in second arg prevents Javadoc to treat '@ example' as command. File name of
  //// example is still informative to user.
  //tagHandlers["example"] = make_pair(&CSharpDocConverter::handleTagSame, " example");
  //tagHandlers["if"] = make_pair(&CSharpDocConverter::handleTagIf, "If: ");
  //tagHandlers["ifnot"] = make_pair(&CSharpDocConverter::handleTagIf, "If not: ");
  //tagHandlers["image"] = make_pair(&CSharpDocConverter::handleTagImage, "");
  tagHandlers["link"] = make_pair(&CSharpDocConverter::handleTagLink, "");
  //tagHandlers["see"] = make_pair(&CSharpDocConverter::handleTagSee, "");
  //tagHandlers["sa"] = make_pair(&CSharpDocConverter::handleTagSee, "");
  tagHandlers["note"] = make_pair(&CSharpDocConverter::handleTagSame, "remarks");
  //tagHandlers["overload"] = make_pair(&CSharpDocConverter::handleTagMessage,
  //                                    "This is an overloaded member function, provided for"
  //                                    " convenience. It differs from the above function only in what" " argument(s) it accepts.");
  tagHandlers["par"] = make_pair(&CSharpDocConverter::handleTagPar, "");
  tagHandlers["remark"] = make_pair(&CSharpDocConverter::handleTagSame, "remarks");
  tagHandlers["remarks"] = make_pair(&CSharpDocConverter::handleTagSame, "Remarks: ");
  tagHandlers["todo"] = make_pair(&CSharpDocConverter::handleTagSame, "");
  //tagHandlers["verbatim"] = make_pair(&CSharpDocConverter::handleTagExtended, "literal");

  //// \f commands output literal Latex formula, which is still better than nothing.
  tagHandlers["f$"] = make_pair(&CSharpDocConverter::handleTagVerbatim, "");
  tagHandlers["f["] = make_pair(&CSharpDocConverter::handleTagVerbatim, "");
  tagHandlers["f{"] = make_pair(&CSharpDocConverter::handleTagVerbatim, "");

  tagHandlers["warning"] = make_pair(&CSharpDocConverter::handleTagSame, "remarks");
  //// this command just prints it's contents
  //// (it is internal command of swig's parser, contains plain text)
  tagHandlers["plainstd::string"] = make_pair(&CSharpDocConverter::handlePlainString, "");
  tagHandlers["plainstd::endl"] = make_pair(&CSharpDocConverter::handleNewLine, "");
  tagHandlers["n"] = make_pair(&CSharpDocConverter::handleNewLine, "");

  // HTML tags
  tagHandlers["<a"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<a");
  tagHandlers["<b"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<b");
  tagHandlers["<blockquote"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<blockquote");
  tagHandlers["<body"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<body");
  tagHandlers["<br"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<br");
  tagHandlers["<center"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<center");
  tagHandlers["<caption"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<caption");
  tagHandlers["<code"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<code");
  tagHandlers["<dd"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<dd");
  tagHandlers["<dfn"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<dfn");
  tagHandlers["<div"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<div");
  tagHandlers["<dl"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<dl");
  tagHandlers["<dt"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<dt");
  tagHandlers["<em"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<em");
  tagHandlers["<form"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<form");
  tagHandlers["<hr"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<hr");
  tagHandlers["<h1"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<h1");
  tagHandlers["<h2"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<h2");
  tagHandlers["<h3"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<h3");
  tagHandlers["<i"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<i");
  tagHandlers["<input"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<input");
  tagHandlers["<img"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<img");
  tagHandlers["<li"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<li");
  tagHandlers["<meta"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<meta");
  tagHandlers["<multicol"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<multicol");
  tagHandlers["<ol"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<ol");
  tagHandlers["<p"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<p");
  tagHandlers["<pre"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<pre");
  tagHandlers["<small"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<small");
  tagHandlers["<span"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<span");
  tagHandlers["<strong"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<strong");
  tagHandlers["<sub"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<sub");
  tagHandlers["<sup"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<sup");
  tagHandlers["<table"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<table");
  tagHandlers["<td"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<td");
  tagHandlers["<th"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<th");
  tagHandlers["<tr"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<tr");
  tagHandlers["<tt"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<tt");
  tagHandlers["<kbd"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<kbd");
  tagHandlers["<ul"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<ul");
  tagHandlers["<var"] = make_pair(&CSharpDocConverter::handleDoxyHtmlTag, "<var");

  // HTML entities
  tagHandlers["&copy"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&copy");
  tagHandlers["&trade"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&trade");
  tagHandlers["&reg"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&reg");
  tagHandlers["&lt"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&lt");
  tagHandlers["&gt"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&gt");
  tagHandlers["&amp"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&amp");
  tagHandlers["&apos"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&apos");
  tagHandlers["&quot"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&quot");
  tagHandlers["&lsquo"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&lsquo");
  tagHandlers["&rsquo"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&rsquo");
  tagHandlers["&ldquo"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&ldquo");
  tagHandlers["&rdquo"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&rdquo");
  tagHandlers["&ndash"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&ndash");
  tagHandlers["&mdash"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&mdash");
  tagHandlers["&nbsp"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&nbsp");
  tagHandlers["&times"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&times");
  tagHandlers["&minus"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&minus");
  tagHandlers["&sdot"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&sdot");
  tagHandlers["&sim"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&sim");
  tagHandlers["&le"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&le");
  tagHandlers["&ge"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&ge");
  tagHandlers["&larr"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&larr");
  tagHandlers["&rarr"] = make_pair(&CSharpDocConverter::handleHtmlEntity, "&rarr");
}

CSharpDocConverter::CSharpDocConverter(int flags) :
  DoxygenTranslator(flags) {
  fillStaticTables();
}

/**
 * Formats comment lines by inserting '\n *' at to long lines and tabs for
 * indent. Currently it is disabled, which means original comment format is
 * preserved. Experience shows, that this is usually better than breaking
 * lines automatically, especially because original line endings are not removed,
 * which results in short lines. To be useful, this function should have much
 * better algorithm.
 */
std::string CSharpDocConverter::formatCommand(std::string unformattedLine, int indent) {
  std::string formattedLines;
  return unformattedLine; // currently disabled
  std::string::size_type lastPosition = 0;
  std::string::size_type i = 0;
  int isFirstLine = 1;
  while (i != std::string::npos && i < unformattedLine.length()) {
    lastPosition = i;
    if (isFirstLine) {
      i += APPROX_LINE_LENGTH;
    } else {
      i += APPROX_LINE_LENGTH - indent * TAB_SIZE;
    }

    i = unformattedLine.find(" ", i);

    if (i > 0 && i + 1 < unformattedLine.length()) {
      if (!isFirstLine)
        for (int j = 0; j < indent; j++) {
          formattedLines.append("\t");
      } else {
        isFirstLine = 0;
      }
      formattedLines.append(unformattedLine.substr(lastPosition, i - lastPosition + 1));
      formattedLines.append("\n *");

    }
  }
  if (lastPosition < unformattedLine.length()) {
    if (!isFirstLine) {
      for (int j = 0; j < indent; j++) {
        formattedLines.append("\t");
      }
    }
    formattedLines.append(unformattedLine.substr(lastPosition, unformattedLine.length() - lastPosition));
  }

  return formattedLines;
}

/**
 * Returns true, if the given parameter exists in the current node
 * (for example param is a name of function parameter). If feature
 * 'doxygen:nostripparams' is set, then this method always returns
 * true - parameters are copied to output regardless of presence in
 * function params list.
 */
bool CSharpDocConverter::paramExists(std::string param) {

  if (GetFlag(currentNode, "feature:doxygen:nostripparams")) {
    return true;
  }

  ParmList *plist = CopyParmList(Getattr(currentNode, "parms"));

  for (Parm *p = plist; p;) {

    if (Getattr(p, "name") && Char(Getattr(p, "name")) == param) {
      return true;
    }
    /* doesn't seem to work always: in some cases (especially for 'self' parameters)
     * tmap:in is present, but tmap:in:next is not and so this code skips all the parameters
     */
    //p = Getattr(p, "tmap:in") ? Getattr(p, "tmap:in:next") : nextSibling(p);
    p = nextSibling(p);
  }

  Delete(plist);

  return false;
}

std::string CSharpDocConverter::translateSubtree(DoxygenEntity &doxygenEntity) {
  std::string translatedComment;

  if (doxygenEntity.isLeaf) {
    return translatedComment;
  }

  for (DoxygenEntityListIt p = doxygenEntity.entityList.begin(); p != doxygenEntity.entityList.end(); p++) {

    translateEntity(*p, translatedComment);
    translateSubtree(*p);
  }

  return translatedComment;
}

/**
 * Checks if a handler for the given tag exists, and calls it.
 */
void CSharpDocConverter::translateEntity(DoxygenEntity &tag, std::string &translatedComment) {

  std::map<std::string, std::pair<tagHandler, std::string> >::iterator it;
  it = tagHandlers.find(getBaseCommand(tag.typeOfEntity));

  if (it != tagHandlers.end()) {
    (this->*(it->second.first))(tag, translatedComment, it->second.second);
  } else {
    // do NOT print warning, since there are many tags, which are not
    // translatable - many warnings hide important ones
    // addError(WARN_DOXYGEN_COMMAND_ERROR, "Unknown doxygen or HTML tag: " + tag.typeOfEntity);
  }
}


void CSharpDocConverter::handleTagAnchor(DoxygenEntity &tag, std::string &translatedComment, std::string &) {
  translatedComment += "<a id=\"" + translateSubtree(tag) + "\"></a>";
}


void CSharpDocConverter::handleTagHtml(DoxygenEntity &tag, std::string &translatedComment, std::string &arg) {
  if (tag.entityList.size()) { // do not include empty tags
    std::string tagData = translateSubtree(tag);
    // wrap the thing, ignoring whitespace
    size_t wsPos = tagData.find_last_not_of("\n\t ");
    if (wsPos != std::string::npos)
      translatedComment += "<" + arg + ">" + tagData.substr(0, wsPos + 1) + "</" + arg + ">" + tagData.substr(wsPos + 1);
    else
      translatedComment += "<" + arg + ">" + translateSubtree(tag) + "</" + arg + "> ";
  }
}

void CSharpDocConverter::handleDoxyHtmlTag(DoxygenEntity &tag, std::string &translatedComment, std::string &arg) {
  std::string htmlTagArgs = tag.data;
  if (htmlTagArgs == "/") {
    // end html tag, for example "</ul>
    translatedComment += "</" + arg.substr(1) + ">";
  } else {
    translatedComment += arg + htmlTagArgs + ">";
  }
}

void CSharpDocConverter::handleHtmlEntity(DoxygenEntity &, std::string &translatedComment, std::string &arg) {
  // html entities can be preserved
  translatedComment += arg + ';';
}

void CSharpDocConverter::handleNewLine(DoxygenEntity &, std::string &translatedComment, std::string &) {
  translatedComment += " \n /// ";
}

void CSharpDocConverter::handleTagChar(DoxygenEntity &tag, std::string &translatedComment, std::string &arg) {
  // escape it if we need to, else just print
  if (arg.size())
    translatedComment += arg;
  else
    translatedComment += tag.typeOfEntity;
}

// handles tags which are the same in Doxygen and CSharpdoc.
void CSharpDocConverter::handleTagSame(DoxygenEntity &tag, std::string &translatedComment, std::string &arg) {
  if (arg.size())
    tag.typeOfEntity = arg;
  translatedComment += formatCommand(std::string("<" + tag.typeOfEntity + ">" + translateSubtree(tag) + "</" + tag.typeOfEntity + ">"), 2);
}

void CSharpDocConverter::handleTagCref(DoxygenEntity & tag, std::string & translatedComment, std::string & arg) {
  if (tag.entityList.size() != 1)
    return;

  if (arg.size())
    tag.typeOfEntity = arg;

  string ref = tag.entityList.front().data;

  translatedComment += formatCommand(std::string("<" + tag.typeOfEntity + " cref=\"" + ref + "\"/>"), 2);
}

void CSharpDocConverter::handleParagraph(DoxygenEntity &tag, std::string &translatedComment, std::string &) {
  translatedComment += formatCommand(translateSubtree(tag), 0);
}

void CSharpDocConverter::handlePlainString(DoxygenEntity &tag, std::string &translatedComment, std::string &) {
  translatedComment += tag.data;
  // if (tag.data.size() && tag.data[tag.data.size()-1] != ' ')
  //    translatedComment += " ";
}

void CSharpDocConverter::handleSummary(DoxygenEntity & tag, std::string & translatedComment, std::string &) {
    std::string dummy;
    translatedComment += " /// <summary>\n /// ";
    handleParagraph(tag, translatedComment, dummy);
    translatedComment += "\n /// </summary>";
}

void CSharpDocConverter::handleTagVerbatim(DoxygenEntity &tag, std::string &translatedComment, std::string &arg) {
  translatedComment += arg + " ";
  for (DoxygenEntityListCIt it = tag.entityList.begin(); it != tag.entityList.end(); it++) {
    translatedComment += it->data;
  }
}

void CSharpDocConverter::handleTagExtended(DoxygenEntity &tag, std::string &translatedComment, std::string &arg) {
  std::string dummy;
  translatedComment += "<" + arg + ">";
  handleParagraph(tag, translatedComment, dummy);
  translatedComment += "</" + arg + ">";
}

//TODO
//void CSharpDocConverter::handleTagIf(DoxygenEntity &tag, std::string &translatedComment, std::string &arg) {
//  std::string dummy;
//  translatedComment += arg;
//  if (tag.entityList.size()) {
//    translatedComment += tag.entityList.begin()->data;
//    tag.entityList.pop_front();
//    translatedComment += " {" + translateSubtree(tag) + "}";
//  }
//}

//TODO
//void CSharpDocConverter::handleTagMessage(DoxygenEntity &tag, std::string &translatedComment, std::string &arg) {
//  std::string dummy;
//  translatedComment += formatCommand(arg, 0);
//  handleParagraph(tag, translatedComment, dummy);
//}

//TODO
//void CSharpDocConverter::handleTagImage(DoxygenEntity &tag, std::string &translatedComment, std::string &) {
//  if (tag.entityList.size() < 2)
//    return;
//
//  std::string file;
//  std::string title;
//
//  std::list<DoxygenEntity>::iterator it = tag.entityList.begin();
//  if (it->data != "html")
//    return;
//
//  it++;
//  file = it->data;
//
//  it++;
//  if (it != tag.entityList.end())
//    title = it->data;
//
//  translatedComment += "<img src=" + file;
//  if (title.size())
//    translatedComment += " alt=" + title;
//
//  // the size indication is supported for Latex only in Doxygen, see manual
//
//  translatedComment += "/>";
//}

void CSharpDocConverter::handleTagPar(DoxygenEntity &tag, std::string &translatedComment, std::string &) {
  if (tag.entityList.empty())
    return;

  std::string dummy;
  translatedComment += "<para alt=\"" + tag.entityList.begin()->data + "\">";
  tag.entityList.pop_front();
  handleParagraph(tag, translatedComment, dummy);
  translatedComment += "</para>";
}


void CSharpDocConverter::handleTagParam(DoxygenEntity &tag, std::string &translatedComment, std::string &) {
  std::string dummy;

  if (!tag.entityList.size())
    return;
  if (!paramExists(tag.entityList.begin()->data))
    return;

  translatedComment += "<param name=\"";
  translatedComment += tag.entityList.begin()->data;
  tag.entityList.pop_front();
  translatedComment += "\">";
  handleParagraph(tag, translatedComment, dummy);
  translatedComment += "</param>";
}


void CSharpDocConverter::handleTagRef(DoxygenEntity &tag, std::string &translatedComment, std::string &) {
  std::string dummy;
  if (!tag.entityList.size())
    return;

  // we translate to link, although \page is not supported in CSharp, but 
  // reader at least knows what to look at. Also for \anchor tag on the same
  // page this link works.
  string anchor = tag.entityList.begin()->data;
  tag.entityList.pop_front();
  string anchorText = anchor;
  if (!tag.entityList.empty()) {
    anchorText = tag.entityList.begin()->data;
  }
  translatedComment += "<a href=\"#" + anchor + "\">" + anchorText + "</a>";
}

void CSharpDocConverter::handleTagLink(DoxygenEntity &tag, std::string &translatedComment, std::string &) {
  std::string dummy;
  if (!tag.entityList.size())
    return;

  string linkObject = tag.entityList.begin()->data;
  tag.entityList.pop_front();
  size_t spacePos = linkObject.find(' ');
  string linkTarget = linkObject.substr(0, spacePos);
  string linkName = linkObject.substr(spacePos + 1);

  translatedComment += linkName;
  handleParagraph(tag, translatedComment, dummy);
}

/* This function moves all line endings at the end of child entities
 * out of the child entities to the parent.
 */
int CSharpDocConverter::shiftEndlinesUpTree(DoxygenEntity &root, int level) {
  DoxygenEntityListIt it = root.entityList.begin();
  while (it != root.entityList.end()) {
    // remove line endings
    int ret = shiftEndlinesUpTree(*it, level + 1);
    it++;
    if (ret > 0) {
      // only insert a single newline, we don't multiple newlines between xml-tags
      root.entityList.insert(it, DoxygenEntity("plainstd::endl"));
    }
  }

  // continue only if we are not root
  if (!level) {
    return 0;
  }

  int removedCount = 0;
  while (!root.entityList.empty()
         && root.entityList.rbegin()->typeOfEntity == "plainstd::endl") {
    root.entityList.pop_back();
    removedCount++;
  }
  return removedCount;
}

/**
 * This makes sure that all comment lines contain '*'. It is not mandatory in doxygen,
 * but highly recommended for Javadoc. '*' in empty lines are indented according
 * to indentation of the first line. Indentation of non-empty lines is not
 * changed - garbage in garbage out.
 */
std::string CSharpDocConverter::indentAndInsertAsterisks(const string &doc) {

  size_t idx = doc.find('\n');
  size_t indent = 0;
  bool singleLineComment = idx == string::npos;
  // Detect indentation.
  //   The first line in comment is the one after '/**', which may be
  //   spaces and '\n' or the text. In any case it is not suitable to detect
  //   indentation, so we have to skip the first '\n'.
  //   However, if there is just one line, then use that line to detect indentation.
  if (idx != string::npos) {
    size_t nonspaceIdx = doc.find_first_not_of(" \t", idx + 1);
    if (nonspaceIdx != string::npos) {
      indent = nonspaceIdx - idx;
    }
  }

  if (indent == 0) {
    // we can't indent the first line less than 0
    indent = 1;
  }
  // Create the first line of CSharpdoc comment.
  string indentStr(indent - 1, ' ');
  string translatedStr = indentStr + "/**";
  if (indent > 1) {
    // remove the first space, so that '*' will be aligned
    translatedStr = translatedStr.substr(1);
  }

  translatedStr += doc;

  // insert '*' before each comment line, if it does not have it
  idx = translatedStr.find('\n');

  while (idx != string::npos) {

    size_t nonspaceIdx = translatedStr.find_first_not_of(" \t", idx + 1);
    if (nonspaceIdx != string::npos && translatedStr[nonspaceIdx] != '*') {
      // line without '*' found - is it empty?
      if (translatedStr[nonspaceIdx] != '\n') {
        // add '* ' to each line without it
        translatedStr = translatedStr.substr(0, nonspaceIdx) + "* " + translatedStr.substr(nonspaceIdx);
        //printf(translatedStr.c_str());
      } else {
        // we found empty line, replace it with indented '*'
        translatedStr = translatedStr.substr(0, idx + 1) + indentStr + "* " + translatedStr.substr(nonspaceIdx);
      }
    }
    idx = translatedStr.find('\n', nonspaceIdx);
  }

  // Add the last comment line properly indented
  size_t nonspaceEndIdx = translatedStr.find_last_not_of(" \t");
  if (nonspaceEndIdx != string::npos) {
    if (translatedStr[nonspaceEndIdx] != '\n') {
      if (!singleLineComment)
	translatedStr += '\n';
    } else {
      // remove trailing spaces
      translatedStr = translatedStr.substr(0, nonspaceEndIdx + 1);
    }
  }
  translatedStr += indentStr + "*/\n";

  return translatedStr;
}

String *CSharpDocConverter::makeDocumentation(Node *node) {

  String *documentation = getDoxygenComment(node);

  if (documentation == NULL) {
    return NewString("");
  }

  if (GetFlag(node, "feature:doxygen:notranslate")) {

    string doc = Char(documentation);

    string translatedStr = indentAndInsertAsterisks(doc);

    return NewString(translatedStr.c_str());
  }

  // store the current node
  // (currently just to handle params)
  currentNode = node;

  DoxygenEntityList entityList = parser.createTree(node, documentation);

  // strip line endings at the beginning
  while (!entityList.empty()
         && entityList.begin()->typeOfEntity == "plainstd::endl")
  {
      entityList.pop_front();
  }

  // collect brief and partofdescription into common summary node
  DoxygenEntityList summaryEntityList;
  DoxygenEntityList::iterator entity = entityList.begin();
  while (entity != entityList.end())
  {
      if (entity->typeOfEntity == "brief" || entity->typeOfEntity == "partofdescription")
      {
          for (DoxygenEntityList::iterator e = entity->entityList.begin(); e != entity->entityList.end(); ++e)
          {
              summaryEntityList.push_back(*e);
          }
          entity = entityList.erase(entity);
      }
      else
      {
          ++entity;
      }
  }
  DoxygenEntity summary("summary", summaryEntityList);
  entityList.push_front(summary);

  if (m_flags & debug_translator)
  {
      std::cout << std::endl << "---LIST WITH SUMMARY ---" << std::endl;
      printTree(entityList);
  }

  // filter out unsupported nodes
  entity = entityList.begin();
  while (entity != entityList.end())
  {
      std::string baseCommand = getBaseCommand(entity->typeOfEntity);
      if (tagHandlers.find(baseCommand) == tagHandlers.end()
          || (baseCommand == "param" && !paramExists(entity->entityList.front().data)))
      {
          entity = entityList.erase(entity);
      }
      else
      {
          ++entity;
      }
  }

  if (m_flags & debug_translator)
  {
      std::cout << std::endl << "---LIST FILTERED---" << std::endl;
      printTree(entityList);
  }


  DoxygenEntity root("root", entityList);

  if (m_flags & debug_translator)
  {
      std::cout << std::endl << "---LIST WITH ROOT UNSHIFTED---" << std::endl;
      root.printEntity(0);
  }

  shiftEndlinesUpTree(root);

  // strip line endings at the beginning
  while (!root.entityList.empty()
         && root.entityList.begin()->typeOfEntity == "plainstd::endl")
  {
      root.entityList.pop_front();
  }

  // and at the end
  while (!root.entityList.empty()
         && root.entityList.rbegin()->typeOfEntity == "plainstd::endl")
  {
      root.entityList.pop_back();
  }

  if (m_flags & debug_translator)
  {
      std::cout << std::endl << "---LIST WITH ROOT---" << std::endl;
      root.printEntity(0);
  }

  std::string csharpDocString = "";
  csharpDocString += translateSubtree(root);
  csharpDocString += "\n";

  if (m_flags & debug_translator) {
    std::cout << std::endl << "\n---RESULT IN C# XML DOC---" << std::endl;
    std::cout << csharpDocString;
  }

  return NewString(csharpDocString.c_str());
}

void CSharpDocConverter::addError(int warningType, const std::string &message) {
  Swig_warning(warningType, "", 0, "Doxygen parser warning: %s. \n", message.c_str());
}
