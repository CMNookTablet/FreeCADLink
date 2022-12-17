/****************************************************************************
 *   Copyright (c) 2022 Zheng Lei (realthunder) <realthunder.dev@gmail.com> *
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
import ReportViewParams
ReportViewParams.define()
]]]*/

// Auto generated code (Tools/params_utils.py:162)
#include <unordered_map>
#include <App/Application.h>
#include <App/DynamicProperty.h>
#include "ReportViewParams.h"
using namespace Gui;

// Auto generated code (Tools/params_utils.py:171)
namespace {
class ReportViewParamsP: public ParameterGrp::ObserverType {
public:
    ParameterGrp::handle handle;
    std::unordered_map<const char *,void(*)(ReportViewParamsP*),App::CStringHasher,App::CStringHasher> funcs;

    // Auto generated code (Tools/params_utils.py:181)
    boost::signals2::signal<void (const char*)> signalParamChanged;
    void signalAll()
    {
        signalParamChanged("checkShowReportViewOnWarning");
        signalParamChanged("checkShowReportViewOnError");
        signalParamChanged("checkShowReportViewOnNormalMessage");
        signalParamChanged("checkShowReportViewOnLogMessage");
        signalParamChanged("checkShowReportTimecode");
        signalParamChanged("LogMessageSize");
        signalParamChanged("CommandRedirect");

    // Auto generated code (Tools/params_utils.py:190)
    }
    bool checkShowReportViewOnWarning;
    bool checkShowReportViewOnError;
    bool checkShowReportViewOnNormalMessage;
    bool checkShowReportViewOnLogMessage;
    bool checkShowReportTimecode;
    long LogMessageSize;
    QString CommandRedirect;

    // Auto generated code (Tools/params_utils.py:199)
    ReportViewParamsP() {
        handle = App::GetApplication().GetParameterGroupByPath("User parameter:BaseApp/Preferences/OutputWindow");
        handle->Attach(this);

        checkShowReportViewOnWarning = handle->GetBool("checkShowReportViewOnWarning", true);
        funcs["checkShowReportViewOnWarning"] = &ReportViewParamsP::updatecheckShowReportViewOnWarning;
        checkShowReportViewOnError = handle->GetBool("checkShowReportViewOnError", true);
        funcs["checkShowReportViewOnError"] = &ReportViewParamsP::updatecheckShowReportViewOnError;
        checkShowReportViewOnNormalMessage = handle->GetBool("checkShowReportViewOnNormalMessage", false);
        funcs["checkShowReportViewOnNormalMessage"] = &ReportViewParamsP::updatecheckShowReportViewOnNormalMessage;
        checkShowReportViewOnLogMessage = handle->GetBool("checkShowReportViewOnLogMessage", false);
        funcs["checkShowReportViewOnLogMessage"] = &ReportViewParamsP::updatecheckShowReportViewOnLogMessage;
        checkShowReportTimecode = handle->GetBool("checkShowReportTimecode", true);
        funcs["checkShowReportTimecode"] = &ReportViewParamsP::updatecheckShowReportTimecode;
        LogMessageSize = handle->GetInt("LogMessageSize", 0);
        funcs["LogMessageSize"] = &ReportViewParamsP::updateLogMessageSize;
        CommandRedirect = QString::fromUtf8(handle->GetASCII("CommandRedirect", "").c_str());
        funcs["CommandRedirect"] = &ReportViewParamsP::updateCommandRedirect;
    }

    // Auto generated code (Tools/params_utils.py:213)
    ~ReportViewParamsP() {
    }

    // Auto generated code (Tools/params_utils.py:218)
    void OnChange(Base::Subject<const char*> &, const char* sReason) {
        if(!sReason)
            return;
        auto it = funcs.find(sReason);
        if(it == funcs.end())
            return;
        it->second(this);
        signalParamChanged(sReason);
    }


