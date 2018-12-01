set terminal postscript eps enhanced dash color "" 25
set output "Gather.eps"
#set output "Gather.png"
#set terminal png

reset

set title "Gather"

set datafile separator ";"

set xlabel "job count (x1 000)"
set ylabel "latency (seconds)"
set key top left
set style data linespoints
set grid

plot "Gather.csv" using ($1/1000):($2/1000000) ti col lw 10, '' u ($1/1000):($3/1000000) ti col lw 10,

set output
