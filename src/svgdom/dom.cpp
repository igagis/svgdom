#include "dom.hpp"
#include "config.hpp"
#include "Exc.hpp"

#include <pugixml.hpp>

#include <map>
#include <sstream>
#include <iomanip>

using namespace svgdom;


namespace{

void skipWhitespacesAndOrComma(std::istream& s){
	bool commaSkipped = false;
	while(!s.eof()){
		if(std::isspace(s.peek())){
			s.get();
		}else if(s.peek() == ','){
			if(commaSkipped){
				break;
			}
			s.get();
			commaSkipped = true;
		}else{
			break;
		}
	}
}

std::string readTillCharOrWhitespace(std::istream& s, char c){
	std::stringstream ss;
	while(!s.eof()){
		if(std::isspace(s.peek()) || s.peek() == c || s.peek() == std::char_traits<char>::eof()){
			break;
		}
		ss << char(s.get());
	}
	return ss.str();
}

std::string readTillChar(std::istream& s, char c){
	std::stringstream ss;
	while(!s.eof()){
		if(s.peek() == c || s.peek() == std::char_traits<char>::eof()){
			break;
		}
		ss << char(s.get());
	}
	return ss.str();
}


void skipTillCharInclusive(std::istream& s, char c){
	while(!s.eof()){
		if(s.get() == c){
			break;
		}
	}
}

void skipWhitespaces(std::istream& s){
	while(!s.eof()){
		if(!std::isspace(s.peek())){
			break;
		}
		s.get();
	}
}

enum class EXmlNamespace{
	UNKNOWN,
	SVG,
	XLINK
};

const std::string DSvgNamespace = "http://www.w3.org/2000/svg";
const std::string DXlinkNamespace = "http://www.w3.org/1999/xlink";


struct Parser{
	typedef std::map<std::string, EXmlNamespace> T_NamespaceMap;
	std::vector<T_NamespaceMap> namespaces;
	
	std::vector<EXmlNamespace> defaultNamespace;
	
	
	EXmlNamespace findNamespace(const std::string& ns){
		for(auto i = this->namespaces.rbegin(); i != this->namespaces.rend(); ++i){
			auto iter = i->find(ns);
			if(iter == i->end()){
				continue;
			}
			ASSERT(ns == iter->first)
			return iter->second;
		}
		return EXmlNamespace::UNKNOWN;
	}
	
	struct NamespaceNamePair{
		EXmlNamespace ns;
		std::string name;
	};
	
	NamespaceNamePair getNamespace(const std::string& fullName){
		NamespaceNamePair ret;
		
		auto colonIndex = fullName.find_first_of(':');
		if(colonIndex == std::string::npos){
			ret.ns = this->defaultNamespace.back();
			ret.name = fullName;
			return ret;
		}
		
		ASSERT(fullName.length() >= colonIndex + 1)
		
		ret.ns = this->findNamespace(fullName.substr(0, colonIndex));
		ret.name = fullName.substr(colonIndex + 1, fullName.length() - 1 - colonIndex);
		
		return ret;
	}
	
	std::unique_ptr<svgdom::Element> parseNode(const pugi::xml_node& n);

	void fillElement(Element& e, const pugi::xml_node& n){
		for(auto a = n.first_attribute(); !a.empty(); a = a.next_attribute()){
			auto nsn = this->getNamespace(a.name());
			switch(nsn.ns){
				case EXmlNamespace::SVG:
					if(nsn.name == "id"){
						e.id = a.value();
					}
					break;
				default:
					break;
			}
		}
	}
	
	void fillReferencing(Referencing& e, const pugi::xml_node& n){
		for(auto a = n.first_attribute(); !a.empty(); a = a.next_attribute()){
			auto nsn = this->getNamespace(a.name());
			switch(nsn.ns){
				case EXmlNamespace::XLINK:
					if(nsn.name == "href"){
						e.iri = a.value();
					}
					break;
				default:
					break;
			}
		}
	}

	void fillRectangle(Rectangle& r, const pugi::xml_node& n){
		for(auto a = n.first_attribute(); !a.empty(); a = a.next_attribute()){
			auto nsn = this->getNamespace(a.name());
			switch(nsn.ns){
				case EXmlNamespace::SVG:
					if(nsn.name == "x"){
						r.x = Length::parse(a.value());
					}else if(nsn.name == "y"){
						r.y = Length::parse(a.value());
					}else if(nsn.name == "width"){
						r.width = Length::parse(a.value());
					}else if(nsn.name == "height"){
						r.height = Length::parse(a.value());
					}
					break;
				default:
					break;
			}
		}
	}
	
	void fillContainer(Container& c, const pugi::xml_node& n){
		ASSERT(c.children.size() == 0)
		for(auto i = n.first_child(); !i.empty(); i = i.next_sibling()){
			if(auto res = this->parseNode(i)){
				c.children.push_back(std::move(res));
				c.children.back()->parent = &c;
			}
		}
	}
	
	void fillTransformable(Transformable& t, const pugi::xml_node& n){
		ASSERT(t.transformations.size() == 0)
		
		for(auto a = n.first_attribute(); !a.empty(); a = a.next_attribute()){
			auto nsn = this->getNamespace(a.name());
			switch(nsn.ns){
				case EXmlNamespace::SVG:
					if(nsn.name == "transform"){
						t.transformations = Transformable::parse(a.value());
					}
					break;
				default:
					break;
			}
		}
	}
	
