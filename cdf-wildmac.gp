set term postscript eps enhanced "Helvetica" 23
set output "cdf-wildmac.eps"

set xrange[0:500000]
set grid

plot 'wildmac.data' using 1:3 with lines notitle

