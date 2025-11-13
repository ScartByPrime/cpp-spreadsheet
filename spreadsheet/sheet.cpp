#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() {}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("INVALID POSITION");
    }
    try {
        printable_.emplace(pos, std::make_unique<Cell>(*this, pos));
        printable_.at(pos)->Set(std::move(text));
    }
    catch (const FormulaException&) {
        throw;
    }
}


const CellInterface* Sheet::GetCell(Position pos) const {
    return const_cast<Sheet*>(this)->GetCell(pos);
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("INVALID POSITION");
    }

    auto it = printable_.find(pos);
    if (it == printable_.end()) {
        return nullptr;
    }
    return it->second.get();
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("INVALID POSITION");
    }

    auto it = printable_.find(pos);
    if (it != printable_.end()) {
        printable_.erase(it);
    }
}

Size Sheet::GetPrintableSize() const {
    Size result;

    if (printable_.empty()) {
        return result;
    }

    int max_row = -1;
    int max_col = -1;
    for (const auto& kv : printable_) {
        const Position& p = kv.first;
        if (p.row > max_row) max_row = p.row;
        if (p.col > max_col) max_col = p.col;
    }

    if (max_row >= 0) result.rows = max_row + 1;
    if (max_col >= 0) result.cols = max_col + 1;

    return result;
}

void Sheet::PrintValues(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int i = 0; i < size.rows; ++i) {
        for (int j = 0; j < size.cols; ++j) {
            const CellInterface* cell = GetCell({ i, j });
            if (cell) {
                output << cell->GetValue();
            }
            if (j + 1 < size.cols) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    Size size = GetPrintableSize();
    for (int i = 0; i < size.rows; ++i) {
        for (int j = 0; j < size.cols; ++j) {
            const CellInterface* cell = GetCell({ i, j });
            if (cell) {
                output << cell->GetText();
            }
            if (j + 1 < size.cols) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}
