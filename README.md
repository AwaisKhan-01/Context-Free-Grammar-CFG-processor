# CS-4031 Compiler Construction - Assignment #02

## National University of Computer and Emerging Sciences  
**Fast School of Computing - Spring 2025**

## Table of Contents
- [Overview](#overview)
- [Features](#features)
- [Usage Instructions](#usage-instructions)
- [Implementation Details](#implementation-details)
- [Sample Input](#sample-input)
- [Sample Output](#sample-output)
- [Deliverables](#deliverables)
- [Challenges Faced](#challenges-faced)
- [Authors](#authors)

---
## Overview
This project implements a **Context-Free Grammar (CFG) processor** in C that performs the following tasks:
1. **Left Factoring**
2. **Left Recursion Removal**
3. **First Set Computation**
4. **Follow Set Computation**
5. **LL(1) Parsing Table Construction**

The program reads a CFG from an input file, processes it, and outputs the transformed grammar along with computed sets and the parsing table.

---
## Features
- Reads a **generic CFG** from a text file.
- **Identifies and removes left recursion** to prevent infinite recursion.
- **Performs left factoring** to prepare the grammar for predictive parsing.
- **Computes FIRST and FOLLOW sets** for all non-terminals.
- **Constructs an LL(1) Parsing Table** using the computed sets.
- **Outputs the results** in a structured format.

---
## Usage Instructions
### Prerequisites
- C compiler (GCC or Clang recommended)
- Basic knowledge of CFGs and LL(1) parsing

### Compilation and Execution
```sh
# Compile the program
gcc -o cfg_processor main.c left_factoring.c left_recursion.c first_follow.c parsing_table.c -Wall

# Run the program
./cfg_processor input.txt
```

### Input Format
- Productions should be written **one per line** using `->` as the delimiter.
- Example:
  ```txt
  E -> E + T | T
  T -> T * F | F
  F -> ( E ) | id
  ```

---
## Implementation Details
The program is divided into multiple modules:
- **main.c**: Handles input processing and function calls.
- **left_factoring.c**: Implements left factoring.
- **left_recursion.c**: Eliminates left recursion.
- **first_follow.c**: Computes FIRST and FOLLOW sets.
- **parsing_table.c**: Constructs the LL(1) parsing table.

Each module follows a **modular approach**, making it reusable and easy to debug.

---
## Sample Input
**File: `input.txt`**
```txt
E -> E + T | T
T -> T * F | F
F -> ( E ) | id
```

---
## Sample Output
**CFG after Left Factoring:**
```txt
E -> T E'
E' -> + T E' | ε
T -> F T'
T' -> * F T' | ε
F -> ( E ) | id
```

**FIRST Sets:**
```txt
FIRST(E) = { '(', id }
FIRST(T) = { '(', id }
FIRST(F) = { '(', id }
```

**FOLLOW Sets:**
```txt
FOLLOW(E) = { '$' }
FOLLOW(T) = { '+', '$' }
FOLLOW(F) = { '*', '+', '$' }
```

**LL(1) Parsing Table:**
```txt
+---+-----+-----+-----+---+
|   | (   | id  | +   | * |
+---+-----+-----+-----+---+
| E | E→TE' | E→TE' |     |   |
| T | T→FT' | T→FT' |     |   |
| F | F→(E) | F→id  |     |   |
+---+-----+-----+-----+---+
```

---
## Deliverables
- Source code (`.c` and `.h` files)
- Sample input file (`input.txt`)
- Output log (`output.txt`)
- Brief report (`report.pdf`)

---
## Challenges Faced
1. **Handling Ambiguous Grammars:** Some grammars required additional transformations.
2. **Managing Recursion in Parsing:** Implementing recursion-free transformations was tricky.
3. **Optimizing Data Structures:** Efficiently storing FIRST and FOLLOW sets improved performance.

---
## Authors
- **[Your Name]** (Roll Number 1)
- **[Your Partner’s Name]** (Roll Number 2)

---
## License
This project is for educational purposes only. Redistribution or use outside of academic evaluation is not permitted.

