# CEP_Lab_last — Expense Splitter

## Project Overview
A command-line expense splitting tool written in C. Designed for Linux/Seneca Matrix environments. Lets roommates split bills by equal or weighted percentages, with persistent storage across runs.

## Features
- Enter a bill description, total amount, and number of people
- Equal or weighted percentage splits
- Input validation (no crashes on bad input)
- Persistent ledger — every split is appended to `ledger.txt` with a timestamp
- Roommate names saved to `roommates.dat` and suggested on next run
- Clean formatted output aligned to two decimal places

## Build System
- **Language:** C (GCC, `-Wall -Wextra -std=c11`)
- **Build tool:** Make

## Project Structure
```
.
├── main.c          # Full application source
├── Makefile        # Build configuration
├── ledger.txt      # Auto-created: persistent expense history
├── roommates.dat   # Auto-created: saved roommate names
├── .gitignore      # C build artifact ignore rules
├── LICENSE         # MIT License
└── README.md       # Project title
```

## Build & Run
```
make        # compile
make run    # compile and run
make clean  # remove binary
```

## Workflow
- **Start application**: Runs `make run` (console output)

## Data Files
- `ledger.txt` — appended on every completed split; never overwritten
- `roommates.dat` — last set of names used; loaded as suggestions on startup
