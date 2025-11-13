#pragma once

#include "common.h"

#include <functional>

inline std::ostream& operator<<(std::ostream& out, const CellInterface::Value& val) {
    std::visit(
        [&](const auto& x) {
            out << x;
        },
        val);
    return out;
}

class Sheet : public SheetInterface {
public:
    Sheet() = default;
    ~Sheet();

    Sheet(const Sheet&) = delete;
    Sheet& operator=(const Sheet&) = delete;

    Sheet(Sheet&&) = default;
    Sheet& operator=(Sheet&&) = default;

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    std::unordered_map<Position, std::unique_ptr<CellInterface>> printable_;
};