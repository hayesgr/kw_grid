# kw_grid
KW_GRID for lack of a better name is an alternative to a hashtable.
It is faster in many cases.
Keywords are limited to 16bytes of length.
It uses the word itself as a hash. There is no hash calculation. It is fed into _m128i or 2 uint64_t to handle comparisons.
The key word list is sorted into alphabetical order by the first letter and then into shortest to longest in that group.
This is done so a primary look up table can use the first letter of the word and its length to tell you where to look in the second table and how many potentials there are of that size.
That is in some ways similar to the bucket of a hash table. Because we have no actual hash calculation we save quite a bit on performance.
On an intel xeon x5670 we had speeds 11% faster than using gperf testing against the exact same data.
That should be even better on newer processor.

Sorry, the code is a bit messy.


The compiler flags I am using with mingw-w64: 
-Wall
-O3
-m64
(-march=native or -msse4.1)
