Fork to read xlsx files with missing r attributes

<!-- README.md is generated from README.Rmd. Please edit that file -->

# tidyxl

[tidyxl](https://github.com/nacnudus/tidyxl) imports non-tabular data
from Excel files into R. It exposes cell content, position, formatting
and comments in a tidy structure for further manipulation, especially by
the [unpivotr](https://github.com/nacnudus/unpivotr) package. It
supports the xml-based file formats ‘.xlsx’ and ‘.xlsm’ via the embedded
[RapidXML](https://rapidxml.sourceforge.net) C++ library. It does not
support the binary file formats ‘.xlsb’ or ‘.xls’.
