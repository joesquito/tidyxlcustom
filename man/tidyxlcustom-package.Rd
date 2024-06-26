% Generated by roxygen2: do not edit by hand
% Please edit documentation in R/tidyxlcustom-package.R
\docType{package}
\name{tidyxlcustom-package}
\alias{tidyxlcustom}
\alias{tidyxlcustom-package}
\title{tidyxlcustom: Import xlsx (Excel) spreadsheet data and formatting into tidy structures.}
\description{
Tidyxlcustom imports data from spreadsheets without coercing it into a rectangle,
and retains information encoded in cell formatting (e.g. font/fill/border).
This data structure is compatible with the 'unpivotr' package for
recognising information expressed by relative cell positions and cell
formatting, and re-expressing it in a tidy way.
}
\details{
\itemize{
\item \code{\link[=xlsx_cells]{xlsx_cells()}} Import cells from an xlsx file.
\item \code{\link[=xlsx_formats]{xlsx_formats()}} Import formatting from an xlsx file.
\item \code{\link[=xlsx_sheet_names]{xlsx_sheet_names()}} List the names of sheets in an xlsx file.
\item \code{\link[=xlsx_names]{xlsx_names()}} Import names and definitions of named ranges (aka
'named formulas', 'defined names') from an xlsx file.
\item \code{\link[=is_range]{is_range()}} Test whether a 'name' from \code{\link[=xlsx_names]{xlsx_names()}}
refers to a range or not.
\item \code{\link[=xlsx_validation]{xlsx_validation()}} Import cell input validation rules (e.g.
'must be from this drop-down list') from an xlsx file.
\item \code{\link[=xlsx_colour_standard]{xlsx_colour_standard()}} A data frame of standard colour names and
their RGB values.
\item \code{\link[=xlsx_colour_theme]{xlsx_colour_theme()}} Imports a data frame of theme colour names
and their RGB values from an xlsx file.
\item \code{\link[=xlex]{xlex()}} Tokenise (lex) an Excel formula.
}
}
\seealso{
Useful links:
\itemize{
  \item \url{https://github.com/nacnudus/tidyxl}
  \item \url{https://nacnudus.github.io/tidyxl/}
  \item Report bugs at \url{https://github.com/nacnudus/tidyxl/issues}
}

}
\author{
\strong{Maintainer}: Duncan Garmonsway \email{nacnudus@gmail.com}

Other contributors:
\itemize{
  \item Hadley Wickham (Author of included readxl fragments) [contributor]
  \item Jenny Bryan (Author of included readxl fragments) [contributor]
  \item RStudio (Copyright holder of included readxl fragments) [copyright holder]
  \item Marcin Kalicinski (Author of included RapidXML code) [contributor, copyright holder]
}

}
