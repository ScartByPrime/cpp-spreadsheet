#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <algorithm>
#include <optional>

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet, Position pos)
        : impl_(std::make_unique<EmptyImpl>()),
        sheet_(sheet),
        pos_(std::move(pos)){
    }
    ~Cell() = default;

    void Set(std::string text) override;

    void Clear();

    Value GetValue() const override {
        return impl_->GetValue();
    }
    std::vector<Position> GetReferencedCells() const override {
        return references_;
    }

    void AddDependence(const Position pos) override;

    void RemoveDependence(const Position pos) override;

    void ClearCache() const override;

    std::string GetText() const override {
        return impl_->GetText();
    }

private:
    class Impl;

    std::unique_ptr<Impl> impl_;
    // Ячейки, на которые ссылается текущая. Для формульной ячейки,
    // либо текстовой, которую можно интерпретировать как операнд
    std::vector<Position> references_;
    // Ячейки, которые ссылаются на текущую
    std::unordered_set<Position> dependents_; 

    Sheet& sheet_;
    const Position pos_;

    class Impl {
    public:
        virtual ~Impl() = default;
        virtual CellInterface::Value GetValue() const = 0;
        virtual std::string GetText() const = 0;
        virtual std::vector<Position> GetReferencedCells() const = 0;
        virtual void InvalidateCache() const = 0;
    };

    class EmptyImpl : public Impl {
    public:
        CellInterface::Value GetValue() const override {
            return 0.0;
        }

        std::string GetText() const override {
            return std::string{};
        }

        std::vector<Position> GetReferencedCells() const override { 
            return {};
        }

        void InvalidateCache() const override {};
    };

    class TextImpl : public Impl {
    public:
        TextImpl(std::string_view text_parsed, Sheet& sheet)
            : value_(text_parsed), sheet_(sheet) {
        }
        // Геттер, интерпретирующий, если возможно, текстовое значение в качестве операнда,
        // если нет, либо в начале следует символ "'" - возвращает текст.
        CellInterface::Value GetValue() const override;

        std::string GetText() const override {
            return value_;
        }

        std::vector<Position> GetReferencedCells() const override {
            return {};
        }

        void InvalidateCache() const override {};

    private:
        std::string value_;
        Sheet& sheet_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string_view text_parsed, Sheet& sheet);

        CellInterface::Value GetValue() const override;

        std::string GetText() const override {
            return '=' + formula_->GetExpression();
        }

        std::vector<Position> GetReferencedCells() const override;

        void InvalidateCache() const override {
            cache_.reset();
        }

    private:
        std::unique_ptr<FormulaInterface> formula_;
        Sheet& sheet_;
        // Кэш формульной ячейки, вычисляется в GetValue, 
        // инвалидируется при изменении ячейки, от которой зависит экземпляр.
        // nullopt указывает на необходимость вычисления
        mutable std::optional<FormulaInterface::Value> cache_;
    };
    // Вспомогательная ф-я для инвалидации кэша всех зависимых ячеек
    void InvalidateDependentsCache();

    // Вспомогательная ф-я для поиска циклических зависимостей
    bool CircularDependencyCheck(const Position& start, const std::vector<Position>& references) const;
};