	void fillStyleable(Styleable& s, const pugi::xml_node& n){
		ASSERT(s.styles.size() == 0)
		
		for(auto a = n.first_attribute(); !a.empty(); a = a.next_attribute()){
			auto nsn = this->getNamespace(a.name());
			switch(nsn.ns){
				case EXmlNamespace::SVG:
					if(nsn.name == "style"){
						s.styles = Styleable::parse(a.value());
					}
					break;
				default:
					break;
			}
		}
	}
	
	void fillGradient(Gradient& g, const pugi::xml_node& n){
		for(auto a = n.first_attribute(); !a.empty(); a = a.next_attribute()){
			auto nsn = this->getNamespace(a.name());
			switch(nsn.ns){
				case EXmlNamespace::SVG:
					if(nsn.name == "spreadMethod"){
						g.spreadMethod = Gradient::stringToSpreadMethod(a.value());
					}else if(nsn.name == "gradientTransform"){
						//TODO:
					}
					break;
				default:
					break;
			}
		}
	}
	
	std::unique_ptr<Gradient::StopElement> parseGradientStopElement(const pugi::xml_node& n){
		ASSERT(getNamespace(n.name()).ns == EXmlNamespace::SVG)
		ASSERT(getNamespace(n.name()).name == "stop")
		
		auto ret = utki::makeUnique<Gradient::StopElement>();
		this->fillStyleable(*ret, n);
		
		for(auto a = n.first_attribute(); !a.empty(); a = a.next_attribute()){
			auto nsn = this->getNamespace(a.name());
			switch(nsn.ns){
				case EXmlNamespace::SVG:
					if(nsn.name == "offset"){
						std::istringstream s(a.value());
						s >> ret->offset;
						if(!s.eof() && s.peek() == '%'){
							ret->offset /= 100;
						}
					}
					break;
				default:
					break;
			}
		}
		
		return ret;
	}
	
	std::unique_ptr<SvgElement> parseSvgElement(const pugi::xml_node& n){
		ASSERT(getNamespace(n.name()).ns == EXmlNamespace::SVG)
		ASSERT(getNamespace(n.name()).name == "svg")
		
		auto ret = utki::makeUnique<SvgElement>();
		
		this->fillElement(*ret, n);
		this->fillRectangle(*ret, n);
		this->fillContainer(*ret, n);

		return ret;
	}
	
	std::unique_ptr<GElement> parseGElement(const pugi::xml_node& n){
		ASSERT(getNamespace(n.name()).ns == EXmlNamespace::SVG)
		ASSERT(getNamespace(n.name()).name == "g")
		
		auto ret = utki::makeUnique<GElement>();
		
		this->fillElement(*ret, n);
		this->fillTransformable(*ret, n);
		this->fillStyleable(*ret, n);
		this->fillContainer(*ret, n);
		
		return ret;
	}
	
	std::unique_ptr<DefsElement> parseDefsElement(const pugi::xml_node& n){
		ASSERT(getNamespace(n.name()).ns == EXmlNamespace::SVG)
		ASSERT(getNamespace(n.name()).name == "defs")
		
		auto ret = utki::makeUnique<DefsElement>();
		
		this->fillElement(*ret, n);
		this->fillTransformable(*ret, n);
		this->fillStyleable(*ret, n);
		this->fillContainer(*ret, n);
		
		return ret;
	}
	
	std::unique_ptr<PathElement> parsePathElement(const pugi::xml_node& n){
		ASSERT(getNamespace(n.name()).ns == EXmlNamespace::SVG)
		ASSERT(getNamespace(n.name()).name == "path")
		
		auto ret = utki::makeUnique<PathElement>();
		
		this->fillElement(*ret, n);
		this->fillTransformable(*ret, n);
		this->fillStyleable(*ret, n);

		for(auto a = n.first_attribute(); !a.empty(); a = a.next_attribute()){
			auto nsn = this->getNamespace(a.name());
			switch(nsn.ns){
				case EXmlNamespace::SVG:
					if(nsn.name == "d"){
						ret->path = PathElement::parse(a.value());
						return ret;
					}
					break;
				default:
					break;
			}
		}
		
		return ret;
	}
	
