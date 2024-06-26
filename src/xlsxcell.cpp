#include <Rcpp.h>
#include "rapidxml.h"
#include "xlsxbook.h"
#include "xlsxcell.h"
#include "xlsxsheet.h"
#include "string.h"
#include "date.h"
#include "utils.h"

using namespace Rcpp;

xlsxcell::xlsxcell(
    rapidxml::xml_node<>* cell,
    xlsxsheet* sheet,
    xlsxbook& book,
    unsigned long long int& i,
    int& j,
    int& k
    ) {
    parseAddress(cell, sheet, book, i, j, k);
    cacheComment(sheet, book, i);
    cacheValue  (cell, sheet, book, i); // Also caches format, as inextricable
    cacheFormula(cell, sheet, book, i);
}

// Based on tidyverse/readxl
// Get the A1-style address, and parse it for the row and column numbers.
// Simple parser: does not check that order of numbers and letters is correct
// row_ and column_ are one-based
void xlsxcell::parseAddress(
    rapidxml::xml_node<>* cell,
    xlsxsheet* sheet,
    xlsxbook& book,
    unsigned long long int& i,
    int& j,
    int& k
    ) {
  rapidxml::xml_attribute<>* ref = cell->first_attribute("r");

  if (ref) {
    address_ = ref->value(); // we need this std::string in a moment
  } else {
    address_ = asA1(j + 1, k + 1).c_str();
  }

  book.address_[i] = address_;

  col_ = k + 1;
  row_ = j + 1;

  book.col_[i] = col_;
  book.row_[i] = row_;
}

void xlsxcell::cacheComment(
    xlsxsheet* sheet,
    xlsxbook& book,
    unsigned long long int& i
    ) {
  // Look up any comment using the address, and delete it if found
  std::map<std::string, std::string>& comments = sheet->comments_;
  std::map<std::string, std::string>::iterator it = comments.find(address_);
  if(it != comments.end()) {
    SET_STRING_ELT(book.comment_, i, Rf_mkCharCE(it->second.c_str(), CE_UTF8));
    comments.erase(it);
  }
}

