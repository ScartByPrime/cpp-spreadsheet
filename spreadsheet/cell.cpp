#include "cell.h"

#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <optional>
#include <queue>

bool Cell::CircularDependencyCheck(const Position& start,
    const std::vector<Position>& references) const {
    // Самоссылка считается циклом
    for (const Position& ref : references) {
        if (ref == start) {
            return true;
        }
    }

    std::vector<Position> path;
    path.push_back(start);

    for (const Position& ref : references) {
        if (ref == start) {
            continue;
        }
        // Запускаем DFS с depth = 1
        if (DFS(start, ref, 1, path, sheet_)) {
            return true;
        }
    }

    return false;
}


void Cell::InvalidateDependentsCache() {
    std::queue<CellInterface*> q;
    std::unordered_set<CellInterface*> visited;

    q.push(this);
    visited.insert(this);

    while (!q.empty()) {
        auto* current = q.front();
        q.pop();
        if (current) {
            current->ClearCache();

            for (const auto& pos : static_cast<Cell*>(current)->dependents_) {
                CellInterface* dependent = sheet_.GetCell(pos);
                if (visited.insert(dependent).second) {
                    q.push(dependent);
                }
            }
        }
    }
}

void Cell::Set(std::string text) {
    references_.clear();

    if (text.empty()) {
        impl_ = std::make_unique<EmptyImpl>();
        return;
    }

    if (text[0] == '\'') {
        impl_ = std::make_unique<TextImpl>(text, sheet_);
        return;
    }
     
    // формульная ячейка
    if (text.size() > 1 && text[0] == '=') {
        // временный FormulaImpl, чтобы при броске ничего не менять
        try {
            auto temp = std::make_unique<FormulaImpl>(text, sheet_);
            if (CircularDependencyCheck(pos_, temp->GetReferencedCells())) {
                throw CircularDependencyException("CILCULAR DEPENDENCY FOUND");
            }
            impl_ = std::move(temp);
        }
        catch (const std::exception&) {
            // не меняем impl_ и зависимости
            throw;
        }
        
        references_ = impl_->GetReferencedCells();

        for (const auto& pos : references_) {
            const auto cell = sheet_.GetCell(pos);
            if (!cell) {
                sheet_.SetCell(pos, "");
            }
            sheet_.GetCell(pos)->AddDependence(pos_);
        }

        InvalidateDependentsCache();
        
        return;
    }

    // ветка: только "=" не формула, а текст, или обычный текст без '=' в начале
    impl_ = std::make_unique<TextImpl>(text, sheet_);
}

void Cell::AddDependence(const Position pos) {
    dependents_.insert(pos);
}

void Cell::RemoveDependence(const Position pos) {
    dependents_.erase(pos);
}

void Cell::ClearCache() const {
    impl_->InvalidateCache();
}

void Cell::Clear() {
    impl_.reset(new EmptyImpl());
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

std::vector<Position> Cell::FormulaImpl::GetReferencedCells() const {
    return formula_->GetReferencedCells();
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