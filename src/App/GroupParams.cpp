/****************************************************************************
 *   Copyright (c) 2021 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
 *                                                                          *
 *   This file is part of the FreeCAD CAx development system.               *
 *                                                                          *
 *   This library is free software; you can redistribute it and/or          *
 *   modify it under the terms of the GNU Library General Public            *
 *   License as published by the Free Software Foundation; either           *
 *   version 2 of the License, or (at your option) any later version.       *
 *                                                                          *
 *   This library  is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          *
 *   GNU Library General Public License for more details.                   *
 *                                                                          *
 *   You should have received a copy of the GNU Library General Public      *
 *   License along with this library; see the file COPYING.LIB. If not,     *
 *   write to the Free Software Foundation, Inc., 59 Temple Place,          *
 *   Suite 330, Boston, MA  02111-1307, USA                                 *
 *                                                                          *
 ****************************************************************************/

#include "PreCompiled.h"

/*[[[cog
import GroupParams
GroupParams.define()
]]]*/

// Auto generated code. See class document of GroupParams.
#include <unordered_map>
#include <App/Application.h>
#include <App/DynamicProperty.h>
#include "GroupParams.h"
using namespace App;

namespace {

// Auto generated code. See class document of GroupParams.
class GroupParamsP: public ParameterGrp::ObserverType {
public:
    // Auto generated code. See class document of GroupParams.
    ParameterGrp::handle handle;

    // Auto generated code. See class document of GroupParams.
    std::unordered_map<const char *,void(*)(GroupParamsP*),App::CStringHasher,App::CStringHasher> funcs;

    bool ClaimAllChildren; // Auto generated code. See class document of GroupParams.
    bool KeepHiddenChildren; // Auto generated code. See class document of GroupParams.
    bool ExportChildren; // Auto generated code. See class document of GroupParams.
    bool CreateOrigin; // Auto generated code. See class document of GroupParams.
    bool GeoGroupAllowCrossLink; // Auto generated code. See class document of GroupParams.

    // Auto generated code. See class document of GroupParams.
    GroupParamsP() {
        handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/Group");
        handle->Attach(this);

        ClaimAllChildren = handle->GetBool("ClaimAllChildren", true);
        funcs["ClaimAllChildren"] = &GroupParamsP::updateClaimAllChildren;
        KeepHiddenChildren = handle->GetBool("KeepHiddenChildren", true);
        funcs["KeepHiddenChildren"] = &GroupParamsP::updateKeepHiddenChildren;
        ExportChildren = handle->GetBool("ExportChildren", true);
        funcs["ExportChildren"] = &GroupParamsP::updateExportChildren;
        CreateOrigin = handle->GetBool("CreateOrigin", false);
        funcs["CreateOrigin"] = &GroupParamsP::updateCreateOrigin;
        GeoGroupAllowCrossLink = handle->GetBool("GeoGroupAllowCrossLink", false);
        funcs["GeoGroupAllowCrossLink"] = &GroupParamsP::updateGeoGroupAllowCrossLink;
    }

    // Auto generated code. See class document of GroupParams.
    ~GroupParamsP() {
    }

    // Auto generated code. See class document of GroupParams.
    void OnChange(Base::Subject<const char*> &, const char* sReason) {
        if(!sReason)
            return;
        auto it = funcs.find(sReason);
        if(it == funcs.end())
            return;
        it->second(this);
    }


    // Auto generated code. See class document of GroupParams.
    static void updateClaimAllChildren(GroupParamsP *self) {
        self->ClaimAllChildren = self->handle->GetBool("ClaimAllChildren", true);
    }
    // Auto generated code. See class document of GroupParams.
    static void updateKeepHiddenChildren(GroupParamsP *self) {
        self->KeepHiddenChildren = self->handle->GetBool("KeepHiddenChildren", true);
    }
    // Auto generated code. See class document of GroupParams.
    static void updateExportChildren(GroupParamsP *self) {
        self->ExportChildren = self->handle->GetBool("ExportChildren", true);
    }
    // Auto generated code. See class document of GroupParams.
    static void updateCreateOrigin(GroupParamsP *self) {
        self->CreateOrigin = self->handle->GetBool("CreateOrigin", false);
    }
    // Auto generated code. See class document of GroupParams.
    static void updateGeoGroupAllowCrossLink(GroupParamsP *self) {
        self->GeoGroupAllowCrossLink = self->handle->GetBool("GeoGroupAllowCrossLink", false);
    }
};

// Auto generated code. See class document of GroupParams.
GroupParamsP *instance() {
    static GroupParamsP *inst = new GroupParamsP;
    return inst;
}

} // Anonymous namespace

// Auto generated code. See class document of GroupParams.
ParameterGrp::handle GroupParams::getHandle() {
    return instance()->handle;
}

// Auto generated code. See class document of GroupParams.
const char *GroupParams::docClaimAllChildren() {
    return QT_TRANSLATE_NOOP("GroupParams",
"Claim all children objects in tree view. If disabled, then only claim\n"
"children that are not claimed by other children.");
}