	std::unique_ptr<LinearGradientElement> parseLinearGradientElement(const pugi::xml_node& n){
		ASSERT(getNamespace(n.name()).ns == EXmlNamespace::SVG)
		ASSERT(getNamespace(n.name()).name == "linearGradient")
		
		auto ret = utki::makeUnique<LinearGradientElement>();
		
		this->fillElement(*ret, n);
		this->fillContainer(*ret, n);
		this->fillGradient(*ret, n);
		this->fillReferencing(*ret, n);

		for(auto a = n.first_attribute(); !a.empty(); a = a.next_attribute()){
			auto nsn = this->getNamespace(a.name());
			switch(nsn.ns){
				case EXmlNamespace::SVG:
					//TODO:
//					if(nsn.name == "d"){
//						ret->path = PathElement::parse(a.value());
//						return ret;
//					}
					break;
				default:
					break;
			}
		}
		
		return ret;
	}
};//~class


std::unique_ptr<svgdom::Element> Parser::parseNode(const pugi::xml_node& n){
	//parse default namespace
	{
		pugi::xml_attribute dn = n.attribute("xmlns");
		if(!dn.empty()){
			if(std::string(dn.value()) == DSvgNamespace){
				this->defaultNamespace.push_back(EXmlNamespace::SVG);
			}else{
				this->defaultNamespace.push_back(EXmlNamespace::UNKNOWN);
			}
		}else{
			this->defaultNamespace.push_back(this->defaultNamespace.back());
		}
	}
	
	//parse other namespaces
	{
		std::string xmlns = "xmlns:";
		
		this->namespaces.push_back(T_NamespaceMap());
		
		for(auto a = n.first_attribute(); !a.empty(); a = a.next_attribute()){
			auto attr = std::string(a.name());
			
			if(attr.substr(0, xmlns.length()) != xmlns){
				continue;
			}
			
			ASSERT(attr.length() >= xmlns.length())
			auto ns = attr.substr(xmlns.length(), attr.length() - xmlns.length());
			
			if(DSvgNamespace == a.value()){
				this->namespaces.back()[ns] = EXmlNamespace::SVG;
			}else if(DXlinkNamespace == a.value()){
				this->namespaces.back()[ns] = EXmlNamespace::XLINK;
			}
		}
	}
	
	utki::ScopeExit scopeExit([this](){
		ASSERT(this->namespaces.size() > 0)
		this->namespaces.pop_back();
		ASSERT(this->defaultNamespace.size() > 0)
		this->defaultNamespace.pop_back();
	});
	
	auto nsn = getNamespace(n.name());
	switch(nsn.ns){
		case EXmlNamespace::SVG:
			if(nsn.name == "svg"){
				return this->parseSvgElement(n);
			}else if(nsn.name == "g"){
				return this->parseGElement(n);
			}else if(nsn.name == "defs"){
				return this->parseDefsElement(n);
			}else if(nsn.name == "path"){
				return this->parsePathElement(n);
			}else if(nsn.name == "linearGradient"){
				return this->parseLinearGradientElement(n);
			}else if(nsn.name == "stop"){
				return this->parseGradientStopElement(n);
			}
			
			break;
		default:
			//unknown namespace, ignore
			break;
	}
	
	return nullptr;
}

}//~namespace



std::unique_ptr<SvgElement> svgdom::load(const papki::File& f){
	pugi::xml_document doc;
	{
		auto fileContents = f.loadWholeFileIntoMemory();
		if(doc.load_buffer(&*fileContents.begin(), fileContents.size()).status != pugi::xml_parse_status::status_ok){
			TRACE(<< "svgdom::load(): loading XML document failed!" << std::endl)
			return nullptr;
		}
	}
	
	Parser parser;
	
	//return first node which is successfully parsed
	for(auto n = doc.first_child(); !n.empty(); n = n.next_sibling()){
		auto element = parser.parseNode(doc.first_child());
	
		auto ret = std::unique_ptr<SvgElement>(dynamic_cast<SvgElement*>(element.release()));
		if(ret){
			return ret;
		}
	}
	return nullptr;
}



Length Length::parse(const std::string& str) {
	Length ret;

	std::istringstream ss(str);
	
	ss >> std::skipws;
	
	ss >> ret.value;
	
	std::string u;
	
	ss >> std::setw(2) >> u >> std::setw(0);
	
	if(u.length() == 0){
		ret.unit = Length::EUnit::NUMBER;
	}else if(u == "%"){
		ret.unit = Length::EUnit::PERCENT;
	}else if(u == "em"){
		ret.unit = Length::EUnit::EM;
	}else if(u == "ex"){
		ret.unit = Length::EUnit::EX;
	}else if(u == "px"){
		ret.unit = Length::EUnit::PX;
	}else if(u == "cm"){
		ret.unit = Length::EUnit::CM;
	}else if(u == "in"){
		ret.unit = Length::EUnit::IN;
	}else if(u == "pt"){
		ret.unit = Length::EUnit::PT;
	}else if(u == "pc"){
		ret.unit = Length::EUnit::PC;
	}else{
		ret.unit = Length::EUnit::UNKNOWN;
	}
	
	return ret;
}



std::ostream& operator<<(std::ostream& s, const Length& l){
	s << l.value;
	
	switch(l.unit){
		case Length::EUnit::UNKNOWN:
		case Length::EUnit::NUMBER:
		default:
			break;
		case Length::EUnit::PERCENT:
			s << "%";
			break;
		case Length::EUnit::EM:
			s << "em";
			break;
		case Length::EUnit::EX:
			s << "ex";
			break;
		case Length::EUnit::PX:
			s << "px";
			break;
		case Length::EUnit::CM:
			s << "cm";
			break;
		case Length::EUnit::IN:
			s << "in";
			break;
		case Length::EUnit::PT:
			s << "pt";
			break;
		case Length::EUnit::PC:
			s << "pc";
			break;
	}
	
	return s;
}



void Element::attribsToStream(std::ostream& s) const{
	if(this->id.length() != 0){
		s << " id=\"" << this->id << "\"";
	}
}



void Rectangle::attribsToStream(std::ostream& s)const{
	if(this->x.value != 0){
		s << " x=\"" << this->x << "\"";
	}
	
	if(this->y.value != 0){
		s << " y=\"" << this->y << "\"";
	}
	
	if(this->width.value != 100 || this->width.unit != Length::EUnit::PERCENT){ //if width is not 100% (default value)
		s << " width=\"" << this->width << "\"";
	}
	
	if(this->height.value != 100 || this->height.unit != Length::EUnit::PERCENT){ //if height is not 100% (default value)
		s << " height=\"" << this->height << "\"";
	}
}


