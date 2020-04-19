#pragma once

#include "Element.hpp"
#include "Styleable.hpp"
#include "Transformable.hpp"
#include "Rectangle.hpp"
#include "Referencing.hpp"
#include "aspect_ratioed.hpp"

namespace svgdom{
struct ImageElement :
		public Element,
		public Styleable,
		public Transformable,
		public Rectangle,
		public Referencing,
		public AspectRatioed
{
	void accept(visitor& v) override;
	void accept(const_visitor& v) const override;
};
}