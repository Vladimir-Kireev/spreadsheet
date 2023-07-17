#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>
#include <forward_list>
#include <set>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
class Formula : public FormulaInterface {
public:
    explicit Formula(std::string expression) : ast_(ParseFormulaAST(expression)) {
        const auto list = ast_.GetCells();
        std::set<Position> set_list(list.begin(), list.end());
        positions_ = { set_list.begin(), set_list.end() };
    }
    
    Value Evaluate(const SheetInterface& sheet) const override {
        std::function<double(Position)> func = [&sheet](Position pos) {
            auto cell = sheet.GetCell(pos);
            if (!cell) {
                return 0.0;
            }

            CellInterface::Value value = cell->GetValue();
            if (std::string* string_value = std::get_if<std::string>(&value)) {
                try {
                    double result = std::stod(*string_value);
                    return result;
                }
                catch (...) {
                    throw FormulaError(FormulaError::Category::Value);
                }
            }
            if (double* double_value = std::get_if<double>(&value)) {
                return *double_value;
            }
            throw std::get<FormulaError>(value);
        };

        try{
            return ast_.Execute(func);
        } catch (const FormulaError& err) {
            return err;
        }
    }
    
    std::string GetExpression() const override {
        std::stringstream strm;
        ast_.PrintFormula(strm);
        return strm.str();
    }
    
    std::vector<Position> GetReferencedCells() const override {
        return positions_;
    }

    const std::vector<Position>& GetRefToCells() const override {
        return positions_;
    }

private:
    FormulaAST ast_;
    std::vector<Position> positions_;
};
}  // namespace

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    try{
        return std::make_unique<Formula>(std::move(expression));
    } catch (...) {
        throw FormulaException("Formula parsing error");
    }
}