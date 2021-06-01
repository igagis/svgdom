#pragma once

#include <istream>
#include <array>
#include <string_view>

#include <r4/vector.hpp>

#include "config.hpp"
#include "elements/coordinate_units.hpp"

namespace svgdom{

class string_parser{
    std::string_view view;

public:
    string_parser(std::string_view view) :
            view(view)
    {}

    void skip_whitespaces();
    void skip_whitespaces_and_comma();

    std::string_view read_word();

    // skips leading whitespaces
    template <class real_type>
    real_type read_real(){
        real_type ret;

        char* end;

        if constexpr (std::is_same<real_type, float>::value){
            ret = real_type(std::strtof(this->view.data(), &end));
        }else if constexpr (std::is_same<real_type, double>::value){
            ret = real_type(std::strtod(this->view.data(), &end));
        }else{
            ret = real_type(std::strtold(this->view.data(), &end));
        }

        if(end == this->view.data()){
            throw std::invalid_argument("string_parser::read_real(): could not parse real number");
        }

        this->view = std::string_view(end, this->view.data() + this->view.size() - end);

        return ret;
    }

    char read_char();

    std::string_view read_chars(size_t n);

    bool empty()const noexcept{
        return this->view.empty();
    }
};

void skip_whitespaces(std::istream& s);

void skip_whitespaces_and_comma(std::istream& s);

void skip_till_char_inclusive(std::istream& s, char c);

std::string read_till_char(std::istream& s, char c);

std::string read_till_char_or_whitespace(std::istream& s, char c);

real read_in_real(std::istream& s);

std::string trim_tail(const std::string& s);

std::string iri_to_local_id(const std::string& iri);

coordinate_units parse_coordinate_units(const std::string& s);

std::string coordinate_units_to_string(coordinate_units u);

r4::vector2<real> parse_number_and_optional_number(std::string_view s, r4::vector2<real> defaults);

std::string number_and_optional_number_to_string(std::array<real, 2> non, real optionalNumberDefault);

}
