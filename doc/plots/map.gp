set terminal postscript eps enhanced dash color "" 25
set output "Map.eps"
#set terminal x11

set title "Map"

reset

set datafile separator ";"

set xlabel "array size (x10 000)"
set ylabel "latency (us)"
#set yrange [0:5000]
#set key outside
set key top left
set style data linespoints 
#set logscale y
#set logscale x 2
set grid
#set boxwidth 20

#plot [][]\
#	 "abc.csv" using ($1):($3*100) with lines dt 3 lw 6 title "run 1",\

plot "Map.csv" using ($1):($2) ti col lw 3, '' u ($1):($3) ti col lw 3,	 

set output
