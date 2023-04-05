# bidibiba
BIDIBIBA is a BIDIrection BIsection BAndwidth test with a great name.
What the test does is create an arbitrary split in a dragonfly network that divides the nodes into two equal sized partitions (the bisection).  It ensures that for every process in a dragonfly group on one side of the bisection it finds a partner on the other side of the bisection.  Then it runs bandwidth tests across each pair simultaneously.

This is similar to the middle iteration of a ring-exchanged all-to-all, in that it is going to stress the bisection of your network.

The last thing the benchmark does is print out the stats of processes achieved throughput, such as:

A total of 1942 nodes participating from 28 groups.
*****SUMMARY BW (MiB/s)*****
min, mean, 50, 75, max, sum
14764.5,18099,18094.8,18259.2,21331.4,3.51483e+07

These programs are designed for the NERSC Cori and NERSC Perlmutter systems, but should be easily adaptable to any Dragonfly network so long as you know how to calculate the groups that a node belongs to.  For a FatTree it's just a matter of splitting the ports at the Top of Rack switch and the descendants.
