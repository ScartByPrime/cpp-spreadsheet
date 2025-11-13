#include "formula.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, const FormulaError& fe) {
    return output << "#ARITHM!";
}
Formula::Formula(std::string expression) try
    :ast_(ParseFormulaAST(expression)) {
}
catch (const std::exception&) {
    throw FormulaException("INCORRECT FORMULA");
}

FormulaInterface::Value Formula::Evaluate(const SheetInterface& sheet) const {
    double result = 0.0;
    try {
        result = ast_.Execute(sheet);
    }
    catch (const FormulaError& fe) {
        return fe;
    }
    return result;
}

std::string Formula::GetExpression() const {
    std::ostringstream ostr;
    ast_.PrintFormula(ostr);
    return ostr.str();
}

std::vector<Position> Formula::GetReferencedCells() const {
    const std::forward_list<Position>& cells = ast_.GetCells();
    std::vector<Position> result;
    std::unordered_set<Position> seen;

    for (const auto& x : cells) {
        if (seen.insert(x).second) {  // если элемент новый
            result.push_back(x);
        }
    }

    return result;
}

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}