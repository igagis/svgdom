#pragma once

#include <vector>
#include <functional>

#include "elements/Styleable.hpp"

namespace svgdom{
class StyleStack{
public:
	std::vector<std::reference_wrapper<const svgdom::Styleable>> stack;
	
	const svgdom::StyleValue* getStyleProperty(svgdom::StyleProperty_e p);
	
	class Push{
		StyleStack& ss;
	public:
		Push(StyleStack& ss, const svgdom::Styleable& s);
		~Push()noexcept;
	};
};
}