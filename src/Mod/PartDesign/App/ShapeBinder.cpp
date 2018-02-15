/***************************************************************************
 *   Copyright (c) 2015 Stefan Tröger <stefantroeger@gmx.net>              *
 *                                                                         *
 *   This file is part of the FreeCAD CAx development system.              *
 *                                                                         *
 *   This library is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU Library General Public           *
 *   License as published by the Free Software Foundation; either          *
 *   version 2 of the License, or (at your option) any later version.      *
 *                                                                         *
 *   This library  is distributed in the hope that it will be useful,      *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU Library General Public License for more details.                  *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this library; see the file COPYING.LIB. If not,    *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,         *
 *   Suite 330, Boston, MA  02111-1307, USA                                *
 *                                                                         *
 ***************************************************************************/


#include "PreCompiled.h"
#ifndef _PreComp_
# include <cfloat>
# include <BRepBuilderAPI_MakeFace.hxx>
# include <BRepBuilderAPI_MakeWire.hxx>
# include <BRep_Builder.hxx>
# include <BRep_Tool.hxx>
# include <TopExp_Explorer.hxx>
# include <TopoDS.hxx>
# include <Precision.hxx>
#endif

#include <App/Document.h>
#include "ShapeBinder.h"
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/FaceMakerBullseye.h>

#ifndef M_PI
#define M_PI       3.14159265358979323846
#endif

using namespace PartDesign;

// ============================================================================

PROPERTY_SOURCE(PartDesign::ShapeBinder, Part::Feature)

ShapeBinder::ShapeBinder()
{
    ADD_PROPERTY_TYPE(Support, (0,0), "",(App::PropertyType)(App::Prop_None),"Support of the geometry");
    Placement.setStatus(App::Property::Hidden, true);
}

ShapeBinder::~ShapeBinder()
{
}

short int ShapeBinder::mustExecute(void) const {

    if(Support.isTouched())
        return 1;

    return Part::Feature::mustExecute();
}

App::DocumentObjectExecReturn* ShapeBinder::execute(void) {

    if(! this->isRestoring()){
        Part::Feature* obj = nullptr;
        std::vector<std::string> subs;

        ShapeBinder::getFilteredReferences(&Support, obj, subs);
        //if we have a link we rebuild the shape, but we change nothing if we are a simple copy
        if(obj) {
            Part::TopoShape shape = ShapeBinder::buildShapeFromReferences(obj, subs);
            Base::Placement placement(shape.getTransform());
            Shape.setValue(shape);
            Placement.setValue(placement);
        }
    }

    return Part::Feature::execute();
}

void ShapeBinder::getFilteredReferences(App::PropertyLinkSubList* prop, Part::Feature*& obj, std::vector< std::string >& subobjects) {

    obj = nullptr;
    subobjects.clear();

    auto objs = prop->getValues();
    auto subs = prop->getSubValues();

    if(objs.empty()) {
        return;
    }

    //we only allow one part feature, so get the first one we find
    size_t index = 0;
    while(index < objs.size() && !objs[index]->isDerivedFrom(Part::Feature::getClassTypeId()))
        index++;

    //do we have any part feature?
    if(index >= objs.size())
        return;

    obj = static_cast<Part::Feature*>(objs[index]);

    //if we have no subshpape we use the whole shape
    if(subs[index].empty()) {
            return;
    }

    //collect all subshapes for the object
    index = 0;
    for(std::string sub : subs) {

        //we only allow subshapes from a single Part::Feature
        if(objs[index] != obj)
            continue;

        //in this mode the full shape is not allowed, as we already started the subshape
        //processing
        if(sub.empty())
            continue;

        subobjects.push_back(sub);
    }
}


Part::TopoShape ShapeBinder::buildShapeFromReferences( Part::Feature* obj, std::vector< std::string > subs) {

    if(!obj)
        return TopoDS_Shape();

    if(subs.empty())
        return obj->Shape.getShape();

    //if we use multiple subshapes we build a shape from them by fusing them together
    Part::TopoShape base;
    std::vector<TopoDS_Shape> operators;
    for(std::string sub : subs) {

        if(base.isNull())
            base = obj->Shape.getShape().getSubShape(sub.c_str());
        else
            operators.push_back(obj->Shape.getShape().getSubShape(sub.c_str()));
    }

    try {
        if(!operators.empty() && !base.isNull())
            return base.fuse(operators);
    }
    catch(...) {
        return base;
    }
    return base;
}

void ShapeBinder::handleChangedPropertyType(Base::XMLReader &reader, const char *TypeName, App::Property *prop)
{
    // The type of Support was App::PropertyLinkSubList in the past
    if (prop == &Support && strcmp(TypeName, "App::PropertyLinkSubList") == 0) {
        Support.Restore(reader);
    }
}

// ============================================================================

namespace Part {
PartExport std::list<TopoDS_Edge> sort_Edges(double tol3d, std::list<TopoDS_Edge>& edges);
}

// ============================================================================

PROPERTY_SOURCE(PartDesign::SubShapeBinder, PartDesign::ShapeBinder)

