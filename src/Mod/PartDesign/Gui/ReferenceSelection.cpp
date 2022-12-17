/******************************************************************************
 *   Copyright (c) 2012 Konstantinos Poulios <logari81@gmail.com>             *
 *                                                                            *
 *   This file is part of the FreeCAD CAx development system.                 *
 *                                                                            *
 *   This library is free software; you can redistribute it and/or            *
 *   modify it under the terms of the GNU Library General Public              *
 *   License as published by the Free Software Foundation; either             *
 *   version 2 of the License, or (at your option) any later version.         *
 *                                                                            *
 *   This library  is distributed in the hope that it will be useful,         *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *   GNU Library General Public License for more details.                     *
 *                                                                            *
 *   You should have received a copy of the GNU Library General Public        *
 *   License along with this library; see the file COPYING.LIB. If not,       *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,            *
 *   Suite 330, Boston, MA  02111-1307, USA                                   *
 *                                                                            *
 ******************************************************************************/

#include "PreCompiled.h"

#ifndef _PreComp_
# include <TopoDS.hxx>
# include <TopoDS_Edge.hxx>
# include <TopoDS_Face.hxx>
# include <BRepAdaptor_Curve.hxx>
# include <BRepAdaptor_Surface.hxx>
# include <QDialog>
# include <QLabel>
# include <gp_Pln.hxx>
#endif

#include <boost/algorithm/string/find.hpp>

#include <App/OriginFeature.h>
#include <App/GeoFeatureGroupExtension.h>
#include <App/Origin.h>
#include <App/Part.h>
#include <App/MappedElement.h>
#include <Gui/Application.h>
#include <Gui/Document.h>
#include <Gui/Command.h>
#include <Gui/MainWindow.h>
#include <Mod/Part/App/TopoShape.h>
#include <Mod/Part/App/PartFeature.h>
#include <Mod/PartDesign/App/Feature.h>
#include <Mod/PartDesign/App/Body.h>
#include <Mod/PartDesign/App/DatumPoint.h>
#include <Mod/PartDesign/App/DatumLine.h>
#include <Mod/PartDesign/App/DatumPlane.h>

#include "Utils.h"

#include "ReferenceSelection.h"
#include "TaskFeaturePick.h"
#include <ui_DlgReference.h>

FC_LOG_LEVEL_INIT("PartDesign",true,true)

using namespace PartDesignGui;
using namespace Gui;

/* TRANSLATOR PartDesignGui::ReferenceSelection.cpp */

