#include <Rcpp.h>
#include "zip.h"
#include "rapidxml.h"
#include "xlsxcell.h"
#include "xlsxsheet.h"
#include "xlsxbook.h"
#include "string.h"
#include "utils.h"

using namespace Rcpp;

xlsxsheet::xlsxsheet(
    const std::string& name,
    std::string& sheet_xml,
    xlsxbook& book,
    String comments_path,
    const bool& include_blank_cells
    ):
  name_(name),
  book_(book),
  include_blank_cells_(include_blank_cells) {
  rapidxml::xml_document<> xml;
  xml.parse<rapidxml::parse_strip_xml_namespaces | rapidxml::parse_non_destructive | rapidxml::parse_no_string_terminators | rapidxml::parse_no_entity_translation>(&sheet_xml[0]);

  rapidxml::xml_node<>* worksheet = xml.first_node("worksheet");
  rapidxml::xml_node<>* sheetData = worksheet->first_node("sheetData");

  // If defaultColWidth not given, ECMA says you can work it out based on
  // baseColWidth, but that isn't necessarily given either, and the formula
  // is wrong because the reality is so complicated, see
  // https://support.microsoft.com/en-gb/kb/214123.
  defaultColWidth_ = 8.38;
  defaultRowHeight_ = 15;

  defaultColOutlineLevel_ = 1;
  defaultRowOutlineLevel_ = 1;

  cacheDefaultRowColAttributes(worksheet);
  cacheColAttributes(worksheet);
  cacheComments(comments_path);
  cacheCellcount(sheetData);
}

void xlsxsheet::cacheDefaultRowColAttributes(rapidxml::xml_node<>* worksheet) {
  rapidxml::xml_node<>* sheetFormatPr_ = worksheet->first_node("sheetFormatPr");

  if (sheetFormatPr_ != NULL) {
    // Don't use utils::getAttributeValueDouble because it might overwrite the
    // default value with NA_REAL.
    rapidxml::xml_attribute<>* defaultRowHeight =
      sheetFormatPr_->first_attribute("defaultRowHeight");
    if (defaultRowHeight != NULL)
      defaultRowHeight_ = strtod(defaultRowHeight->value(), NULL);

    rapidxml::xml_attribute<>*defaultColWidth =
      sheetFormatPr_->first_attribute("defaultColWidth");
    if (defaultColWidth != NULL)
      defaultColWidth_ = strtod(defaultColWidth->value(), NULL);
  }
}

void xlsxsheet::cacheColAttributes(rapidxml::xml_node<>* worksheet) {
  // Having done cacheDefaultRowColDims(), initilize vectors to default width
  // and undefined outlineLevel, then update with custom widths and actual
  // outlineLevels.  The number of columns might be available by parsing
  // <dimension><ref>, but if not then only by parsing the address of all the
  // cells.  I think it's better just to use the maximum possible number of
  // columns, 16834.

  colWidths_.assign(16384, defaultColWidth_);
  colOutlineLevels_.assign(16384, defaultColOutlineLevel_);

  rapidxml::xml_node<>* cols = worksheet->first_node("cols");
  if (cols == NULL)
    return; // No custom widths or outline levelsk

  for (rapidxml::xml_node<>* col = cols->first_node("col");
      col; col = col->next_sibling("col")) {

    // <col> applies to columns from a min to a max, which must be iterated over
    unsigned int min  = strtol(col->first_attribute("min")->value(), NULL, 10);
    unsigned int max  = strtol(col->first_attribute("max")->value(), NULL, 10);

    rapidxml::xml_attribute<>* width = col->first_attribute("width");
    double width_value = defaultColWidth_;
    if (width != NULL) {
      width_value = strtod(width->value(), NULL);
      for (unsigned int column = min; column <= max; ++column) {
        colWidths_[column - 1] = width_value;
      }
    }

    rapidxml::xml_attribute<>* outlineLevel = col->first_attribute("outlineLevel");
    int outlineLevelValue = defaultColOutlineLevel_;
    if (outlineLevel != NULL) {
      outlineLevelValue = strtol(outlineLevel->value(), NULL, 10) + 1;
      for (unsigned int column = min; column <= max; ++column) {
        colOutlineLevels_[column - 1] = outlineLevelValue;
      }
    }
  }
}

