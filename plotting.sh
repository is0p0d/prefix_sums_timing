#!/usr//bin/gnuplot -persist

set title 'Parallel Prefix Scan, Recursive vs Tree'
set ylabel 'CPU Time Expense'
set xlabel 'Problem Size'
set grid
set datafile separator ","
set term png
set output 'prefix_plot.png'
plot 'output.dat' using 1:2 title 'Recursive' with lines,\
     'output.dat' using 1:3 title 'Tree' with lines