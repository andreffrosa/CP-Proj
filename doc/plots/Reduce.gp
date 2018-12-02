set terminal postscript eps enhanced dash color "" 25
set output "Reduce.eps"
#set terminal png
#set output "Reduce.png"

reset

set title "Reduce"

set datafile separator ";"

set xlabel "job count (x1 000)"
set ylabel "latency (seconds)"
set key top left
set style data linespoints
set grid

plot "Reduce.csv" using ($1/1000):($2/1000) ti col lw 10, '' u ($1/1000):($3/1000) ti col lw 10, '' u ($1/1000):($4/1000) ti col lw 10 lt 4,

set output