unsigned long long int xlsxsheet::cacheCellcount(
    rapidxml::xml_node<>* sheetData) {
  // Iterate over all rows and cells to count.  The 'dimension' tag is no use
  // here because it describes a rectangle of cells, many of which may be blank.
  unsigned long long int cellcount = 0;
  unsigned long long int commentcount = 0; // no. of matching comments
  rapidxml::xml_attribute<>* ref;
  std::string ref_val;
  std::map<std::string, std::string>::iterator comment;
  int j = 0;
  for (rapidxml::xml_node<>* row = sheetData->first_node("row");
      row; row = row->next_sibling("row")) {

    ref = row->first_attribute("r");
    if (ref) {
        j = std::atoi(ref->value()) - 1;
    }

    int k = 0;
    for (rapidxml::xml_node<>* c = row->first_node("c");
      c; c = c->next_sibling("c")) {
      // Check for a matching comment.
      // Comments on blank cells don't have a matching cell, so a blank cell
      // must be created for them.  Such additional cells must be added to the
      // actual cellcount, to initialize the returned vectors to the correct
      // length.
      ref = c->first_attribute("r");
      if (ref) {

        ref_val = std::string(ref->value(), ref->value_size());

        // Get the char* array from the std::string, which is null-terminated
        const char* tempCharArray = ref_val.c_str();

        std::pair<int, int> location = parseRef(tempCharArray);
        j = location.first;
        k = location.second;
        
      } else {
        ref_val = asA1(j + 1, k + 1);
      }
      
      comment = comments_.find(ref_val);
      if(comment != comments_.end()) {
        ++commentcount;
      }
      if (include_blank_cells_ || c->first_node() != NULL) {
        ++cellcount;
      }
      if ((cellcount + 1) % 1000 == 0) {
        checkUserInterrupt();
      }
      k++;
    }
    j++;
  }
  cellcount_ = cellcount + (comments_.size() - commentcount);
  return(cellcount_);
}

void xlsxsheet::cacheComments(String comments_path) {
  // Having constructed the map, they will each be deleted when they are matched
  // to a cell.  That will leave only those comments that are on empty cells.
  // Those are then appended as empty cells with comments.
  if (comments_path != NA_STRING) {
    std::string comments_file = zip_buffer(book_.path_, comments_path);
    rapidxml::xml_document<> xml;
    xml.parse<rapidxml::parse_strip_xml_namespaces>(&comments_file[0]);

    // Iterate over the comments to store the ref and text
    rapidxml::xml_node<>* comments = xml.first_node("comments");
    rapidxml::xml_node<>* commentList = comments->first_node("commentList");
    for (rapidxml::xml_node<>* comment = commentList->first_node();
        comment; comment = comment->next_sibling()) {
      rapidxml::xml_attribute<>* ref = comment->first_attribute("ref");
      std::string ref_string(ref->value(), ref->value_size());
      rapidxml::xml_node<>* r = comment->first_node();
      // Get the inline string
      std::string inlineString;
      parseString(r, inlineString); // value is modified in place
      comments_[ref_string] = inlineString;
    }
  }
}

