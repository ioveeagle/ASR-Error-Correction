g++ -c -O2 -Wall feature.c

### -funroll-loops -D_DEBUG -DSSE2
#g++ -msse2 -DSSE2 -c -O3 -Wall feature.c
#g++ -D_DEBUG -mavx -DAVX -c -O3 -Wall feature.c
#g++ -D_DEBUG -O3 -c -Wall feature.c
g++ *.o -o feature.x
rm *.o

