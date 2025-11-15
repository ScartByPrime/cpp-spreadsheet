#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <algorithm>
#include <optional>

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet)
        : impl_(std::make_unique<EmptyImpl>()),
        sheet_(sheet){
    }
    ~Cell() = default;

    void Set(std::string text) override;

    void Clear() override;

    Value GetValue() const override;

    std::vector<Position> GetReferencedCells() const override;

    void AddDependence(const CellInterface* cell) override;

    void ClearCache() const override;

    std::string GetText() const override;

private:
    class Impl;

    std::unique_ptr<Impl> impl_;
    // Ячейки, на которые ссылается текущая. Для формульной ячейки,
    // либо текстовой, которую можно интерпретировать как операнд
    std::vector<Position> references_;
    // Ячейки, которые ссылаются на текущую
    std::unordered_set<const CellInterface*> dependents_; 

    Sheet& sheet_;

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
        CellInterface::Value GetValue() const override;

        std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override;

        void InvalidateCache() const override {};
    };

    class TextImpl : public Impl {
    public:
        TextImpl(std::string_view text_parsed, Sheet& sheet);
        // Геттер, интерпретирующий, если возможно, текстовое значение в качестве операнда,
        // если нет, либо в начале следует символ "'" - возвращает текст.
        CellInterface::Value GetValue() const override;

        std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override;

        void InvalidateCache() const override {};

    private:
        std::string value_;
        Sheet& sheet_;
    };

    class FormulaImpl : public Impl {
    public:
        FormulaImpl(std::string_view text_parsed, Sheet& sheet);

        CellInterface::Value GetValue() const override;

        std::string GetText() const override;

        std::vector<Position> GetReferencedCells() const override;

        void InvalidateCache() const override;

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
    bool CircularDependencyCheck(const Cell* start, const std::vector<Position>& references) const;
};