set term postscript eps enhanced "Helvetica" 23
set output "cdf.eps"

set xrange[0:500000]

plot 'no_wrap.data' using 1:3 with lines title "no wrap",\
     'wrap.data' using 1:3 with lines title "wrap"
