# boardtracker
Simulate board density measurements

## Compile and run

```
g++ -std=c++14 main.cpp BoardDensitySystem.cpp -o boardtracker
./boardtracker
```

The program will run for 10 seconds, taking x-ray and position measurements at random intervals, and then print statistics at the end.

(or add `-g` if you wish to generate debug symbols)
