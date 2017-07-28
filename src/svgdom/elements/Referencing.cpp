#include "Referencing.hpp"

#include "../util.hxx"

using namespace svgdom;


void Referencing::attribsToStream(std::ostream& s) const {
	if(this->iri.length() == 0){
		return;
	}
	s << " xlink:href=\"" << this->iri << "\"";
}

std::string Referencing::getLocalIdFromIri() const {
	return iriToLocalId(this->iri);
}
