set terminal postscript eps enhanced dash color "" 25
set output "Gather.eps"
#set output "Gather.png"
#set terminal png

reset

set title "Gather"

set datafile separator ";"

set xlabel "array size (x10 000)"
set ylabel "latency (us)"
set key top left
set style data linespoints 
set grid

plot "Gather.csv" using ($1):($2) ti col lw 3, '' u ($1):($3) ti col lw 3,	 

set output
