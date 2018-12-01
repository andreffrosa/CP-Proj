set terminal postscript eps enhanced dash color "" 25
set output "Scatter.eps"
#set terminal png
#set output "Scatter.png"

reset

set title "Scatter"

set datafile separator ";"

set xlabel "array size (x10 000)"
set ylabel "latency (us)"
set key top left
set style data linespoints 
set grid

plot "Scatter.csv" using ($1):($2) ti col lw 3, '' u ($1):($3) ti col lw 3,	 

set output
