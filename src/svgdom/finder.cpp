#include "finder.hpp"

#include <utki/debug.hpp>

#include "visitor.hpp"

using namespace svgdom;

namespace{
class CacheCreator : virtual public svgdom::ConstVisitor{
public:
	std::map<std::string, finder::ElementInfo> cache;
	
	StyleStack styleStack;
	
	void addToCache(const svgdom::Element& e){
		if(e.id.length() != 0){
			this->cache.insert(std::make_pair(e.id, finder::ElementInfo(e, this->styleStack)));
		}
	}
	
	void visitContainer(const svgdom::Element& e, const svgdom::Container& c, const svgdom::Styleable& s){
		StyleStack::Push push(this->styleStack, s);
		this->addToCache(e);
		this->relayAccept(c);
	}
	void visitElement(const svgdom::Element& e, const svgdom::Styleable& s){
		StyleStack::Push push(this->styleStack, s);
		this->addToCache(e);
	}
	
	void defaultVisit(const svgdom::Element& e) override{
		this->addToCache(e);
	}
	
	void visit(const svgdom::GElement& e) override{
		this->visitContainer(e, e, e);
	}
	void visit(const svgdom::SymbolElement& e) override{
		this->visitContainer(e, e, e);
	}
	void visit(const svgdom::SvgElement& e) override{
		this->visitContainer(e, e, e);
	}
	void visit(const svgdom::RadialGradientElement& e) override{
		this->visitContainer(e, e, e);
	}
	void visit(const svgdom::LinearGradientElement& e) override{
		this->visitContainer(e, e, e);
	}
	void visit(const svgdom::DefsElement& e) override{
		this->visitContainer(e, e, e);
	}
	void visit(const svgdom::FilterElement& e) override{
		this->visitContainer(e, e, e);
	}
	
	void visit(const svgdom::PolylineElement& e) override{
		this->visitElement(e, e);
	}
	void visit(const svgdom::CircleElement& e) override{
		this->visitElement(e, e);
	}
	void visit(const svgdom::UseElement& e) override{
		this->visitElement(e, e);
	}
	void visit(const svgdom::Gradient::StopElement& e) override{
		this->visitElement(e, e);
	}
	void visit(const svgdom::PathElement& e) override{
		this->visitElement(e, e);
	}
	void visit(const svgdom::RectElement& e) override{
		this->visitElement(e, e);
	}
	void visit(const svgdom::LineElement& e) override{
		this->visitElement(e, e);
	}
	void visit(const svgdom::EllipseElement& e) override{
		this->visitElement(e, e);
	}
	void visit(const svgdom::PolygonElement& e) override{
		this->visitElement(e, e);
	}	
	void visit(const svgdom::FeGaussianBlurElement& e) override{
		this->visitElement(e, e);
	}
	void visit(const ImageElement& e) override{
		this->visitElement(e, e);
	}
};
}

finder::finder(const svgdom::Element& root) :
		cache([&root](){
			CacheCreator visitor;
	
			root.accept(visitor);
			
			return std::move(visitor.cache);
		}())
{}

const finder::ElementInfo* finder::find_by_id(const std::string& id)const{
	if(id.length() == 0){
		return nullptr;
	}
	
	auto i = this->cache.find(id);
	if(i == this->cache.end()){
		return nullptr;
	}
	
	return &i->second;
}
