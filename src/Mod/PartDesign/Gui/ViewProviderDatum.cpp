/***************************************************************************
 *   Copyright (c) 2013 Jan Rheinlaender                                   *
 *                                   <jrheinlaender@users.sourceforge.net> *
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
# include <QApplication>
# include <QMessageBox>
# include <QAction>
# include <QMenu>
#endif

#include <Mod/Part/App/DatumFeature.h>
#include <Gui/Control.h>
#include <Gui/Command.h>
#include <Gui/Application.h>

#include "TaskDatumParameters.h"
#include "ViewProviderBody.h"
#include "Utils.h"

#include "ViewProviderDatum.h"

using namespace PartDesignGui;

PROPERTY_SOURCE_WITH_EXTENSIONS(PartDesignGui::ViewProviderDatum, Gui::ViewProviderDatum)

ViewProviderDatum::ViewProviderDatum()
{
    PartGui::ViewProviderAttachExtension::initExtension(this);
    oldWb = "";
}

ViewProviderDatum::~ViewProviderDatum()
{
}

void ViewProviderDatum::setupContextMenu(QMenu* menu, QObject* receiver, const char* member)
{
    QAction* act;
    act = menu->addAction(QObject::tr("Edit datum"), receiver, member);
    act->setData(QVariant((int)ViewProvider::Default));
}

bool ViewProviderDatum::setEdit(int ModNum)
{
    if (!ViewProvider::setEdit(ModNum))
        return false;
    // TODO Share this code with Features view providers somehow (2015-09-08, Fat-Zer)
    if (ModNum == ViewProvider::Default ) {
        // When double-clicking on the item for this datum feature the
        // object unsets and sets its edit mode without closing
        // the task panel
        Gui::TaskView::TaskDialog *dlg = Gui::Control().activeDialog();
        TaskDlgDatumParameters *datumDlg = qobject_cast<TaskDlgDatumParameters *>(dlg);
        if (datumDlg && datumDlg->getViewProvider() != this)
            datumDlg = 0; // another datum feature left open its task panel
        if (dlg && !datumDlg && !dlg->tryClose())
            return false;

        // clear the selection (convenience)
        Gui::Selection().clearSelection();

        oldWb = Gui::Command::assureWorkbench("PartDesignWorkbench");

        // start the edit dialog
        if (datumDlg)
            Gui::Control().showDialog(datumDlg);
        else
            Gui::Control().showDialog(new TaskDlgDatumParameters(this));

        return true;
    }
    else {
        return ViewProvider::setEdit(ModNum);
    }
}

bool ViewProviderDatum::doubleClicked(void)
{
    return getDocument()->setEdit(this, ViewProvider::Default);
}

void ViewProviderDatum::unsetEdit(int ModNum)
{
    // return to the WB we were in before editing the PartDesign feature
    Gui::Command::assureWorkbench(oldWb.c_str());

    if (ModNum == ViewProvider::Default) {
        // when pressing ESC make sure to close the dialog
        Gui::Control().closeDialog();
    }
    else {
        Gui::ViewProviderGeometryObject::unsetEdit(ModNum);
    }
}

Base::Vector3d ViewProviderDatum::getBasePoint () const
{
    if (auto datum = Base::freecad_dynamic_cast<Part::Datum>(getObject()))
        return datum->getBasePoint();
    return Base::Vector3d();
}