    // Auto generated code (Tools/params_utils.py:234)
    static void updatecheckShowReportViewOnWarning(ReportViewParamsP *self) {
        self->checkShowReportViewOnWarning = self->handle->GetBool("checkShowReportViewOnWarning", true);
    }
    // Auto generated code (Tools/params_utils.py:234)
    static void updatecheckShowReportViewOnError(ReportViewParamsP *self) {
        self->checkShowReportViewOnError = self->handle->GetBool("checkShowReportViewOnError", true);
    }
    // Auto generated code (Tools/params_utils.py:234)
    static void updatecheckShowReportViewOnNormalMessage(ReportViewParamsP *self) {
        self->checkShowReportViewOnNormalMessage = self->handle->GetBool("checkShowReportViewOnNormalMessage", false);
    }
    // Auto generated code (Tools/params_utils.py:234)
    static void updatecheckShowReportViewOnLogMessage(ReportViewParamsP *self) {
        self->checkShowReportViewOnLogMessage = self->handle->GetBool("checkShowReportViewOnLogMessage", false);
    }
    // Auto generated code (Tools/params_utils.py:234)
    static void updatecheckShowReportTimecode(ReportViewParamsP *self) {
        self->checkShowReportTimecode = self->handle->GetBool("checkShowReportTimecode", true);
    }
    // Auto generated code (Tools/params_utils.py:234)
    static void updateLogMessageSize(ReportViewParamsP *self) {
        self->LogMessageSize = self->handle->GetInt("LogMessageSize", 0);
    }
    // Auto generated code (Tools/params_utils.py:234)
    static void updateCommandRedirect(ReportViewParamsP *self) {
        self->CommandRedirect = QString::fromUtf8(self->handle->GetASCII("CommandRedirect", "").c_str());
    }
};

// Auto generated code (Tools/params_utils.py:252)
ReportViewParamsP *instance() {
    static ReportViewParamsP *inst = new ReportViewParamsP;
    return inst;
}

} // Anonymous namespace

// Auto generated code (Tools/params_utils.py:261)
ParameterGrp::handle ReportViewParams::getHandle() {
    return instance()->handle;
}

// Auto generated code (Tools/params_utils.py:269)
boost::signals2::signal<void (const char*)> &
ReportViewParams::signalParamChanged() {
    return instance()->signalParamChanged;
}

// Auto generated code (Tools/params_utils.py:276)
void signalAll() {
    instance()->signalAll();
}

// Auto generated code (Tools/params_utils.py:284)
const char *ReportViewParams::doccheckShowReportViewOnWarning() {
    return "";
}

// Auto generated code (Tools/params_utils.py:290)
const bool & ReportViewParams::getcheckShowReportViewOnWarning() {
    return instance()->checkShowReportViewOnWarning;
}

// Auto generated code (Tools/params_utils.py:296)
const bool & ReportViewParams::defaultcheckShowReportViewOnWarning() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:303)
void ReportViewParams::setcheckShowReportViewOnWarning(const bool &v) {
    instance()->handle->SetBool("checkShowReportViewOnWarning",v);
    instance()->checkShowReportViewOnWarning = v;
}

// Auto generated code (Tools/params_utils.py:310)
void ReportViewParams::removecheckShowReportViewOnWarning() {
    instance()->handle->RemoveBool("checkShowReportViewOnWarning");
}

// Auto generated code (Tools/params_utils.py:284)
const char *ReportViewParams::doccheckShowReportViewOnError() {
    return "";
}

// Auto generated code (Tools/params_utils.py:290)
const bool & ReportViewParams::getcheckShowReportViewOnError() {
    return instance()->checkShowReportViewOnError;
}

// Auto generated code (Tools/params_utils.py:296)
const bool & ReportViewParams::defaultcheckShowReportViewOnError() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:303)
void ReportViewParams::setcheckShowReportViewOnError(const bool &v) {
    instance()->handle->SetBool("checkShowReportViewOnError",v);
    instance()->checkShowReportViewOnError = v;
}

// Auto generated code (Tools/params_utils.py:310)
void ReportViewParams::removecheckShowReportViewOnError() {
    instance()->handle->RemoveBool("checkShowReportViewOnError");
}

// Auto generated code (Tools/params_utils.py:284)
const char *ReportViewParams::doccheckShowReportViewOnNormalMessage() {
    return "";
}

// Auto generated code (Tools/params_utils.py:290)
const bool & ReportViewParams::getcheckShowReportViewOnNormalMessage() {
    return instance()->checkShowReportViewOnNormalMessage;
}

