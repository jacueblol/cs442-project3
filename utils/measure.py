#!/usr/bin/env python3

"""
Measure the performance of a project 3 pzip binary
"""

from argparse import ArgumentParser
from dataclasses import dataclass
from resource import RUSAGE_CHILDREN
from resource import getrusage as resource_usage
from subprocess import run
from time import time as timestamp


@dataclass
class Performance:
    """
    Project 3 performance metrics
    """

    wall_time: float
    """
    Wall time of the execution (in seconds)
    """

    sys_time: float
    """
    CPU time spent in the system (in seconds)
    """

    user_time: float
    """
    CPU time spent in the user space (in seconds)
    """

    parallel_efficiency: float
    """
    Parallel efficiency of the execution (unitless, higher is better)
    """


def measure_performance(
    binary_path: str, input_path: str, output_path: str, threads: int
) -> Performance:
    """
    Measure the performance of a project 3 submission
    """

    # Get resource usage
    start_time, start_resources = timestamp(), resource_usage(RUSAGE_CHILDREN)

    # Run the binary
    run_result = run([binary_path, input_path, output_path, str(threads)], check=True)

    if run_result.returncode != 0:
        print(f"Execution failed with error code {run_result.returncode}")
        exit(run_result.returncode)

    # Get resource usage
    end_resources, end_time = resource_usage(RUSAGE_CHILDREN), timestamp()

    # Calculate parallel efficiency
    wall_time = end_time - start_time
    sys_time = end_resources.ru_stime - start_resources.ru_stime
    user_time = end_resources.ru_utime - start_resources.ru_utime
    parallel_efficiency = (user_time + sys_time) / (wall_time * threads)

    return Performance(wall_time, sys_time, user_time, parallel_efficiency)


def main():
    """
    Main function
    """
    # Parse arguments
    parser = ArgumentParser(
        description="Measure the performance of a project 3 pzip binary"
    )
    parser.add_argument("binary", type=str, help="Path of the binary to measure")
    parser.add_argument("input", type=str, help="Path of input file")
    parser.add_argument("output", type=str, help="Path to save output to")
    parser.add_argument("threads", type=int, help="Number of threads to use")

    args = parser.parse_args()

    # Measure performance
    performance = measure_performance(
        args.binary, args.input, args.output, args.threads
    )

    # Print
    print(f"Wall time: {performance.wall_time:.5f} seconds")
    print(f"CPU Time (System): {performance.sys_time:.5f}  seconds")
    print(f"CPU Time (User): {performance.user_time:.5f} seconds")
    print(f"Number of threads: {args.threads}")
    print(f"Parallel efficiency (PE): {performance.parallel_efficiency:.5f}")


if __name__ == "__main__":
    main()
