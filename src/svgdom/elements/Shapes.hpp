#pragma once

#include "Transformable.hpp"
#include "Styleable.hpp"
#include "element.hpp"
#include "rectangle.hpp"

namespace svgdom{

/**
 * @brief Element representing a geometric shape.
 */
struct Shape :
		public Element,
		public Transformable,
		public Styleable
{
};

struct PathElement : public Shape{
	struct Step{
		enum class Type_e{
			UNKNOWN,
			CLOSE,
			MOVE_ABS,
			MOVE_REL,
			LINE_ABS,
			LINE_REL,
			HORIZONTAL_LINE_ABS,
			HORIZONTAL_LINE_REL,
			VERTICAL_LINE_ABS,
			VERTICAL_LINE_REL,
			CUBIC_ABS,
			CUBIC_REL,
			CUBIC_SMOOTH_ABS,
			CUBIC_SMOOTH_REL,
			QUADRATIC_ABS,
			QUADRATIC_REL,
			QUADRATIC_SMOOTH_ABS,
			QUADRATIC_SMOOTH_REL,
			ARC_ABS,
			ARC_REL
		} type;
	
		real x, y;
		
		union{
			real x1;
			real rx;
		};
		
		union{
			real y1;
			real ry;
		};
		
		union{
			real x2;
			real xAxisRotation;
		};
		
		union{
			real y2;
			struct{
				bool largeArc;
				bool sweep;
			} flags;
		};
		
		static Type_e charToType(char c);
		static char typeToChar(Type_e t);
	};
	std::vector<Step> path;
	
	std::string pathToString()const;
	
	static decltype(path) parse(const std::string& str);
	
	void accept(visitor& v)override;
	void accept(const_visitor& v) const override;
};

struct RectElement :
		public Shape,
		public rectangle
{
	Length rx = Length::make(0, length_unit::unknown);
	Length ry = Length::make(0, length_unit::unknown);
	
	void accept(visitor& v)override;
	void accept(const_visitor& v) const override;

	static rectangle rectangleDefaultValues;
};

struct CircleElement : public Shape{
	Length cx = Length::make(0, length_unit::unknown);
	Length cy = Length::make(0, length_unit::unknown);
	Length r = Length::make(0, length_unit::unknown);
	
	void accept(visitor& v)override;
	void accept(const_visitor& v) const override;

};

struct EllipseElement : public Shape{
	Length cx = Length::make(0, length_unit::unknown);
	Length cy = Length::make(0, length_unit::unknown);
	Length rx = Length::make(0, length_unit::unknown);
	Length ry = Length::make(0, length_unit::unknown);
	
	void accept(visitor& v)override;
	void accept(const_visitor& v) const override;

};

struct LineElement : public Shape{
	Length x1 = Length::make(0, length_unit::unknown);
	Length y1 = Length::make(0, length_unit::unknown);
	Length x2 = Length::make(0, length_unit::unknown);
	Length y2 = Length::make(0, length_unit::unknown);
	
	void accept(visitor& v)override;
	void accept(const_visitor& v) const override;

};

struct PolylineShape : public Shape{
	std::vector<std::array<real, 2>> points;
	
	std::string pointsToString()const;
	
	static decltype(points) parse(const std::string& str);
};

struct PolylineElement : public PolylineShape{
	void accept(visitor& v)override;
	void accept(const_visitor& v) const override;

};

struct PolygonElement : public PolylineShape{
	void accept(visitor& v)override;
	void accept(const_visitor& v) const override;

};

}