namespace{

std::string indentStr(unsigned indent){
	std::string ind;

	std::stringstream ss;
	for(unsigned i = 0; i != indent; ++i){
		ss << "\t";
	}
	return ss.str();
}

}//~namespace


void SvgElement::toStream(std::ostream& s, unsigned indent) const{
	auto ind = indentStr(indent);
	
	s << ind << "<svg";
	this->Element::attribsToStream(s);
	this->Rectangle::attribsToStream(s);
	
	if(this->children.size() == 0){
		s << "/>";
	}else{
		s << ">" << std::endl;
		this->childrenToStream(s, indent + 1);
		s << ind << "</svg>";
	}
	s << std::endl;
}

void Container::childrenToStream(std::ostream& s, unsigned indent) const{
	for(auto& e : this->children){
		e->toStream(s, indent);
	}
}

std::string Element::toString() const{
	std::stringstream s;
	this->toStream(s, 0);
	return s.str();
}

void GElement::toStream(std::ostream& s, unsigned indent) const{
	auto ind = indentStr(indent);
	
	s << ind << "<g";
	this->Element::attribsToStream(s);
	this->Transformable::attribsToStream(s);
	this->Styleable::attribsToStream(s);
	
	if(this->children.size() == 0){
		s << "/>";
	}else{
		s << ">" << std::endl;
		this->childrenToStream(s, indent + 1);
		s << ind << "</g>";
	}
	s << std::endl;
}

void DefsElement::toStream(std::ostream& s, unsigned indent) const{
	auto ind = indentStr(indent);
	
	s << ind << "<defs";
	this->Element::attribsToStream(s);
	this->Transformable::attribsToStream(s);
	this->Styleable::attribsToStream(s);
	
	if(this->children.size() == 0){
		s << "/>";
	}else{
		s << ">" << std::endl;
		this->childrenToStream(s, indent + 1);
		s << ind << "</defs>";
	}
	s << std::endl;
}


void Styleable::attribsToStream(std::ostream& s) const{
	if(this->styles.size() == 0){
		return;
	}
	
	s << " style=\"";
	
	bool isFirst = true;
	
	for(auto& st : this->styles){
		if(isFirst){
			isFirst = false;
		}else{
			s << "; ";
		}
		
		ASSERT(st.first != EStyleProperty::UNKNOWN)
		
		s << propertyToString(st.first) << ":";
		
		switch(st.first){
			default:
				ASSERT(false)
				break;
			case EStyleProperty::STOP_OPACITY:
			case EStyleProperty::OPACITY:
			case EStyleProperty::STROKE_OPACITY:
			case EStyleProperty::FILL_OPACITY:
				s << st.second.opacity;
				break;
			case EStyleProperty::STOP_COLOR:
			case EStyleProperty::FILL:
			case EStyleProperty::STROKE:
				s << st.second.paintToString();
				break;
			case EStyleProperty::STROKE_WIDTH:
				s << st.second.length;
				break;
			case EStyleProperty::STROKE_LINECAP:
				switch(st.second.strokeLineCap){
					default:
						ASSERT(false)
						break;
					case EStrokeLineCap::BUTT:
						s << "butt";
						break;
					case EStrokeLineCap::ROUND:
						s << "round";
						break;
					case EStrokeLineCap::SQUARE:
						s << "square";
						break;
				}
				break;
		}
	}
	
	s << "\"";
}

void Transformable::attribsToStream(std::ostream& s) const{
	if(this->transformations.size() == 0){
		return;
	}
	
	s << " transform=\"";
	
	bool isFirst = true;
	
	for(auto& t : this->transformations){
		if(isFirst){
			isFirst = false;
		}else{
			s << " ";
		}
		
		switch(t.type){
			default:
				ASSERT(false)
				break;
			case Transformation::EType::MATRIX:
				s << "matrix(" << t.a << "," << t.b << "," << t.c << "," << t.d << "," << t.e << "," << t.f << ")";
				break;
			case Transformation::EType::TRANSLATE:
				s << "translate(" << t.x;
				if(t.y != 0){
					s << "," << t.y;
				}
				s << ")";
				break;
			case Transformation::EType::SCALE:
				s << "scale(" << t.x;
				if(t.x != t.y){
					s << "," << t.y;
				}
				s << ")";
				break;
			case Transformation::EType::ROTATE:
				s << "rotate(" << t.angle;
				if(t.x != 0 || t.y != 0){
					s << "," << t.x << "," << t.y;
				}
				s << ")";
				break;
			case Transformation::EType::SKEWX:
				s << "skewX(" << t.angle << ")";
				break;
			case Transformation::EType::SKEWY:
				s << "skewY(" << t.angle << ")";
				break;
		}
	}
	s << "\"";
}