SubShapeBinder::SubShapeBinder()
{
    ADD_PROPERTY_TYPE(Fuse, (true), "Base",App::Prop_None,"Fused linked solid shapes");
    ADD_PROPERTY_TYPE(MakeFace, (true), "Base",App::Prop_None,"Create face for linked wires");
    ADD_PROPERTY_TYPE(ClaimChildren, (false), "Base",App::Prop_Output,"Claim linked object as children");
    ADD_PROPERTY_TYPE(Relative, (false), "Base",App::Prop_None,"Enable relative sub-object linking");
    Placement.setStatus(App::Property::Hidden, true);
}

App::DocumentObjectExecReturn* SubShapeBinder::execute(void) {
    if(this->isRestoring())
        return Part::Feature::execute();

    BRep_Builder builder;
    TopoDS_Compound comp;
    builder.MakeCompound(comp);

    int count = 0;
    auto subset = Support.getSubListValues();
    for(auto &info : subset) {
        auto obj = info.first;
        if(!obj || !obj->getNameInDocument())
            continue;
        static std::string none("");
        std::set<std::string> subs(info.second.begin(),info.second.end());
        if(subs.empty())
            subs.insert(none);
        else if(subs.size()>1)
            subs.erase(none);
        for(const auto &sub : subs) {
            auto shape = Part::Feature::getShape(obj,sub.c_str(),true);
            if(!shape.IsNull()){
                builder.Add(comp,shape);
                ++count;
            }
        }
    }
    if(!count)
        return new App::DocumentObjectExecReturn("No shapes");

    if(Fuse.getValue()) {
        // If the compound has solid, fuse them together, and ignore other type of
        // shapes
        count = 0;
        Part::TopoShape base;
        std::vector<TopoDS_Shape> operators;
        for(TopExp_Explorer it(comp, TopAbs_SOLID); it.More(); it.Next(),++count) {
            if(!count) 
                base = it.Current();
            else
                operators.push_back(it.Current());
        }
        if(count) {
            if(operators.empty())
                Shape.setValue(base.getShape());
            else
                Shape.setValue(base.fuse(operators));
            return Part::Feature::execute();
        }
    }

    if(MakeFace.getValue() && !TopExp_Explorer(comp, TopAbs_FACE).More()) {
        if(!TopExp_Explorer(comp, TopAbs_SHELL).More()) {
            std::list<TopoDS_Edge> edges;
            for(TopExp_Explorer it(comp,TopAbs_EDGE);it.More();it.Next())
                edges.push_back(TopoDS::Edge(it.Current()));
            if(edges.size()) {
                Part::FaceMakerBullseye mkFace;
                do {
                    BRepBuilderAPI_MakeWire mkWire;
                    for(auto &edge : Part::sort_Edges(Precision::Confusion(),edges))
                        mkWire.Add(edge);
                    mkFace.addWire(mkWire.Wire());
                }while(edges.size());
                try {
                    mkFace.Build();
                    const TopoDS_Shape &shape = mkFace.Shape();
                    if(!shape.IsNull()) {
                        Shape.setValue(shape);
                        return Part::Feature::execute();
                    }
                }catch (Base::Exception &){}
            }
        }
    }
    Shape.setValue(comp);
    return Part::Feature::execute();
}

void SubShapeBinder::setLinks(
        const std::vector<std::pair<App::DocumentObject*,std::string> > &subs,
        bool reset)
{
    std::map<App::DocumentObject*, std::set<std::string> > values;
    if(!reset) {
        const auto &objs = Support.getValues();
        const auto &subs = Support.getSubValues();
        for(size_t i=0;i<objs.size();++i) {
            auto obj = objs[i];
            if(!obj || !obj->getNameInDocument())
                continue;
            values[obj].insert(subs[i]);
        }
    }
    for(auto &info : subs) {
        auto obj = info.first;
        if(!obj || !obj->getNameInDocument())
            continue;
        if(obj->getDocument()!=getDocument()) 
            throw Base::RuntimeError("Direct external linking is not allowed");
        if(Relative.getValue()) 
            values[obj].insert(info.second);
        else {
            auto &sub = info.second;
            auto idx = sub.rfind('.');
            if(idx == std::string::npos) {
                values[obj].insert(sub);
                continue;
            }
            auto sobj = obj->getSubObject(sub.c_str());
            if(!sobj) 
                throw Base::RuntimeError("Cannot find sub object");
            if(sobj->getDocument()!=getDocument())
                values[obj].insert(info.second);
            else
                values[sobj].insert(sub.c_str()+idx+1);
        }
    }
    auto inSet = getInListEx(true);
    inSet.insert(this);
    std::vector<App::PropertyLinkSubList::SubSet> newValues;
    for(auto &value : values) {
        newValues.emplace_back();
        auto &newSub = newValues.back();
        newSub.first = value.first;
        if(inSet.find(value.first)!=inSet.end())
            throw Base::RuntimeError("Cyclic dependency");
        newSub.second.insert(newSub.second.end(),value.second.begin(),value.second.end());
    }
    Support.setSubListValues(newValues);
}
