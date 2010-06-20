gcc -ggdb -o simulation simulation_v2.c -Wall -I/opt/local/include -L/opt/local/lib -lgsl
#gcc -o simulation simulation_v1.c -Wall -I/opt/local/include -L/opt/local/lib -lgsl
gcc -ggdb -o uconnect u_connect.c -Wall -I/opt/local/include -L/opt/local/lib -lgsl

rm ./uconnect_comparison/1.txt 
rm ./uconnect_comparison/2.txt 
rm ./uconnect_comparison/3.txt 
rm ./uconnect_comparison/4.txt 
rm ./uconnect_comparison/5.txt


#CONFIGURATION FOR DC=0.015
./simulation 2 100000 2550250 12750 1275125 1 0 1 0 > 1.txt
./simulation 2 100000 2550250 12750 1275125 1 0 0 0 > 2.txt
#./simulation 2 10000 2550250 12750 1275125 1 0 1 0 > 3.txt
#./simulation 2 10000 2550250 12750 1275125 1 0 0 0 > 4.txt



#COMPARING WITH REALISTIC PARAMETERS THE BASIC 0.015, WITH THE HALF, AND HALF-KEEPING THE BEACON UNTOUCHED, OPTIMIZED FOR U_CONNECT

#./simulation 2 1000000 2550250 12750 1275125 1 0 1 0 > 1.txt
#./simulation 2 1000000 1275125 6375 637562 1 0 1 0 > 2.txt
#./simulation 2 1000000 1275125 12750 637562 1 0 1 0 > 3.txt
#./simulation 2 1000000 1260250 9000 630125 1 0 1 0 > 4.txt

#---------

#plot commands for gnuplot to get the beacon vs dc

#set xrange[0:10000]
#set yrange[0:0.15]
#set grid
#set xrange[0:13000]
#plot (x + ((637562.5/x)*150))/1275125
#set xrange[0:20000]
#plot (x + ((637562.5/x)*200))/1275125
#set yrange[0:0.1]
#set term x11
#plot (x + ((637562.5/x)*250))/1275125
#set xtitle "Beacon value in microseconds"
#set xlabel "Beacon value in microseconds"
#set ylabel "Duty Cycle"
#set term png size 1024,760
#set output "beacon_vs_dc.png"
#plot (x + ((637562.5/x)*250))/1275125 t "DC in function of Beaconing length"

#----------------

#COMPARING THE REALISTIC PARAMETER WITH ALL THE VERSIONS WHEN GENERATING T BETWEEN [0,T]
#./simulation 2 1000000 2550250 12750 1275125 1 0 1 0 > 1.txt
#./simulation 2 1000000 2550250 12750 1275125 1 0 0 1 > 2.txt
#./simulation 2 1000000 2550250 12750 1275125 1 0 1 1 > 3.txt
#./simulation 2 1000000 2550250 0 1287875 1 0 0 1 > 4.txt
#./simulation 2 1000000 2550250 0 1287875 1 0 1 1 > 5.txt

#PLOT COMMANDS TO SHOW THE OVERLAPPING OF ACTIVE PERIODS FOR RUTH

#set yrange [0:3]
#set xrange [0:2550250*7]
#set xtics 2550250
#set xtics ("1 Epoch" 2550250, "2 Epoch" 2550250*2, "3 Epoch" 2550250*3, "4 Epoch" 2550250*4, "5 Epoch" 2550250*5, "6 Epoch" 2550250*6, "7 Epoch" 2550250*7)
#set term png size 1024,760
#set output "behaviour.png"
#plot "jarl.txt" t "Active times" with boxes

#U-CONNECT SIMULATOR VALUES
#./uconnect 2 1000000 101 1 0 1 > 1.txt
#./uconnect 2 1000000 101 1 0 2 > 2.txt
#./uconnect 2 1000000 101 1 1 1 > 3.txt
#./uconnect 2 1000000 101 1 1 2 > 4.txt

#./simulation 2 1000000 2550250 12750 1275125 1 0 1 1 > 1.txt
#./simulation 2 1000000 2550250 12750 1275125 1 0 0 1 > 2.txt
#./simulation 2 1000000 2550250 0 1287875 1 0 1 1 > 3.txt
#./simulation 2 1000000 2550250 0 1287875 1 0 0 1 > 4.txt

mv *.txt ./uconnect_comparison/