decltype(Transformable::transformations) Transformable::parse(const std::string& str){
	std::istringstream s(str);
	
	s >> std::skipws;
	s >> std::setfill(' ');
	
	decltype(Transformable::transformations) ret;
	
	while(!s.eof()){
		std::string transform = readTillCharOrWhitespace(s, '(');
		
		Transformation t;
		
		if(transform == "matrix"){
			t.type = Transformation::EType::MATRIX;
		}else if(transform == "translate"){
			t.type = Transformation::EType::TRANSLATE;
		}else if(transform == "scale"){
			t.type = Transformation::EType::SCALE;
		}else if(transform == "rotate"){
			t.type = Transformation::EType::ROTATE;
		}else if(transform == "skewX"){
			t.type = Transformation::EType::SKEWX;
		}else if(transform == "skewY"){
			t.type = Transformation::EType::SKEWY;
		}else{
			return ret;//unknown transformation, stop parsing
		}
		
		{
			std::string str;
			s >> std::setw(1) >> str >> std::setw(0);
			if(str != "("){
				return ret;//expected (
			}
		}
		
		switch(t.type){
			default:
				ASSERT(false)
				break;
			case Transformation::EType::MATRIX:
				s >> t.a;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> t.b;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> t.c;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> t.d;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> t.e;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> t.f;
				if(s.fail()){
					return ret;
				}
				break;
			case Transformation::EType::TRANSLATE:
				s >> t.x;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> t.y;
				if(s.fail()){
					s.clear();
					t.y = 0;
				}
				break;
			case Transformation::EType::SCALE:
				s >> t.x;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> t.y;
				if(s.fail()){
					s.clear();
					t.y = t.x;
				}
				break;
			case Transformation::EType::ROTATE:
				s >> t.angle;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> t.x;
				if(s.fail()){
					s.clear();
					t.x = 0;
					t.y = 0;
				}else{
					skipWhitespacesAndOrComma(s);
					s >> t.y;
					if(s.fail()){
						return ret;//malformed rotate transformation
					}
				}
				break;
			case Transformation::EType::SKEWY:
			case Transformation::EType::SKEWX:
				s >> t.angle;
				if(s.fail()){
					return ret;
				}
				break;
		}
		
		{
			std::string str;
			s >> std::setw(1) >> str >> std::setw(0);
			if(str != ")"){
				return ret;//expected )
			}
		}
		
		ret.push_back(t);
		
		skipWhitespacesAndOrComma(s);
	}
	
	return ret;
}

EStyleProperty Styleable::stringToProperty(std::string str){
	if(str == "fill"){
		return EStyleProperty::FILL;
	}else if(str == "fill-opacity"){
		return EStyleProperty::FILL_OPACITY;
	}else if(str == "stroke"){
		return EStyleProperty::STROKE;
	}else if(str == "stroke-width"){
		return EStyleProperty::STROKE_WIDTH;
	}else if(str == "stroke-linecap"){
		return EStyleProperty::STROKE_LINECAP;
	}else if(str == "stroke-opacity"){
		return EStyleProperty::STROKE_OPACITY;
	}else if(str == "opacity"){
		return EStyleProperty::OPACITY;
	}else if(str == "stop-opacity"){
		return EStyleProperty::STOP_OPACITY;
	}else if(str == "stop-color"){
		return EStyleProperty::STOP_COLOR;
	}
	
	return EStyleProperty::UNKNOWN;
}

std::string Styleable::propertyToString(EStyleProperty p){
	switch(p){
		default:
			ASSERT(false)
			return "";
		case EStyleProperty::FILL:
			return "fill";
		case EStyleProperty::FILL_OPACITY:
			return "fill-opacity";
		case EStyleProperty::STROKE:
			return "stroke";
		case EStyleProperty::STROKE_WIDTH:
			return "stroke-width";
		case EStyleProperty::STROKE_LINECAP:
			return "stroke-linecap";
		case EStyleProperty::STROKE_OPACITY:
			return "stroke-opacity";
		case EStyleProperty::OPACITY:
			return "opacity";
		case EStyleProperty::STOP_OPACITY:
			return "stop-opacity";
		case EStyleProperty::STOP_COLOR:
			return "stop-color";
	}
}


decltype(Styleable::styles) Styleable::parse(const std::string& str){
	std::stringstream s(str);
	
	s >> std::skipws;
	s >> std::setfill(' ');
	
	decltype(Styleable::styles) ret;
	
	while(!s.eof()){
		skipWhitespaces(s);
		std::string property = readTillCharOrWhitespace(s, ':');
		
		EStyleProperty type = Styleable::stringToProperty(property);
		
		if(type == EStyleProperty::UNKNOWN){
			//unknown style property, skip it
			TRACE(<< "Unknown style property: " << property << std::endl)
			skipTillCharInclusive(s, ';');
			continue;
		}
		
		{
			std::string str;
			s >> std::setw(1) >> str >> std::setw(0);
			if(str != ":"){
				return ret;//expected colon
			}
		}
		
		skipWhitespaces(s);
		
		StylePropertyValue v;
		
		switch(type){
			default:
				ASSERT(false)
				break;
			case EStyleProperty::STOP_OPACITY:
			case EStyleProperty::OPACITY:
			case EStyleProperty::STROKE_OPACITY:
			case EStyleProperty::FILL_OPACITY:
				s >> v.opacity;
				if(s.fail()){
					s.clear();
				}else{
					utki::clampRange(v.opacity, real(0), real(1));
				}
				break;
			case EStyleProperty::STOP_COLOR:
			case EStyleProperty::FILL:
			case EStyleProperty::STROKE:
				v = StylePropertyValue::parsePaint(readTillChar(s, ';'));
//				TRACE(<< "paint read = " << std::hex << v.integer << std::endl)
				break;
			case EStyleProperty::STROKE_WIDTH:
				v.length = Length::parse(readTillChar(s, ';'));
//				TRACE(<< "stroke-width read = " << v.length << std::endl)
				break;
			case EStyleProperty::STROKE_LINECAP:
				{
					auto str = readTillCharOrWhitespace(s, ';');
					if(str == "butt"){
						v.strokeLineCap = EStrokeLineCap::BUTT;
					}else if(str == "round"){
						v.strokeLineCap = EStrokeLineCap::ROUND;
					}else if(str == "square"){
						v.strokeLineCap = EStrokeLineCap::SQUARE;
					}else{
						TRACE(<< "unknown strokeLineCap value:" << str << std::endl)
					}
				}
				break;
		}
		
		{
			std::string str;
			s >> std::setw(1) >> str >> std::setw(0);
			if(!s.eof() && str != ";"){
				return ret;//expected semicolon
			}
		}
		
		ret[type] = std::move(v);
	}
	
	return ret;
}



