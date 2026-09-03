# TSP Genetic Algorithm

![C++](https://img.shields.io/badge/C%2B%2B-17-00599C?style=flat-square&logo=cplusplus)
![CMake](https://img.shields.io/badge/Build-CMake-064F8C?style=flat-square&logo=cmake)
![License](https://img.shields.io/badge/License-MIT-green?style=flat-square)

A configurable genetic algorithm for experimenting with solutions to the **Traveling Salesman Problem (TSP)**. The project generates scenarios, evolves candidate routes and exports visual and numeric results for analysis.

## Features

- Uniform and circular point distributions
- Tournament and roulette-wheel selection
- Order Crossover (OX) and Partially Mapped Crossover (PMX)
- Mutation rate, population size and random seed configuration
- Early stopping based on generations without improvement
- Best-route and convergence plots exported as SVG
- Per-generation metrics exported as CSV
- Deterministic experiments through a configurable seed

## Build

### Requirements

- A C++17-compatible compiler, such as GCC or Clang
- GNU Make or CMake 3.10+

Using Make:

```bash
git clone https://github.com/Itonsow/tsp-genetic-algorithm.git
cd tsp-genetic-algorithm
make
```

Using CMake:

```bash
cmake -S . -B build
cmake --build build --config Release
```

## Usage

Run a quick validation:

```bash
./build/tsp_ga --check
```

Run a custom experiment:

```bash
./build/tsp_ga \
  --cenario uniforme \
  --pontos 60 \
  --epocas 800 \
  --pop 200 \
  --mut 0.08 \
  --selection torneio \
  --crossover ox \
  --seed 42
```

Important options:

| Option | Accepted values | Default |
| --- | --- | --- |
| `--cenario` | `uniforme`, `circulo` | `uniforme` |
| `--pontos` | Integer greater than or equal to 8 | `50` |
| `--epocas` | Positive integer | `500` |
| `--pop` | Positive integer | `200` |
| `--mut` | Number between 0 and 1 | `0.05` |
| `--selection` | `torneio`, `roulette` | `torneio` |
| `--crossover` | `ox`, `pmx` | `ox` |
| `--seed` | Integer | `42` |

Use `./build/tsp_ga --help` to see the complete list.

## Generated output

Each run can generate:

```text
outputs/
├── melhor_volta.svg     # Best route found
├── melhor_volta.txt     # Route order and total distance
├── convergencia.svg     # Fitness evolution
└── metricas.csv         # Best, average and worst fitness by generation
```

Intermediate SVG frames are written to `frames/` and can be converted into an animation with `svg2gif_optimized.sh` when its external image tools are available.

## Project structure

| Path | Purpose |
| --- | --- |
| `src/tsp.hpp` | TSP instance and route representation |
| `src/ga.hpp` | Genetic algorithm operators and evolution loop |
| `src/plot_utils.hpp` | SVG and CSV output helpers |
| `src/main.cpp` | CLI configuration and program orchestration |
| `third_party/plot.h` | Lightweight plotting dependency |

## License

Distributed under the MIT License. See [`LICENSE`](LICENSE) for details.
