set terminal postscript eps enhanced dash color "" 25
set output "Pipeline.eps"
#set terminal png
#set output "Pipeline.png"

reset

set title "Pipeline"

set datafile separator ";"

set xlabel "array size (x10 000)"
set ylabel "latency (us)"
set key top left
set style data linespoints 
set grid

plot "Pipeline.csv" using ($1):($2) ti col lw 3, '' u ($1):($3) ti col lw 3, '' u ($1):($4) ti col lw 3,

set output
