set terminal postscript eps enhanced dash color "" 25
set output "Farm.eps"
#set terminal png
#set output "Farm.png"

reset

set title "Farm"

set datafile separator ";"

set xlabel "job count (x1 000)"
set ylabel "latency (seconds)"
set key top left
set style data linespoints
set grid

plot "Farm.csv" using ($1/1000):($2/1000) ti col lw 10, '' u ($1/1000):($3/1000) ti col lw 10, 

set output