// Auto generated code. See class document of GroupParams.
const bool & GroupParams::getClaimAllChildren() {
    return instance()->ClaimAllChildren;
}

// Auto generated code. See class document of GroupParams.
const bool & GroupParams::defaultClaimAllChildren() {
    const static bool def = true;
    return def;
}

// Auto generated code. See class document of GroupParams.
void GroupParams::setClaimAllChildren(const bool &v) {
    instance()->handle->SetBool("ClaimAllChildren",v);
    instance()->ClaimAllChildren = v;
}

// Auto generated code. See class document of GroupParams.
void GroupParams::removeClaimAllChildren() {
    instance()->handle->RemoveBool("ClaimAllChildren");
}

// Auto generated code. See class document of GroupParams.
const char *GroupParams::docKeepHiddenChildren() {
    return QT_TRANSLATE_NOOP("GroupParams",
"Remember invisible children objects and keep those objects hidden\n"
"when the group is made visible.");
}

// Auto generated code. See class document of GroupParams.
const bool & GroupParams::getKeepHiddenChildren() {
    return instance()->KeepHiddenChildren;
}

// Auto generated code. See class document of GroupParams.
const bool & GroupParams::defaultKeepHiddenChildren() {
    const static bool def = true;
    return def;
}

// Auto generated code. See class document of GroupParams.
void GroupParams::setKeepHiddenChildren(const bool &v) {
    instance()->handle->SetBool("KeepHiddenChildren",v);
    instance()->KeepHiddenChildren = v;
}

// Auto generated code. See class document of GroupParams.
void GroupParams::removeKeepHiddenChildren() {
    instance()->handle->RemoveBool("KeepHiddenChildren");
}

// Auto generated code. See class document of GroupParams.
const char *GroupParams::docExportChildren() {
    return QT_TRANSLATE_NOOP("GroupParams",
"Export visible children (e.g. when doing STEP export). Note, that once this option\n"
"is enabled, the group object will be touched when its child toggles visibility.");
}

// Auto generated code. See class document of GroupParams.
const bool & GroupParams::getExportChildren() {
    return instance()->ExportChildren;
}

// Auto generated code. See class document of GroupParams.
const bool & GroupParams::defaultExportChildren() {
    const static bool def = true;
    return def;
}

// Auto generated code. See class document of GroupParams.
void GroupParams::setExportChildren(const bool &v) {
    instance()->handle->SetBool("ExportChildren",v);
    instance()->ExportChildren = v;
}

// Auto generated code. See class document of GroupParams.
void GroupParams::removeExportChildren() {
    instance()->handle->RemoveBool("ExportChildren");
}

// Auto generated code. See class document of GroupParams.
const char *GroupParams::docCreateOrigin() {
    return QT_TRANSLATE_NOOP("GroupParams",
"Create all origin features when the origin group is created. If Disabled\n"
"The origin features will only be created when the origin group is expanded\n"
"for the first time.");
}

// Auto generated code. See class document of GroupParams.
const bool & GroupParams::getCreateOrigin() {
    return instance()->CreateOrigin;
}

// Auto generated code. See class document of GroupParams.
const bool & GroupParams::defaultCreateOrigin() {
    const static bool def = false;
    return def;
}

// Auto generated code. See class document of GroupParams.
void GroupParams::setCreateOrigin(const bool &v) {
    instance()->handle->SetBool("CreateOrigin",v);
    instance()->CreateOrigin = v;
}

// Auto generated code. See class document of GroupParams.
void GroupParams::removeCreateOrigin() {
    instance()->handle->RemoveBool("CreateOrigin");
}

// Auto generated code. See class document of GroupParams.
const char *GroupParams::docGeoGroupAllowCrossLink() {
    return QT_TRANSLATE_NOOP("GroupParams",
"Allow objects to be contained in more than one GeoFeatureGroup (e.g. App::Part).\n"
"If diabled, adding an object to one group will auto remove it from other groups.\n"
"WARNING! Disabling this option may produce an invalid group after changing its children.");
}

// Auto generated code. See class document of GroupParams.
const bool & GroupParams::getGeoGroupAllowCrossLink() {
    return instance()->GeoGroupAllowCrossLink;
}

// Auto generated code. See class document of GroupParams.
const bool & GroupParams::defaultGeoGroupAllowCrossLink() {
    const static bool def = false;
    return def;
}

// Auto generated code. See class document of GroupParams.
void GroupParams::setGeoGroupAllowCrossLink(const bool &v) {
    instance()->handle->SetBool("GeoGroupAllowCrossLink",v);
    instance()->GeoGroupAllowCrossLink = v;
}

// Auto generated code. See class document of GroupParams.
void GroupParams::removeGeoGroupAllowCrossLink() {
    instance()->handle->RemoveBool("GeoGroupAllowCrossLink");
}
//[[[end]]]
