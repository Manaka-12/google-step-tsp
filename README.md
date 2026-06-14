# Google STEP 2026: Travelling Salesperson Problem Challenges

Originally By: [Hayato Ito](https://github.com/hayatoito) (hayato@google.com)  
2020-2026 Versions By: [Hugh O'Cinneide](https://github.com/hkocinneide)
(hughoc@google.com), [Hiromu Ikeda](https://github.com/rombot98) (hiromu@google.com) and [Ayaka Kinoshita](https://github.com/oribe1115) (oribe@google.com)

## Quick Links

- [Scoreboard]

[scoreboard]:
  https://docs.google.com/spreadsheets/d/1uOhewb9KtMENU5AyLF8VcsyosQGOfkeh-Kka8M-DYcI/edit?usp=sharing&resourcekey=0-w2dZASN1e-X_gcl8vicB4Q
[github issues]: https://github.com/hayatoito/google-step-tsp/issues

## Problem Statement

In this assignment, you will design an algorithm to solve a fundamental problem
faced by every travelling salesperson, called _Travelling Salesperson Problem_
(TSP). I’ll explain TSP in the onsite class. TSP is very famous problem. See
[Wikipedia](http://en.wikipedia.org/wiki/Travelling_salesman_problem). You can
understand the problem without any difficulties.

Quoted from
[Wikipedia](http://en.wikipedia.org/wiki/Travelling_salesman_problem):

> The travelling salesman problem (also called the travelling salesperson problem or TSP) 
> asks the following question: Given a list of cities and the distances between each pair 
> of cities, what is the shortest possible route that visits each city exactly once and 
> returns to the origin city?


# How I solved

### Random
- [solver_random.py](solver_random.py)

The tour is initialized in the order of input indices.



### Greedy
- [solver_greedy.py](solver_greedy.py)

The tour is generated using a greedy algorithm.


### Greedy + 2-opt
- [solver_greedy2opt.py](solver_greedy2opt.py)

The initial solution is generated using a greedy algorithm and then improved using 2-opt optimization.


### Greedy + 2-opt + SA(焼きなまし法)
- [solver_greedy2optSA.py](solver_greedy2optSA.py)

The initial solution is generated using a greedy algorithm and then improved using 2-opt optimization combined with Simulated Annealing to escape local optima.
Reference : https://qiita.com/take314/items/7eae18045e989d7eaf52

## Result of Visualizer


## Challenge 0

| Method | Result | Total Distance
|--------|--------|--------|
| Random  | ![](/result/random/pictures/challenge0.png) | 3862.2 m 
| Greedy  | ![](result/greedy/pictures/challenge0.png) | 3418.1 m
| Greedy-2opt | ![](result/greedy2opt/pictures/challenge0.png) | 3291.62 m 
| Greedy-2opt + SA | ![](result/greedy2opt/pictures/challenge0.png) | 3291.62 m 

## Challenge 1

| Method | Result |
|--------|--------|
| Random  | ![](/result/random/pictures/challenge1.png) | 6101.57 m 
| Greedy  | ![](result/greedy/pictures/challenge1.png) | 3832.29 m
| Greedy-2opt| ![](result/greedy2opt/pictures/challenge1.png) | 3832.29 m
| Greedy-2opt + SA | ![](result/greedy2optSA/pictures/challenge1.png) | 3778.72 m

## Challenge 2

| Method | Result |
|--------|--------|
| Random  | ![](/result/random/pictures/challenge2.png) | 13479.25 m
| Greedy  | ![](result/greedy/pictures/challenge2.png) | 5065.58 m
| Greedy-2opt | ![](result/greedy2opt/pictures/challenge2.png) | 4670.27 m
| Greedy-2opt + SA | ![](result/greedy2optSA/pictures/challenge2.png) | 4494.42 m

## Result of Greedy-2opt + SA


Path length for each challenge:

| Challenge | Path length (m) |
|------------|----------------|
| 0 | 3291.62 |
| 1 | 3778.72 |
| 2 | 4494.42 |
| 3 | 8467.91 |
| 4 | 11271.92 |
| 5 | 25331.84 |
| 6 | 49892.05 |

## Data Format Specification

### Input Format

The input consists of `N + 1` lines. The first line is always `x,y`. It is
followed by `N` lines, each line represents an i-th city’s location, point
`xi,yi` where `xi`, `yi` is a floating point number.

```
x,y
x_0,y_0
x_1,y_1
…
x_N-1,y_N-1
```

### Output Format

Output has `N + 1` lines. The first line should be “index”. It is followed by
`N` lines, each line is the index of city, which represents the visitation
order.

```
index
v_0
v_1
v_2
…
v_N-1
```

### Example (Challenge 0, N = 5)

Input Example:

```
x,y
214.98279057984195,762.6903632435094
1222.0393903625825,229.56212316547953
792.6961393471055,404.5419583098643
1042.5487563564207,709.8510160219619
150.17533883877582,25.512728869805677
```

Output (Solution) Example:

```
index
0
2
3
1
4
```

These formats are requirements for the visualizer, which can take only properly
formatted CSV files as input.



## What’s included in the assignment

To help you understand the problem, there are some sample scripts / resources in
the assignment, including, but not limited to:

- `solver_random.py` - Sample stupid solver. You never lose to this stupid one.
- `sample/random_{0-6}.csv` - Sample output files by solver_random.py.
  this definitely.
- `sample/sa_{0-6}.csv` - Yet another sample output files. I expect all of you
  will beat this one too. The solver itself is not included intentionally.
- `output_{0-6}.csv` - You should overwrite these files with your program's
  output.
- `output_verifier.py` - Try to validate your output files and print the path
  length.
- `input_generator.py` - Python script which was used to create input files,
  `input_{0-6}.csv`
- `visualizer/` - The directory for visualizer.

Details are intentionally omitted here. It is your responsibility to understand
the contents of the repository.