// Auto generated code (Tools/params_utils.py:296)
const bool & ReportViewParams::defaultcheckShowReportViewOnNormalMessage() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:303)
void ReportViewParams::setcheckShowReportViewOnNormalMessage(const bool &v) {
    instance()->handle->SetBool("checkShowReportViewOnNormalMessage",v);
    instance()->checkShowReportViewOnNormalMessage = v;
}

// Auto generated code (Tools/params_utils.py:310)
void ReportViewParams::removecheckShowReportViewOnNormalMessage() {
    instance()->handle->RemoveBool("checkShowReportViewOnNormalMessage");
}

// Auto generated code (Tools/params_utils.py:284)
const char *ReportViewParams::doccheckShowReportViewOnLogMessage() {
    return "";
}

// Auto generated code (Tools/params_utils.py:290)
const bool & ReportViewParams::getcheckShowReportViewOnLogMessage() {
    return instance()->checkShowReportViewOnLogMessage;
}

// Auto generated code (Tools/params_utils.py:296)
const bool & ReportViewParams::defaultcheckShowReportViewOnLogMessage() {
    const static bool def = false;
    return def;
}

// Auto generated code (Tools/params_utils.py:303)
void ReportViewParams::setcheckShowReportViewOnLogMessage(const bool &v) {
    instance()->handle->SetBool("checkShowReportViewOnLogMessage",v);
    instance()->checkShowReportViewOnLogMessage = v;
}

// Auto generated code (Tools/params_utils.py:310)
void ReportViewParams::removecheckShowReportViewOnLogMessage() {
    instance()->handle->RemoveBool("checkShowReportViewOnLogMessage");
}

// Auto generated code (Tools/params_utils.py:284)
const char *ReportViewParams::doccheckShowReportTimecode() {
    return "";
}

// Auto generated code (Tools/params_utils.py:290)
const bool & ReportViewParams::getcheckShowReportTimecode() {
    return instance()->checkShowReportTimecode;
}

// Auto generated code (Tools/params_utils.py:296)
const bool & ReportViewParams::defaultcheckShowReportTimecode() {
    const static bool def = true;
    return def;
}

// Auto generated code (Tools/params_utils.py:303)
void ReportViewParams::setcheckShowReportTimecode(const bool &v) {
    instance()->handle->SetBool("checkShowReportTimecode",v);
    instance()->checkShowReportTimecode = v;
}

// Auto generated code (Tools/params_utils.py:310)
void ReportViewParams::removecheckShowReportTimecode() {
    instance()->handle->RemoveBool("checkShowReportTimecode");
}

// Auto generated code (Tools/params_utils.py:284)
const char *ReportViewParams::docLogMessageSize() {
    return "";
}

// Auto generated code (Tools/params_utils.py:290)
const long & ReportViewParams::getLogMessageSize() {
    return instance()->LogMessageSize;
}

// Auto generated code (Tools/params_utils.py:296)
const long & ReportViewParams::defaultLogMessageSize() {
    const static long def = 0;
    return def;
}

// Auto generated code (Tools/params_utils.py:303)
void ReportViewParams::setLogMessageSize(const long &v) {
    instance()->handle->SetInt("LogMessageSize",v);
    instance()->LogMessageSize = v;
}

// Auto generated code (Tools/params_utils.py:310)
void ReportViewParams::removeLogMessageSize() {
    instance()->handle->RemoveInt("LogMessageSize");
}

// Auto generated code (Tools/params_utils.py:284)
const char *ReportViewParams::docCommandRedirect() {
    return QT_TRANSLATE_NOOP("ReportViewParams",
"Prefix for marking python command in message to be redirected to Python console\n"
"This is used as a debug help for output command from external libraries");
}

// Auto generated code (Tools/params_utils.py:290)
const QString & ReportViewParams::getCommandRedirect() {
    return instance()->CommandRedirect;
}

// Auto generated code (Tools/params_utils.py:296)
const QString & ReportViewParams::defaultCommandRedirect() {
    const static QString def = QStringLiteral("");
    return def;
}

// Auto generated code (Tools/params_utils.py:303)
void ReportViewParams::setCommandRedirect(const QString &v) {
    instance()->handle->SetASCII("CommandRedirect",v.toUtf8().constData());
    instance()->CommandRedirect = v;
}

// Auto generated code (Tools/params_utils.py:310)
void ReportViewParams::removeCommandRedirect() {
    instance()->handle->RemoveASCII("CommandRedirect");
}
//[[[end]]]

