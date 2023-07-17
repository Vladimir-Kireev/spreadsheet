#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <set>
#include <unordered_map>
#include <iostream>

struct PosHasher {
    size_t operator()(Position pos) const {
        return hasher_(&pos.row) * 37 + hasher_(&pos.col);
    }

private:
    std::hash<const void*> hasher_;
};

class Sheet : public SheetInterface {
public:
    ~Sheet() = default;

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

private:
    void GetValidTable(Position pos);
    void CheckSize(Position pos);
    void GetNewSize();
    bool IsValidPos(Position pos) const;
    
    template<typename Func>
    void Print(std::ostream& output, Func foo) const;
    
    void AddLinks(Position pos);
    void EraseLinks(Position pos);
    void InvalidationCache(Position pos);
    bool CyclicDependencies(Position pos, const Cell* cell);

    std::vector<std::vector<std::unique_ptr<Cell>>> table_;
    Size size_;
    std::set<Position> positions_;
    std::unordered_map<Position, std::set<Position>, PosHasher> linked_cells_;
};

template<typename Func>
void Sheet::Print(std::ostream& output, Func foo) const {
    using namespace std::literals;
    for (int i = 0; i < size_.rows; ++i) {
        for (int j = 0; j < size_.cols - 1; ++j) {
            if (static_cast<size_t>(j) >= table_[i].size() || !table_[i][j]) {
                output << '\t';
                continue;
            }
            output << foo(table_[i][j]) << '\t';
        }
        output << (static_cast<size_t>(size_.cols - 1) >= table_[i].size() ? ""s : foo(table_[i][size_.cols - 1])) << '\n';
    }
}