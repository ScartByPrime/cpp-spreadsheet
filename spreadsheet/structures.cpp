#include "common.h"

#include <cctype>
#include <sstream>
#include <algorithm>

const int LETTERS = 26;
const int MAX_POSITION_LENGTH = 17;
const int MAX_POS_LETTER_COUNT = 3;

const Position Position::NONE = { -1, -1 };

bool Position::operator==(const Position rhs) const {
    return row == rhs.row && col == rhs.col;
}
bool Position::operator!=(const Position rhs) const {
    return !(*this == rhs);
}

bool Position::operator<(const Position rhs) const {
    return std::tie(row, col) < std::tie(rhs.row, rhs.col);
}

bool Position::IsValid() const {
    return row >= 0 && col >= 0 && row < MAX_ROWS && col < MAX_COLS;
}

std::string Position::ToString() const {
    if (!IsValid()) {
        return "";
    }

    std::string result;
    result.reserve(MAX_POSITION_LENGTH);
    int c = col;
    while (c >= 0) {
        result.insert(result.begin(), 'A' + c % LETTERS);
        c = c / LETTERS - 1;
    }

    result += std::to_string(row + 1);

    return result;
}

Position Position::FromString(std::string_view str) {
    auto it = std::find_if(str.begin(), str.end(), [](const char c) {
        return !(std::isalpha(c) && std::isupper(c));
        });
    auto letters = str.substr(0, it - str.begin());
    auto digits = str.substr(it - str.begin());

    if (letters.empty() || digits.empty()) {
        return Position::NONE;
    }
    if (letters.size() > MAX_POS_LETTER_COUNT) {
        return Position::NONE;
    }

    if (!std::isdigit(digits[0])) {
        return Position::NONE;
    }

    int row;
    std::istringstream row_in{ std::string{digits} };
    if (!(row_in >> row) || !row_in.eof()) {
        return Position::NONE;
    }

    int col = 0;
    for (char ch : letters) {
        col *= LETTERS;
        col += ch - 'A' + 1;
    }

    return { row - 1, col - 1 };
}

bool Size::operator==(Size rhs) const {
    return cols == rhs.cols && rows == rhs.rows;
}

FormulaError::FormulaError(Category category)
:category_(std::move(category)) {
}

FormulaError::Category FormulaError::GetCategory() const {
    return category_;
}

bool FormulaError::operator==(FormulaError rhs) const {
    return category_ == rhs.category_;
}

std::string_view FormulaError::ToString() const {
    switch (category_) {
    case FormulaError::Category::Arithmetic:
        return "Arithmetic";
    case FormulaError::Category::Ref:
        return "Ref";
    case FormulaError::Category::Value:
        return "Value";
     }
    return "";
}

bool DFS(const CellInterface* start,
    const CellInterface* cur,
    int depth,
    std::vector<const CellInterface*>& path,
    const SheetInterface& sheet) {
    // Если уже в пути — не углубляемся (предотвращаем прогулку по уже пройденному пути).
    if (std::find(path.begin(), path.end(), cur) != path.end()) {
        return false;
    }

    path.push_back(cur);

    if (cur) {
        const std::vector<Position> neigh = cur->GetReferencedCells();
        for (const Position& next : neigh) {
            // Наткнулись на старт — определяем длину цикла:
            // depth + 1 т.к. добавляется ребро cur->start.
            if (sheet.GetCell(next) == start) {
                if (depth + 1 >= 3) { // учитываем только циклы длины >= 3
                    path.pop_back();
                    return true;
                }
                else {
                    continue;
                }
            }

            // Если next уже в текущем пути — пропускаем, чтобы не зациклиться внутри пути
            if (std::find(path.begin(), path.end(), sheet.GetCell(next)) != path.end()) {
                continue;
            }

            if (DFS(start, sheet.GetCell(next), depth + 1, path, sheet)) {
                path.pop_back();
                return true;
            }
        }
    }

    path.pop_back();
    return false;
}