StylePropertyValue StylePropertyValue::parsePaint(const std::string& str){
	StylePropertyValue ret;
	
	if(str.size() == 0){
		ret.effective = false;
		return ret;
	}
	
	ASSERT(!std::isspace(str[0])) //leading spaces should be skept already

	//check if 'none'
	{
		std::string cmp = "none";
		if(cmp == str.substr(0, cmp.length())){
			ret.effective = false;
			return ret;
		}
	}
	
	//check if # notation
	if(str[0] == '#'){
		std::istringstream s(str.substr(1, 6));
		
		std::array<std::uint8_t, 6> d;
		unsigned numDigits = 0;
		for(auto i = d.begin(); i != d.end(); ++i, ++numDigits){
			char c = s.get();
			if('0' <= c && c <= '9'){
				(*i) = c - '0';
			}else if('a' <= c && c <= 'f'){
				(*i) = 10 + c - 'a';
			}else if('A' <= c && c <= 'F'){
				(*i) = 10 + c - 'A';
			}else{
				break;
			}
			
			ASSERT(((*i) & 0xf0) == 0) //only one hex digit
		}
		switch(numDigits){
			case 3:
				ret.color = (std::uint32_t(d[0]) << 4) | (std::uint32_t(d[0]))
						| (std::uint32_t(d[1]) << 12) | (std::uint32_t(d[1]) << 8)
						| (std::uint32_t(d[2]) << 20) | (std::uint32_t(d[2]) << 16);
				break;
			case 6:
				ret.color = (std::uint32_t(d[0]) << 4) | (std::uint32_t(d[1]))
						| (std::uint32_t(d[2]) << 12) | (std::uint32_t(d[3]) << 8)
						| (std::uint32_t(d[4]) << 20) | (std::uint32_t(d[5]) << 16);
				break;
			default:
				ret.effective = false;
				break;
		}
		
//		TRACE(<< "# color read = " << std::hex << ret.integer << std::endl)
		return ret;
	}
	
	//check if rgb() or RGB() notation
	{
		std::string rgb = "rgb(";
		if(rgb == str.substr(0, rgb.length())){
			std::istringstream s(str);
			
			s >> std::setw(rgb.length());
			
			s >> rgb;
			ASSERT(rgb == "rgb(")
			
			std::uint32_t r, g, b;
			
			skipWhitespaces(s);
			s >> r;
			skipWhitespacesAndOrComma(s);
			s >> g;
			skipWhitespacesAndOrComma(s);
			s >> b;
			skipWhitespaces(s);
			
			if(s.get() == ')'){
				ret.color = r | (g << 8) | (b << 16);
			}
		}
	}
	
	//TODO: check if color name
	
	return ret;
}

std::string StylePropertyValue::paintToString()const{
	if(!this->effective){
		return "none";
	}
	
	if(this->str.size() == 0){
		//it is a # notation
		
		std::stringstream s;
		s << std::hex;
		s << "#";
		s << ((this->color >> 4) & 0xf);
		s << ((this->color) & 0xf);
		s << ((this->color >> 12) & 0xf);
		s << ((this->color >> 8) & 0xf);
		s << ((this->color >> 20) & 0xf);
		s << ((this->color >> 16) & 0xf);
		return s.str();
	}
	
	//TODO: other notations
	
	return "";
}

void PathElement::toStream(std::ostream& s, unsigned indent) const{
	auto ind = indentStr(indent);
	
	s << ind << "<path";
	this->Element::attribsToStream(s);
	this->Transformable::attribsToStream(s);
	this->Styleable::attribsToStream(s);
	
	this->attribsToStream(s);
	
	s << "/>";
	s << std::endl;
}