bool ReferenceSelection::allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName)
{
    // TODO review this function (2015-09-04, Fat-Zer)
    PartDesign::Body *body;
    App::DocumentObject* originGroupObject = nullptr;

    if ( support ) {
        body = PartDesign::Body::findBodyOf (support);
    } else {
        body = PartDesignGui::getBody (false);
    }

    if ( body ) { // Search for Part of the body
        originGroupObject = App::OriginGroupExtension::getGroupOfObject ( body ) ;
    } else if ( support ) { // if no body search part for support
        originGroupObject = App::OriginGroupExtension::getGroupOfObject ( support ) ;
    } else { // fallback to active part
        originGroupObject = PartDesignGui::getActivePart ( );
    }

    App::OriginGroupExtension* originGroup = nullptr;
    if(originGroupObject)
        originGroup = originGroupObject->getExtensionByType<App::OriginGroupExtension>();

    // Don't allow selection in other document
    if ( support && pDoc != support->getDocument() ) {
        return false;
    }

    // Enable selection from origin of current part/
    if ( pObj->getTypeId().isDerivedFrom(App::OriginFeature::getClassTypeId()) ) {
        bool fits = false;
        if ( _conf.plane && pObj->getTypeId().isDerivedFrom(App::Plane::getClassTypeId()) ) {
            fits = true;
        } else if ( _conf.edge && pObj->getTypeId().isDerivedFrom(App::Line::getClassTypeId()) ) {
            fits = true;
        }

        if (fits) { // check that it is actually belongs to the chosen body or part
            try { // here are some throwers
                if (body) {
                    if (body->getOrigin ()->hasObject (pObj) ) {
                        return true;
                    }
                } else if (originGroup ) {
                    if ( originGroup->getOrigin ()->hasObject (pObj) ) {
                        return true;
                    }
                }
            } catch (const Base::Exception&)
            { }
        }
        return false; // The Plane/Axis doesn't fits our needs
    }

    if (pObj->getTypeId().isDerivedFrom(Part::Datum::getClassTypeId())) {

        if (!body) { // Allow selecting Part::Datum features from the active Body
            return false;
        } else if (!_conf.allowOtherBody && !body->hasObject(pObj)) {
            return false;
        }

        if (_conf.plane && (pObj->getTypeId().isDerivedFrom(PartDesign::Plane::getClassTypeId())))
            return true;
        if (_conf.edge && (pObj->getTypeId().isDerivedFrom(PartDesign::Line::getClassTypeId())))
            return true;
        if (_conf.point && (pObj->getTypeId().isDerivedFrom(PartDesign::Point::getClassTypeId())))
            return true;

        return false;
    }

    if (!_conf.allowOtherBody) {
        if (support == NULL)
            return false;
        if (pObj != support)
            return false;
    }
    // Handle selection of geometry elements
    if (!sSubName || sSubName[0] == '\0')
        return _conf.whole;
    if (_conf.edge && boost::ifind_first(sSubName, "edge")) {
        auto shape = Part::Feature::getShape(pObj, sSubName, /*needSubElement*/true);
        if (!shape.IsNull() && shape.ShapeType() == TopAbs_EDGE) {
            if (_conf.planar)
                return Part::TopoShape(shape).isLinearEdge();
            else
                return true;
        }
    }
    if (_conf.plane && boost::ifind_first(sSubName, "face")) {
        auto shape = Part::Feature::getShape(pObj, sSubName, /*needSubElement*/true);
        if (!shape.IsNull() && shape.ShapeType() == TopAbs_FACE) {
            if (_conf.planar)
                return Part::TopoShape(shape).isPlanarFace();
            else
                return true;
        }
    }
    if (_conf.point && boost::ifind_first(sSubName, "vertex")) {
        return true;
    }
    if (!_conf.edge && _conf.circle && boost::ifind_first(sSubName, "edge")) {
        auto shape = Part::Feature::getShape(pObj, sSubName, /*needSubElement*/true);
        if (!shape.IsNull() && shape.ShapeType() == TopAbs_EDGE) {
            BRepAdaptor_Curve adapt(TopoDS::Edge(shape));
            return adapt.GetType() == GeomAbs_Circle;
        }
    }
    if (_conf.wire && boost::ifind_first(sSubName, "wire")) {
        auto shape = Part::Feature::getShape(pObj, sSubName, /*needSubElement*/true);
        if (!shape.IsNull() && shape.ShapeType() == TopAbs_WIRE) {
            if (_conf.planar) {
                gp_Pln pln;
                return Part::TopoShape(shape).findPlane(pln);
            } else
                return true;
        }
    }

    return false;
}

bool NoDependentsSelection::allow(App::Document* /*pDoc*/, App::DocumentObject* pObj, const char* /*sSubName*/)
{
    if(!inList.count(pObj)) {
        return true;
    }
    else {
        this->notAllowedReason = QT_TR_NOOP("Selecting this will cause circular dependency.");
        return false;
    }
}

bool CombineSelectionFilterGates::allow(App::Document* pDoc, App::DocumentObject* pObj, const char* sSubName)
{
    return filter1->allow(pDoc, pObj, sSubName) && filter2->allow(pDoc, pObj, sSubName);
}