void xlsxsheet::parseSheetData(
    rapidxml::xml_node<>* sheetData,
    unsigned long long int& i) {
  // Iterate through rows and cells in sheetData.  Cell elements are children
  // of row elements.  Columns are described elswhere in cols->col.
  rowHeights_.assign(1048576, defaultRowHeight_); // cache rowHeight while here
  rowOutlineLevels_.assign(1048576, defaultRowOutlineLevel_); // cache rowOutlineLevel while here
  unsigned long int rowNumber;

  rapidxml::xml_node<> *row = sheetData->first_node("row");
  if (!row)
  {
    return;
  }

  int j = 0;
  for (; row; row = row->next_sibling("row")) {

    // if row declares its number, take this opportunity to update j
    // when it exists, this row number is 1-indexed, but j is 0-indexed
    rapidxml::xml_attribute<>* ref = row->first_attribute("r");
    if (ref) {
        j = std::atoi(ref->value()) - 1;
    }

    
    rowNumber = j + 1;
    // Check for custom row height
    double rowHeight = defaultRowHeight_;
    rapidxml::xml_attribute<>* ht = row->first_attribute("ht");
    if (ht != NULL) {
      rowHeight = strtod(ht->value(), NULL);
      rowHeights_[j] = rowHeight;
    }
    // Check for row outline level
    unsigned int rowOutlineLevel = defaultRowOutlineLevel_;
    rapidxml::xml_attribute<>* outlineLevel = row->first_attribute("outlineLevel");
    if (outlineLevel != NULL) {
      rowOutlineLevel = strtol(outlineLevel->value(), NULL, 10) + 1;
      rowOutlineLevels_[j] = rowOutlineLevel;
    }

    int k = 0;

    if (include_blank_cells_) {
      for (rapidxml::xml_node<>* c = row->first_node();
          c; c = c->next_sibling()) {

        // if cell declares its location, take this opportunity to update j and k
        ref = c->first_attribute("r");
        if (ref) {
          std::pair<int, int> location = parseRef(ref->value());
          j = location.first;
          k = location.second;
        }
        
        xlsxcell cell(c, this, book_, i, j, k);

        // Sheet name, row height and col width aren't really determined by
        // the cell, so they're done in this sheet instance
        book_.sheet_[i] = name_;
        SET_STRING_ELT(book_.sheet_, i, Rf_mkCharCE(name_.c_str(), CE_UTF8));
        book_.height_[i] = rowHeight;
        book_.width_[i] = colWidths_[book_.col_[i] - 1];
        book_.rowOutlineLevel_[i] = rowOutlineLevel;
        book_.colOutlineLevel_[i] = colOutlineLevels_[book_.col_[i] - 1];

        ++i;
        if ((i + 1) % 1000 == 0)
          checkUserInterrupt();
        k++;
      }
    } else {
      for (rapidxml::xml_node<>* c = row->first_node();
          c; c = c->next_sibling()) {
        // If cell has no child nodes then it is empty (no value or formula)
        // besides maybe formatting (linked to via attributes not child nodes).
        rapidxml::xml_node<>* first_child = c->first_node();
        if (first_child != NULL) {

          // if cell declares its location, take this opportunity to update j and k
          ref = c->first_attribute("r");
          if (ref) {
            std::pair<int, int> location = parseRef(ref->value());
            j = location.first;
            k = location.second;
          }
          
          xlsxcell cell(c, this, book_, i, j, k);

          // TODO: check readxl's method of importing ranges

          // Sheet name, row height and col width aren't really determined by
          // the cell, so they're done in this sheet instance
          book_.sheet_[i] = name_;
          SET_STRING_ELT(book_.sheet_, i, Rf_mkCharCE(name_.c_str(), CE_UTF8));
          book_.height_[i] = rowHeight;
          book_.width_[i] = colWidths_[book_.col_[i] - 1];
          book_.rowOutlineLevel_[i] = colOutlineLevels_[book_.col_[i] - 1];
          book_.colOutlineLevel_[i] = rowOutlineLevel;

          ++i;
          if ((i + 1) % 1000 == 0)
            checkUserInterrupt();
        }
        k++;
      }
    }
    j++;
  }
}

void xlsxsheet::appendComments(unsigned long long int& i) {
  // Having constructed the comments_ map, they are each be deleted when they
  // are matched to a cell.  That leaves only those comments that are on empty
  // cells.  This code appends those remaining comments as empty cells.
  int col;
  int row;
  for(std::map<std::string, std::string>::iterator it = comments_.begin();
      it != comments_.end(); ++it) {
    // TODO: move address parsing to utils
    std::string address = it->first.c_str(); // we need this std::string in a moment
    // Iterate though the A1-style address string character by character
    col = 0;
    row = 0;
    for(std::string::const_iterator iter = address.begin();
        iter != address.end(); ++iter) {
      if (*iter >= '0' && *iter <= '9') { // If it's a number
        row = row * 10 + (*iter - '0'); // Then multiply existing row by 10 and add new number
      } else if (*iter >= 'A' && *iter <= 'Z') { // If it's a character
        col = 26 * col + (*iter - 'A' + 1); // Then do similarly with columns
      }
    }
    SET_STRING_ELT(book_.sheet_, i, Rf_mkCharCE(name_.c_str(), CE_UTF8));
    book_.address_[i] = address;
    book_.row_[i] = row;
    book_.col_[i] = col;
    book_.is_blank_[i] = true;
    book_.content_[i] = NA_STRING;
    book_.data_type_[i] = "blank";
    book_.error_[i] = NA_STRING;
    book_.logical_[i] = NA_LOGICAL;
    book_.numeric_[i] = NA_REAL;
    book_.date_[i] = NA_REAL;
    book_.character_[i] = NA_STRING;
    book_.formula_[i] = NA_STRING;
    book_.is_array_[i] = false;
    book_.formula_ref_[i] = NA_STRING;
    book_.formula_group_[i] = NA_INTEGER;
    SET_STRING_ELT(book_.comment_, i, Rf_mkCharCE(it->second.c_str(), CE_UTF8));
    book_.height_[i] = rowHeights_[row - 1];
    book_.width_[i] = colWidths_[col - 1];
    book_.rowOutlineLevel_[i] = rowOutlineLevels_[row - 1];
    book_.colOutlineLevel_[i] = colOutlineLevels_[col - 1];
    book_.style_format_[i] = "Normal";
    book_.local_format_id_[i] = 1;
    ++i;
  }
  // Iterate though the A1-style address string character by character
}
