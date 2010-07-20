set term postscript eps enhanced "Helvetica" 23
set output "cdf.eps"

set xrange[0:500000]

plot 'no_wrap.data' using 1:3 with lines title "no wrap",\
     'wrap.data' using 1:3 with lines title "wrap",\
     'delay.data' using 1:3 with lines title "delay",\
     'no_wrap_split.data' using 1:3 with lines title "no wrap split",\
     'no_wrap_rsplit.data' using 1:3 with lines title "no wrap rsplit",\
     'wrap_split.data' using 1:3 with lines title "wrap split",\
     'wrap_rsplit.data' using 1:3 with lines title "wrap rsplit",\
     'delay_split.data' using 1:3 with lines title "delay split",\
     'delay_rsplit.data' using 1:3 with lines title "delay rsplit"

