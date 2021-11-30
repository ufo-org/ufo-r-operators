library(testthat)
library(ufovectors)

options(warn=1)

test_check("ufovectors", reporter = SummaryReporter$new() )
