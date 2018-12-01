set terminal postscript eps enhanced dash color "" 25
set output "Reduce.eps"
#set terminal png
#set output "Reduce.png"

reset

set title "Reduce"

set datafile separator ";"

set xlabel "array size (x10 000)"
set ylabel "latency (us)"
set key top left
set style data linespoints 
set grid

plot "Reduce.csv" using ($1):($2) ti col lw 3, '' u ($1):($3) ti col lw 3, '' u ($1):($4) ti col lw 3,

set output
