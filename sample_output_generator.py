#!/usr/bin/env python3

from common import format_tour, read_input

import solver_greedy2optSA

CHALLENGES = 7


def generate_sample_output():
    for i in range(7):
        cities = read_input(f'input_{i}.csv')
        for solver, name in [(solver_greedy2optSA, 'greedy2optSA')]:
            tour = solver.solve(cities)
            with open(f'output_{i}.csv', 'w') as f:
                f.write(format_tour(tour) + '\n')
        print(f"Finish{i}")


if __name__ == '__main__':
    generate_sample_output()
