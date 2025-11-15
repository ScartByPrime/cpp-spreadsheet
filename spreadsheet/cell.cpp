#include "cell.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <optional>
#include <queue>

bool Cell::CircularDependencyCheck(const Cell* start,
    const std::vector<Position>& references) const {
    // Самоссылка считается циклом
    for (const Position& ref : references) {
        if (sheet_.GetCell(ref) == start) {
            return true;
        }
    }

    std::vector<const CellInterface*> path;
    path.push_back(start);

    for (const Position& ref : references) {
        const CellInterface* cur = sheet_.GetCell(ref);
        if (cur == start) {
            continue;
        }
        // Запускаем DFS с depth = 1
        if (DFS(start, cur, 1, path, sheet_)) {
            return true;
        }
    }

    return false;
}


void Cell::InvalidateDependentsCache() {
    std::queue<const CellInterface*> q;
    std::unordered_set<const CellInterface*> visited;

    q.push(this);
    visited.insert(this);

    while (!q.empty()) {
        auto* current = q.front();
        q.pop();
        if (current) {
            current->ClearCache();

            for (const auto& dependent : static_cast<const Cell*>(current)->dependents_) {
                if (visited.insert(dependent).second) {
                    q.push(dependent);
                }
            }
        }
    }
}

void Cell::Set(std::string text) {
    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
        references_.clear();
        return;
    }

    if (text[0] == '\'') {
        try {
            auto temp = std::make_unique<TextImpl>(text, sheet_);
            impl_ = std::move(temp);
        }
        catch (const std::exception&) {
            throw;
        }
        references_.clear();
        return;
    }
     
    // формульная ячейка
    if (text.size() > 1 && text[0] == '=') {
        // временный FormulaImpl, чтобы при броске ничего не менять
        try {
            auto temp = std::make_unique<FormulaImpl>(text, sheet_);
            if (CircularDependencyCheck(this, temp->GetReferencedCells())) {
                throw CircularDependencyException("CILCULAR DEPENDENCY FOUND");
            }
            impl_ = std::move(temp);
        }
        catch (const std::exception&) {
            // не меняем impl_ и зависимости
            throw;
        }
        
        references_ = impl_->GetReferencedCells();

        for (const auto& p : references_) {
            const auto cell = sheet_.GetCell(p);
            if (!cell) {
                sheet_.SetCell(p, "");
            }
            sheet_.GetCell(p)->AddDependence(this);
        }

        InvalidateDependentsCache();
        
        return;
    }

    // ветка: только "=" не формула, а текст, или обычный текст без '=' в начале
    try {
        auto temp = std::make_unique<TextImpl>(text, sheet_);
        impl_ = std::move(temp);
    }
    catch (const std::exception&) {
        throw;
    }
    references_.clear();
}

CellInterface::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return references_;
}

void Cell::AddDependence(const CellInterface* cell) {
    dependents_.insert(cell);
}

void Cell::ClearCache() const {
    impl_->InvalidateCache();
}

void Cell::Clear() {
    Set("");
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

CellInterface::Value Cell::EmptyImpl::GetValue() const {
    return 0.0;
}

std::string Cell::EmptyImpl::GetText() const {
    return std::string{};
}

std::vector<Position> Cell::EmptyImpl::GetReferencedCells() const {
    return {};
}

Cell::TextImpl::TextImpl(std::string_view text_parsed, Sheet& sheet)
    : value_(text_parsed), sheet_(sheet) {
}

CellInterface::Value Cell::TextImpl::GetValue() const {
    if (value_[0] == '\'') {
        return value_.substr(1);
    }

    const std::string& s = value_;

    size_t i = 0;
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) ++i;
    if (i == s.size()) {
        return value_;
    }

    try {
        size_t idx = 0;
        double val = std::stod(s, &idx);

        // Проверить, что далее идут только пробелы
        size_t j = idx;
        while (j < s.size() && std::isspace(static_cast<unsigned char>(s[j]))) ++j;
        if (j != s.size()) {
            // Остались непустые символы после числа - не число
            return value_;
        }

        if (!std::isfinite(val)) {
            return FormulaError(FormulaError::Category::Value);
        }

        return val;
    }
    catch (const std::exception&) {
        // Любая ошибка парсинга - вернуть текст
        return value_;
    }
}

std::string Cell::TextImpl::GetText() const {
    return value_;
}

std::vector<Position> Cell::TextImpl::GetReferencedCells() const {
    return {};
}

Cell::FormulaImpl::FormulaImpl(std::string_view text_parsed, Sheet& sheet) try
    : formula_(ParseFormula(std::string(text_parsed.substr(1)))), sheet_(sheet){
}
catch (const FormulaException&) {
    throw;
}

CellInterface::Value Cell::FormulaImpl::GetValue() const {
    if (cache_) {
        if (std::holds_alternative<double>(cache_.value())) {
            return std::get<double>(cache_.value());
        }
        else if (std::holds_alternative<FormulaError>(cache_.value())) {
            return std::get<FormulaError>(cache_.value());
        }
    }

    try {
        auto eval_result = formula_->Evaluate(sheet_);

        cache_ = eval_result;

        if (std::holds_alternative<double>(eval_result)) {
            return std::get<double>(eval_result);
        }
        else if (std::holds_alternative<FormulaError>(eval_result)) {
            return std::get<FormulaError>(eval_result);
        }
    }
    catch (const FormulaError& er) {
        return er;
    }

    return FormulaError(FormulaError::Category::Value);
}

std::string Cell::FormulaImpl::GetText() const {
    return '=' + formula_->GetExpression();
}

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
}

void Cell::FormulaImpl::InvalidateCache() const {
    cache_.reset();
}