void xlsxcell::cacheValue(
    rapidxml::xml_node<>* cell,
    xlsxsheet* sheet,
    xlsxbook& book,
    unsigned long long int& i
    ) {
  // 'v' for 'value' is either literal (numeric) or an index into a string table
  rapidxml::xml_node<>* v = cell->first_node("v");
  rapidxml::xml_node<>* is = cell->first_node("is");

  std::string vvalue;
  if (v != NULL) {
    vvalue = v->value();
    SET_STRING_ELT(book.content_, i, Rf_mkCharCE(vvalue.c_str(), CE_UTF8));
  }

  // 't' for 'type' defines the meaning of 'v' for value
  rapidxml::xml_attribute<>* t = cell->first_attribute("t");
  std::string tvalue;
  if (t != NULL) {
    tvalue = t->value();
  }

  // 's' for 'style' indexes into data structures of formatting
  rapidxml::xml_attribute<>* s = cell->first_attribute("s");
  // Default the local format id to '1' if not present
  int svalue;
  if (s != NULL) {
    svalue = strtol(s->value(), NULL, 10);
  } else {
    svalue = 0;
  }
  book.local_format_id_[i] = svalue + 1;
  book.style_format_[i] = book.styles_.cellStyles_map_[book.styles_.cellXfs_[svalue].xfId_];

  if (t != NULL && tvalue == "inlineStr") {
    book.data_type_[i] = "character";
    if (is != NULL) { // Get the inline string if it's really there
      // Parse it as though it's a simple string
      std::string inlineString;
      parseString(is, inlineString); // value is modified in place
      // Also parse it as though it's a formatted string
      SET_STRING_ELT(book.character_, i, Rf_mkCharCE(inlineString.c_str(), CE_UTF8));
      book.character_formatted_[i] = parseFormattedString(is, book.styles_);
    }
    return;
  } else if (v == NULL) {
    // Can't now be an inline string (tested above)
    book.is_blank_[i] = true;
    book.data_type_[i] = "blank";
    return;
  } else if (t == NULL || tvalue == "n") {
    if (book.styles_.cellXfs_[svalue].applyNumberFormat_ == 1) {
      // local number format applies
      if (book.styles_.isDate_[book.styles_.cellXfs_[svalue].numFmtId_]) {
        // local number format is a date format
        book.data_type_[i] = "date";
        double date = strtod(vvalue.c_str(), NULL);
        book.date_[i] = checkDate(date, book.dateSystem_, book.dateOffset_,
                                  "'" + sheet->name_ + "'!" + address_);
        return;
      } else {
        book.data_type_[i] = "numeric";
        book.numeric_[i] = strtod(vvalue.c_str(), NULL);
      }
    } else if ( // no known case # nocov start
          book.styles_.isDate_[
            book.styles_.cellStyleXfs_[
              book.styles_.cellXfs_[svalue].xfId_
            ].numFmtId_
          ]
        ) {
      // style number format is a date format
      book.data_type_[i] = "date";
      double date = strtod(vvalue.c_str(), NULL);
      book.date_[i] = checkDate(date, book.dateSystem_, book.dateOffset_,
                                  "'" + sheet->name_ + "'!" + address_);
      return;
    } else {
      book.data_type_[i] = "numeric";
      book.numeric_[i] = strtod(vvalue.c_str(), NULL); // # nocov end
    }
  } else if (tvalue == "s") {
    // the t attribute exists and its value is exactly "s", so v is an index
    // into the string table.
    book.data_type_[i] = "character";
    SET_STRING_ELT(book.character_, i, Rf_mkCharCE(book.strings_[strtol(vvalue.c_str(), NULL, 10)].c_str(), CE_UTF8));
    book.character_formatted_[i] = book.strings_formatted_[strtol(vvalue.c_str(), NULL, 10)];
    return;
  } else if (tvalue == "str") {
    // Formula, which could have evaluated to anything, so only a string is safe
    book.data_type_[i] = "character";
    SET_STRING_ELT(book.character_, i, Rf_mkCharCE(vvalue.c_str(), CE_UTF8));
    return;
  } else if (tvalue == "b"){
    book.data_type_[i] = "logical";
    book.logical_[i] = strtod(vvalue.c_str(), NULL);
    return;
  } else if (tvalue == "e") {
    book.data_type_[i] = "error";
    book.error_[i] = vvalue;
    return;
  } else if (tvalue == "d") { // # nocov start
    // Does excel use this date type? Regardless, don't have cross-platform
    // ISO8601 parser (yet) so need to return as text.
    book.data_type_[i] = "date (ISO8601)";
    return; // # nocov end
  } else { // no known case
    book.data_type_[i] = "unknown"; // # nocov start
    return; // # nocov end
  }
}

void xlsxcell::cacheFormula(
    rapidxml::xml_node<>* cell,
    xlsxsheet* sheet,
    xlsxbook& book,
    unsigned long long int& i
    ) {
  rapidxml::xml_node<>* f = cell->first_node("f");
  std::string formula;
  int si_number;
  std::map<int, shared_formula>::iterator it;
  if (f != NULL) {
    formula = f->value();
    SET_STRING_ELT(book.formula_, i, Rf_mkCharCE(formula.c_str(), CE_UTF8));
    rapidxml::xml_attribute<>* f_t = f->first_attribute("t");
    if (f_t != NULL) {
      std::string ftvalue(f_t->value());
      if (ftvalue == "array") {
        book.is_array_[i] = true;
      }
    }

    rapidxml::xml_attribute<>* ref = f->first_attribute("ref");
    if (ref != NULL) {
      book.formula_ref_[i] = ref->value();
    }

    // Formulas are sometimes defined once, and then 'shared' with a range
    // p.1629 'shared' and 'si' attributes
    rapidxml::xml_attribute<>* si = f->first_attribute("si");
    if (si != NULL) {
      si_number = strtol(si->value(), NULL, 10);
      book.formula_group_[i] = si_number;
      if (formula.length() == 0) { // inherits definition
        it = sheet->shared_formulas_.find(si_number);
        SET_STRING_ELT(book.formula_, i, Rf_mkCharCE(it->second.offset(row_, col_).c_str(), CE_UTF8));
      } else { // defines shared formula
        shared_formula new_shared_formula(formula, row_, col_);
        sheet->shared_formulas_.insert({si_number, new_shared_formula});
      }
    }
  }
}
