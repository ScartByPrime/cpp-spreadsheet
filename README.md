Spreadsheet
A lightweight alternative to existing solutions (Microsoft Excel/Google Sheets spreadsheets), implemented entirely in C++ with a focus on formulas.

Solution
- Implemented formula parsing and computational logic using ANTLR (lexer, parser, and AST traversal in C++).
- Implemented cyclic dependency detection with optimizations ensuring each cell is only iterated once via DFS.
- Cache invalidation optimization with cell iterations only once implemented via BFS.
- Build - CMake (CMake Lists included)

Result
Developed an efficient, lightweight, and resource-efficient spreadsheet solution that supports the customer's preferred features:
- Text and formula parsing;
- Adding a new cell, ensuring a consistent table state.
- Optimizing the table using a formula cell cache. - Create links, eliminating circular dependencies.
- Interpret text cells for use in formulas.
- Visualize exceptions, as well as cell data and calculation results, as text.
