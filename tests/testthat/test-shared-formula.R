context("Shared formulas")

test_that("Shared formulas are propogated correctly", {
  formulas <- tidyxlcustom::xlsx_cells("./examples.xlsx")$formula
  expect_equal(formulas[41], "$A$18+1")
  expect_equal(formulas[42], "A20+2")
  expect_equal(formulas[195], "C3&\"C1\"\"\"")
  expect_equal(formulas[199], "LOG12")
  expect_equal(formulas[203], "LOG10(1)")
  expect_equal(formulas[235], "A1A")
  expect_equal(formulas[239], "E09904.2!A3")
  formulas <- tidyxlcustom::xlsx_cells("./formulas.xlsx")$formula
  expect_equal(formulas[3], "N($A2)")
  expect_equal(formulas[12], "G4*H4")
})
