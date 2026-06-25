#!/usr/bin/env python3
# cpp用
import subprocess
import sys
from pathlib import Path

from common import format_tour, read_input

CHALLENGES = 7


SCRIPT_DIR = Path(__file__).resolve().parent
CPP_SOLVER = SCRIPT_DIR / 'solver_greedy3optGA.exe'


def run_solver(input_path, output_path):
    subprocess.run([str(CPP_SOLVER), str(input_path), str(output_path)], check=True)


def generate_sample_output():
    for i in [7]:
        input_path = SCRIPT_DIR / f'input_{i}.csv'
        output_path = SCRIPT_DIR / f'output_{i}.csv'
        run_solver(input_path, output_path)
        print(f"Finish{i}")


if __name__ == '__main__':
    generate_sample_output()
