
<!-- README.md is generated from README.Rmd. Please edit that file -->

# tidyxl

<!-- badges: start -->

[![R-CMD-check](https://github.com/nacnudus/tidyxl/workflows/R-CMD-check/badge.svg)](https://github.com/nacnudus/tidyxl/actions)
[![Cran
Status](http://www.r-pkg.org/badges/version/tidyxl)](https://cran.r-project.org/package=tidyxl)
[![Cran
Downloads](https://cranlogs.r-pkg.org/badges/tidyxl)](https://www.r-pkg.org/pkg/tidyxl)
[![codecov](https://app.codecov.io/gh/nacnudus/tidyxl/coverage.svg?branch=master)](https://app.codecov.io/gh/nacnudus/tidyxl)
[![R-CMD-check](https://github.com/nacnudus/tidyxl/actions/workflows/R-CMD-check.yaml/badge.svg)](https://github.com/nacnudus/tidyxl/actions/workflows/R-CMD-check.yaml)
<!-- badges: end -->

[tidyxl](https://github.com/nacnudus/tidyxl) imports non-tabular data
from Excel files into R. It exposes cell content, position, formatting
and comments in a tidy structure for further manipulation, especially by
the [unpivotr](https://github.com/nacnudus/unpivotr) package. It
supports the xml-based file formats ‘.xlsx’ and ‘.xlsm’ via the embedded
[RapidXML](https://rapidxml.sourceforge.net) C++ library. It does not
support the binary file formats ‘.xlsb’ or ‘.xls’.
