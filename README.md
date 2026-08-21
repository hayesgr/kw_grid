# kw_grid
KW_GRID for lack of a better name is an alternative to a hashtable.
It is intended for demonstration purposes. As it is it is not meant for production code.

## It is faster in many cases.
+ Keywords are limited to 16bytes of length.
+ It uses the word itself as a hash.
+ There is no hash calculation. 
+ It is fed into _m128i or 2 uint64_t to handle comparisons.
+ The key word list is sorted into alphabetical order by the first letter and then into shortest to longest in that group.

This is done so a primary look up table can use the first letter of the word and its length to tell you where to look in the second table and how many potentials there are of that size.
That is in some ways similar to the bucket of a hash table. Because we have no actual hash calculation we save cpu cycles that can be spent checking the list.

## Performance
On an intel xeon x5670 we had speeds 11% faster than using gperf testing against the exact same data.
That should be even better on newer processor.

### The compiler flags I am using with mingw-w64: 
* -Wall
* -O3
* -m64
* (-march=native or -msse4.1)

## Use
1. Create a KW_Data union and use it as a buffer to feed any word in you want to test.
2. In doing so you should be able to get the length of the word you are wanting to test. If it is more than 16 characters you can fail it there yourself.
3. Once you have your word for testing call is_keyword.
      + uint32_t is_Keyword(const KW_Data* kw, uint8_t length);
4. It will return an id if it finds it or it will return INVALID_TOKEN_ID.


