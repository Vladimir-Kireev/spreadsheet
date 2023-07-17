#pragma once

#include "common.h"
#include "formula.h"

#include <functional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(SheetInterface& sheet);
    ~Cell() = default;

    void Set(std::string text);
    void Clear();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;
    const std::vector<Position>& GetRefToCells() const override;
    void InvalidationCache();

private:
    class Impl{
    public:
        Impl() = default;
        virtual ~Impl() = default;

        virtual Cell::Value GetValue() = 0;
        virtual std::string GetText() const = 0;

        virtual std::vector<Position> GetReferencedCells() const = 0;
        virtual const std::vector<Position>& GetRefToCells() const = 0;
        virtual void InvalidationCache() = 0;
    };
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    std::unique_ptr<Impl> impl_;
    SheetInterface& sheet_;
};