void PathElement::attribsToStream(std::ostream& s) const{
	if(this->path.size() == 0){
		return;
	}
	
	s << " d=\"";
	
	Step::EType curType = Step::EType::UNKNOWN;
	
	for(auto& step : this->path){
		if(curType == step.type){
			s << " ";
		}else{
			s << Step::typeToChar(step.type);
			curType = step.type;
		}
		
		switch(step.type){
			case Step::EType::MOVE_ABS:
			case Step::EType::MOVE_REL:
			case Step::EType::LINE_ABS:
			case Step::EType::LINE_REL:
				s << step.x;
				s << ",";
				s << step.y;
				break;
			case Step::EType::CLOSE:
				break;
			case Step::EType::HORIZONTAL_LINE_ABS:
			case Step::EType::HORIZONTAL_LINE_REL:
				s << step.x;
				break;
			case Step::EType::VERTICAL_LINE_ABS:
			case Step::EType::VERTICAL_LINE_REL:
				s << step.y;
				break;
			case Step::EType::CUBIC_ABS:
			case Step::EType::CUBIC_REL:
				s << step.x1;
				s << ",";
				s << step.y1;
				s << " ";
				s << step.x2;
				s << ",";
				s << step.y2;
				s << " ";
				s << step.x;
				s << ",";
				s << step.y;
				break;
			case Step::EType::CUBIC_SMOOTH_ABS:
			case Step::EType::CUBIC_SMOOTH_REL:
				s << step.x2;
				s << ",";
				s << step.y2;
				s << " ";
				s << step.x;
				s << ",";
				s << step.y;
				break;
			case Step::EType::QUADRATIC_ABS:
			case Step::EType::QUADRATIC_REL:
				s << step.x1;
				s << ",";
				s << step.y1;
				s << " ";
				s << step.x;
				s << ",";
				s << step.y;
				break;
			case Step::EType::QUADRATIC_SMOOTH_ABS:
			case Step::EType::QUADRATIC_SMOOTH_REL:
				s << step.x;
				s << ",";
				s << step.y;
				break;
			case Step::EType::ARC_ABS:
			case Step::EType::ARC_REL:
				s << step.rx;
				s << ",";
				s << step.ry;
				s << " ";
				s << step.xAxisRotation;
				s << " ";
				s << (step.flags.largeArc ? "1" : "0");
				s << ",";
				s << (step.flags.sweep ? "1" : "0");
				s << " ";
				s << step.x;
				s << ",";
				s << step.y;
				break;
			default:
				ASSERT(false)
				break;
		}
	}
	
	s << "\"";
}

char PathElement::Step::typeToChar(EType t){
	switch(t){
		case Step::EType::MOVE_ABS:
			return 'M';
		case Step::EType::MOVE_REL:
			return 'm';
		case Step::EType::LINE_ABS:
			return 'L';
		case Step::EType::LINE_REL:
			return 'l';
		case Step::EType::CLOSE:
			return 'z';
		case Step::EType::HORIZONTAL_LINE_ABS:
			return 'H';
		case Step::EType::HORIZONTAL_LINE_REL:
			return 'h';
		case Step::EType::VERTICAL_LINE_ABS:
			return 'V';
		case Step::EType::VERTICAL_LINE_REL:
			return 'v';
		case Step::EType::CUBIC_ABS:
			return 'C';
		case Step::EType::CUBIC_REL:
			return 'c';
		case Step::EType::CUBIC_SMOOTH_ABS:
			return 'S';
		case Step::EType::CUBIC_SMOOTH_REL:
			return 's';
		case Step::EType::QUADRATIC_ABS:
			return 'Q';
		case Step::EType::QUADRATIC_REL:
			return 'q';
		case Step::EType::QUADRATIC_SMOOTH_ABS:
			return 'T';
		case Step::EType::QUADRATIC_SMOOTH_REL:
			return 't';
		case Step::EType::ARC_ABS:
			return 'A';
		case Step::EType::ARC_REL:
			return 'a';
		default:
			ASSERT(false)
			return ' ';
	}
}


PathElement::Step::EType PathElement::Step::charToType(char c){
	switch(c){
		case 'M':
			return Step::EType::MOVE_ABS;
		case 'm':
			return Step::EType::MOVE_REL;
		case 'z':
		case 'Z':
			return Step::EType::CLOSE;
		case 'L':
			return Step::EType::LINE_ABS;
		case 'l':
			return Step::EType::LINE_REL;
		case 'H':
			return Step::EType::HORIZONTAL_LINE_ABS;
		case 'h':
			return Step::EType::HORIZONTAL_LINE_REL;
		case 'V':
			return Step::EType::VERTICAL_LINE_ABS;
		case 'v':
			return Step::EType::VERTICAL_LINE_REL;
		case 'C':
			return Step::EType::CUBIC_ABS;
		case 'c':
			return Step::EType::CUBIC_REL;
		case 'S':
			return Step::EType::CUBIC_SMOOTH_ABS;
		case 's':
			return Step::EType::CUBIC_SMOOTH_REL;
		case 'Q':
			return Step::EType::QUADRATIC_ABS;
		case 'q':
			return Step::EType::QUADRATIC_REL;
		case 'T':
			return Step::EType::QUADRATIC_SMOOTH_ABS;
		case 't':
			return Step::EType::QUADRATIC_SMOOTH_REL;
		case 'A':
			return Step::EType::ARC_ABS;
		case 'a':
			return Step::EType::ARC_REL;
		default:
			return Step::EType::UNKNOWN;
	}
}