namespace PartDesignGui
{

bool getReferencedSelection(const App::DocumentObject* thisObj, const Gui::SelectionChanges& msg,
                            App::DocumentObject*& selObj, std::vector<std::string>& selSub)
{
    selObj = nullptr;
    if (!thisObj)
        return false;

    App::SubObjectT sel = msg.pOriginalMsg ? msg.pOriginalMsg->Object : msg.Object;
    if (sel.getDocument() != thisObj->getDocument())
        return false;

    auto sobj = sel.getSubObject();
    if (!sobj || sobj == thisObj)
        return false;

    if (PartDesign::Feature::isDatum(sobj->getLinkedObject()))
        sel.setSubName(sel.getSubNameNoElement());

    auto objT = PartDesignGui::importExternalObject(sel, false, false);
    selObj = objT.getObject();
    selSub.push_back(objT.getSubName());
    return true;
}

QString getRefStr(const App::DocumentObject* obj, const std::vector<std::string>& sub)
{
    if (obj == NULL)
        return QStringLiteral("");

    if (PartDesign::Feature::isDatum(obj))
        return QString::fromUtf8(obj->getNameInDocument());
    else if (sub.size()>0)
        return QString::fromUtf8(obj->getNameInDocument()) + QStringLiteral(":") +
               QString::fromUtf8(sub.front().c_str());
    else
        return QString();
}

bool populateRefElement(App::PropertyLinkSub *prop, QLabel *label, bool canTouch) {
    if(!prop || !label)
        return false;

    App::DocumentObject* obj = prop->getValue();
    if(!obj || !obj->getNameInDocument()) {
        label->setText(QObject::tr("None"));
        return false;
    }
    const auto &subs = prop->getShadowSubs();
    if(subs.empty()) {
        label->setText(QString::fromUtf8(obj->getNameInDocument()));
        return false;
    }

    // first=new style topo name, second=old index based name
    std::pair<std::string, std::string> sub = subs.front();

    QString fmt = QStringLiteral("%1:%2");

    // Check if the element is missing
    bool touched = false;
    if(Data::ComplexGeoData::hasMissingElement(sub.second.c_str())) {
        if(canTouch) {
            for(auto &mapped : Part::Feature::getRelatedElements(obj,sub.first.c_str())) {
                FC_WARN("guess element reference: " << sub.first << " -> " << mapped.name);
                touched = true;
                sub.second.clear();
                mapped.index.toString(sub.second);
                prop->setValue(obj, {sub.second});
                break;
            }
        }
        if(!touched)
            fmt = QStringLiteral("<font color='red'>%1:%2</font>");
    }

    // Display the shorter index based name
    label->setText(fmt.arg(QString::fromUtf8(obj->getNameInDocument()),
                           QString::fromUtf8(sub.second.c_str())));
    return touched;
}


std::string buildLinkSubPythonStr(const App::DocumentObject* obj, const std::vector<std::string>& subs)
{
    if ( obj == NULL)
        return "None";

    std::string result("[");

    for (const auto & sub : subs)
        result += "\"" + sub + "\",";
    result += "]";

    return result;
}

std::string buildLinkSingleSubPythonStr(const App::DocumentObject* obj,
        const std::vector<std::string>& subs)
{
    if (obj == NULL)
        return "None";

    if (PartDesign::Feature::isDatum(obj))
        return Gui::Command::getObjectCmd(obj,"(",", [''])");
    else
        return Gui::Command::getObjectCmd(obj,"(",", ['") + subs.front() + "'])";
}

std::string buildLinkListPythonStr(const std::vector<App::DocumentObject*> & objs)
{
    if ( objs.empty() ) {
        return "None";
    }

    std::string result("[");

    for (std::vector<App::DocumentObject*>::const_iterator o = objs.begin(); o != objs.end(); o++)
        result += Gui::Command::getObjectCmd(*o,0,",");
    result += "]";

    return result;
}

std::string buildLinkSubListPythonStr(const std::vector<App::DocumentObject*> & objs,
        const std::vector<std::string>& subs)
{
    if ( objs.empty() ) {
        return "None";
    }

    std::string result("[");

    assert (objs.size () == subs.size () );

    for (size_t i=0, objs_sz=objs.size(); i < objs_sz; i++) {
        if (objs[i] ) {
            result += '(';
            result += Gui::Command::getObjectCmd(objs[i]);
            result += ",\"";
            result += subs[i];
            result += "\"),";
        }
    }

    result += "]";

    return result;
}
}
