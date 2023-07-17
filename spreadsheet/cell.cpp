#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>
#include <cmath>


class Cell::EmptyImpl : public Impl{
public:
    EmptyImpl() = default;
    
    Cell::Value GetValue() override {
        return 0.0;
    }
    
    std::string GetText() const override {
        return value_;
    }

    std::vector<Position> GetReferencedCells() const override {
        return empty_positions_;
    }
    
    const std::vector<Position>& GetRefToCells() const override {
        return empty_positions_;
    }

    void InvalidationCache() override {
        return;
    }
    
private:
    std::string value_ = "";
    std::vector<Position> empty_positions_ = {};
};

class Cell::TextImpl : public Impl{
public:
    TextImpl() = default;
    
    explicit TextImpl(std::string text) : value_(std::move(text)){
    }
    
    Cell::Value GetValue() override {
        if(value_[0] == '\'') {
            return std::string(value_.begin() + 1, value_.end());
        }
        return value_;
    }
    
    std::string GetText() const override {
        return value_;
    }

    std::vector<Position> GetReferencedCells() const override {
        return empty_positions_;
    }
    
    const std::vector<Position>& GetRefToCells() const override {
        return empty_positions_;
    }

    void InvalidationCache() override {
        return;
    }
    
private:
    std::string value_;
    std::vector<Position> empty_positions_ = {};
};

class Cell::FormulaImpl : public Impl{
public:
    FormulaImpl() = default;
    
    FormulaImpl(std::string text, SheetInterface& sheet)
        : value_(ParseFormula(std::move(text)))
        , sheet_(sheet){
    }

    Value GetValue() override {
        if (cache_) {
            return *cache_;
        }
        FormulaInterface::Value result = value_->Evaluate(sheet_);
        if(FormulaError* error_result = std::get_if<FormulaError>(&result)){
            cache_ = *error_result;
        }
        else {
            if(std::isfinite(std::get<double>(result))) {
                cache_ = std::get<double>(result);
            } else {
                cache_ = FormulaError(FormulaError::Category::Div0);
            }
        }

        return *cache_;
    }
    
    std::string GetText() const override {
        std::string result = '=' + value_->GetExpression();
        return result;
    }

    std::vector<Position> GetReferencedCells() const override {
        return value_->GetReferencedCells();
    }
    
    const std::vector<Position>& GetRefToCells() const override {
        return value_->GetRefToCells();
    }

    void InvalidationCache() override {
        cache_.reset();
    }
    
private:
    std::unique_ptr<FormulaInterface> value_;
    SheetInterface& sheet_;
    std::optional<Value> cache_;
};



Cell::Cell(SheetInterface& sheet) : impl_(std::make_unique<EmptyImpl>()), sheet_(sheet){
}

void Cell::Set(std::string text) {
    if(text.empty()){
        impl_ = std::make_unique<EmptyImpl>();
    } else if(text[0] == '=' && text.size() > 1) {
        try{
            std::string formula_str(text.begin() + 1, text.end());
            impl_ = std::make_unique<FormulaImpl>(std::move(formula_str), sheet_);
        } catch (...) {
            throw FormulaException("Formula parsing error");
            //throw FormulaError(FormulaError::Category::Ref);
        }
    } else {
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    return impl_->GetReferencedCells();
}

const std::vector<Position>& Cell::GetRefToCells() const {
    return impl_->GetRefToCells();
}

void Cell::InvalidationCache() {
    impl_->InvalidationCache();
}