decltype(PathElement::path) PathElement::parse(const std::string& str){
	decltype(PathElement::path) ret;
	
	std::istringstream s(str);
	s >> std::skipws;
	
	skipWhitespaces(s);
	
	Step::EType curType = Step::EType::MOVE_ABS;
	
	while(!s.eof()){
		ASSERT(!std::isspace(s.peek()))//spaces should be skept
		
		{
			auto t = Step::charToType(s.peek());
			if(t != Step::EType::UNKNOWN){
				curType = t;
				s.get();
			}
		}
		
		skipWhitespaces(s);
		
		Step step;
		step.type = curType;
		
		switch(step.type){
			case Step::EType::MOVE_ABS:
			case Step::EType::MOVE_REL:
			case Step::EType::LINE_ABS:
			case Step::EType::LINE_REL:
				s >> step.x;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.y;
				if(s.fail()){
					return ret;
				}
				break;
			case Step::EType::CLOSE:
				break;
			case Step::EType::HORIZONTAL_LINE_ABS:
			case Step::EType::HORIZONTAL_LINE_REL:
				s >> step.x;
				if(s.fail()){
					return ret;
				}
				break;
			case Step::EType::VERTICAL_LINE_ABS:
			case Step::EType::VERTICAL_LINE_REL:
				s >> step.y;
				if(s.fail()){
					return ret;
				}
				break;
			case Step::EType::CUBIC_ABS:
			case Step::EType::CUBIC_REL:
				s >> step.x1;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.y1;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.x2;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.y2;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.x;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.y;
				if(s.fail()){
					return ret;
				}
				break;
			case Step::EType::CUBIC_SMOOTH_ABS:
			case Step::EType::CUBIC_SMOOTH_REL:
				s >> step.x2;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.y2;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.x;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.y;
				if(s.fail()){
					return ret;
				}
				break;
			case Step::EType::QUADRATIC_ABS:
			case Step::EType::QUADRATIC_REL:
				s >> step.x1;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.y1;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.x;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.y;
				if(s.fail()){
					return ret;
				}
				break;
			case Step::EType::QUADRATIC_SMOOTH_ABS:
			case Step::EType::QUADRATIC_SMOOTH_REL:
				s >> step.x;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.y;
				if(s.fail()){
					return ret;
				}
				break;
			case Step::EType::ARC_ABS:
			case Step::EType::ARC_REL:
				s >> step.rx;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.ry;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.xAxisRotation;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				{
					unsigned f;
					s >> f;
					if(s.fail()){
						return ret;
					}
					step.flags.largeArc = (f != 0);
				}
				skipWhitespacesAndOrComma(s);
				{
					unsigned f;
					s >> f;
					if(s.fail()){
						return ret;
					}
					step.flags.sweep = (f != 0);
				}
				skipWhitespacesAndOrComma(s);
				s >> step.x;
				if(s.fail()){
					return ret;
				}
				skipWhitespacesAndOrComma(s);
				s >> step.y;
				if(s.fail()){
					return ret;
				}
				break;
			default:
				ASSERT(false)
				break;
		}
		
		ret.push_back(step);
		
		skipWhitespacesAndOrComma(s);
	}
	
	return ret;
}

void Container::render(Renderer& renderer) const{
	for(auto& e : this->children){
		e->render(renderer);
	}
}

void PathElement::render(Renderer& renderer) const{
	renderer.render(*this);
}

void GElement::render(Renderer& renderer) const{
	renderer.render(*this);
}


void SvgElement::render(Renderer& renderer) const{
	renderer.render(*this);
}

std::uint32_t StylePropertyValue::getColor() const{
	//TODO: check other notations
	return this->color;
}

Rgb StylePropertyValue::getRgb() const{
	auto c = this->getColor();
	
	Rgb ret;
	
	ret.r = real(c & 0xff) / real(0xff);
	ret.g = real((c >> 8) & 0xff) / real(0xff);
	ret.b = real((c >> 16) & 0xff) / real(0xff);
	
	return ret;
}


const StylePropertyValue* Element::getStyleProperty(EStyleProperty property) const{
	if(auto styleable = dynamic_cast<const Styleable*>(this)){
		auto i = styleable->styles.find(property);
		if(i != styleable->styles.end()){
			return &i->second;
		}
	}
	
	if(!this->parent){
		return nullptr;
	}
	
	return this->parent->getStyleProperty(property);
}

Gradient::ESpreadMethod Gradient::stringToSpreadMethod(const std::string& str) {
	if(str == "pad"){
		return ESpreadMethod::PAD;
	}else if(str == "reflect"){
		return ESpreadMethod::REFLECT;
	}else if(str == "repeat"){
		return ESpreadMethod::REPEAT;
	}
	return ESpreadMethod::PAD;
}

std::string Gradient::spreadMethodToString(ESpreadMethod sm) {
	switch(sm){
		default:
			ASSERT(false)
			return "";
		case ESpreadMethod::PAD:
			return "pad";
		case ESpreadMethod::REFLECT:
			return "reflect";
		case ESpreadMethod::REPEAT:
			return "repeat";
	}
}

void LinearGradientElement::toStream(std::ostream& s, unsigned indent) const {
	auto ind = indentStr(indent);
	
	s << ind << "<linearGradient";
	this->Element::attribsToStream(s);
	this->Gradient::attribsToStream(s);
	this->Referencing::attribsToStream(s);
	
	if(this->children.size() == 0){
		s << "/>";
	}else{
		s << ">" << std::endl;
		this->childrenToStream(s, indent + 1);
		s << ind << "</linearGradient>";
	}
	s << std::endl;
}

void Gradient::attribsToStream(std::ostream& s)const{
	if(this->spreadMethod != ESpreadMethod::PAD){
		s << " spreadMethod=\"" << Gradient::spreadMethodToString(this->spreadMethod) << "\"";
	}
	
	//TODO: gradientTransform
}


void Gradient::StopElement::toStream(std::ostream& s, unsigned indent) const {
	auto ind = indentStr(indent);
	s << ind << "<stop";
	s << " offset=\"" << this->offset << "\"";
	this->Styleable::attribsToStream(s);
	s << "/>" << std::endl;
}

void Referencing::attribsToStream(std::ostream& s) const {
	if(this->iri.length() == 0){
		return;
	}
	s << " xlink:href=\"" << this->iri << "\"";
}
