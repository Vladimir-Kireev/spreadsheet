#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <optional>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, const CellInterface::Value& value) {
    std::visit([&](const auto& x) { output << x; }, value);
    return output;
}

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect Position"s);
    }

    GetValidTable(pos);
    auto new_cell = std::make_unique<Cell>(*this);
    new_cell->Set(text);
    if (CyclicDependencies(pos, new_cell.get())) {
        throw CircularDependencyException("Found cyclic dependence"s);
    }
    if (table_.at(pos.row).at(pos.col)) {
        InvalidationCache(pos);
        EraseLinks(pos);
    }

    table_.at(pos.row).at(pos.col) = std::move(new_cell);
    AddLinks(pos);
    CheckSize(pos);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if(!IsValidPos(pos)){
        return nullptr;
    }
    return table_.at(pos.row).at(pos.col).get();
}

CellInterface* Sheet::GetCell(Position pos) {
    if(!IsValidPos(pos)){
        return nullptr;
    }
    return table_.at(pos.row).at(pos.col).get();
}

void Sheet::ClearCell(Position pos) {
    if(!IsValidPos(pos)){
        return;
    }
    table_.at(pos.row).at(pos.col) = nullptr;
    positions_.erase(pos);
    if (pos.row + 1 == size_.rows || pos.col + 1 == size_.cols) {
        GetNewSize();
    }
}


Size Sheet::GetPrintableSize() const {
    return Size{size_.rows, size_.cols};
}

void Sheet::PrintValues(std::ostream& output) const {
    Print(output, [](const std::unique_ptr<Cell>& cell) {return cell->GetValue();});
}

void Sheet::PrintTexts(std::ostream& output) const {
    Print(output, [](const std::unique_ptr<Cell>& cell) {return cell->GetText();});
}

void Sheet::GetValidTable(Position pos) {
    if (table_.size() <= static_cast<size_t>(pos.row)) {
        table_.resize(pos.row + 1);
    }
    if (table_.at(pos.row).size() <= static_cast<size_t>(pos.col)) {
        table_.at(pos.row).resize(pos.col + 1);
    }
}

void Sheet::CheckSize(Position pos) {
    positions_.insert(pos);
    if (size_.rows < pos.row + 1) {
        size_.rows = pos.row + 1;
    }
    if (size_.cols < pos.col + 1) {
        size_.cols = pos.col + 1;
    }
}

void Sheet::GetNewSize() {
    int new_rows = 0;
    int new_cols = 0;
    for(const auto& position : positions_) {
        new_rows = std::max(new_rows, position.row + 1);
        new_cols = std::max(new_cols, position.col + 1);
    }
    size_ = {new_rows, new_cols};
}

bool Sheet::IsValidPos(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("Incorrect Position"s);
    }
    if (pos.row + 1 > size_.rows || pos.col + 1 > size_.cols) {
        return false;
    }
    if (static_cast<size_t>(pos.row) >= table_.size() ||
        static_cast<size_t>(pos.col) >= table_.at(pos.row).size())
    {
        return false;
    }
    return true;
}

void Sheet::AddLinks(Position pos) {
    const std::vector<Position>& positions = table_[pos.row][pos.col]->GetRefToCells();
    for (const auto& position : positions) {
        
        // Если в формуле фигурирует несозданная ячейка - создаем ее
        if(positions_.find(position) == positions_.end()) {
            SetCell(position, ""s);
        }
        linked_cells_[position].insert(pos);
    }
}

void Sheet::EraseLinks(Position pos) {
    const std::vector<Position>& positions = table_[pos.row][pos.col]->GetRefToCells();
    for (const auto& position : positions) {
        linked_cells_[position].erase(pos);
    }
}

void Sheet::InvalidationCache(Position pos) {
    std::vector<Position> all_cells(linked_cells_[pos].begin(), linked_cells_[pos].end());
    std::set<Position> unique_cell = {};
    size_t i = 0;
    
    while (i < all_cells.size()) {
        Position pos_cell = all_cells[i];

        if (unique_cell.find(pos_cell) != unique_cell.end()) {
            ++i;
            continue;
        }

        const CellInterface* cell = GetCell(pos_cell);
        if(cell) {
            table_[pos_cell.row][pos_cell.col]->InvalidationCache();
        }
        if(linked_cells_.find(pos_cell) != linked_cells_.end()) {
            const auto& linked_cells_set = linked_cells_[pos_cell];
            all_cells.insert(all_cells.end(), linked_cells_set.begin(), linked_cells_set.end());
        }
        
        unique_cell.insert(pos_cell);
        ++i;
    }
}

bool Sheet::CyclicDependencies(Position pos, const Cell* cell) {
    const std::vector<Position>& cells = cell->GetRefToCells();
    std::vector<Position> all_cells(cells.begin(), cells.end());
    std::set<Position> unique_cell = {};
    size_t i = 0;
    
    while (i < all_cells.size()) {
        Position pos_cell = all_cells[i];

        if (unique_cell.find(pos_cell) != unique_cell.end()) {
            ++i;
            continue;
        }
        if (pos_cell == pos) {
            return true;
        }

        unique_cell.insert(pos_cell);
        
        const CellInterface* cell = GetCell(pos_cell);
        if(cell) {
            const std::vector<Position>& new_cells = cell->GetRefToCells();
            all_cells.insert(all_cells.end(), new_cells.begin(), new_cells.end());
        }
        
        ++i;
    }
    return false;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}