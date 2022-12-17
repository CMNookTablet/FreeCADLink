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

#ifndef GUI_DIALOG_DLGICONBROWSER_H
#define GUI_DIALOG_DLGICONBROWSER_H

#include <memory>
#include <QTimer>
#include <Base/Parameter.h>
#include "PropertyPage.h"

namespace Gui {

class PrefWidgetStates;

namespace Dialog {

class Ui_DlgIconBrowser;

class DlgIconBrowser : public CustomizeActionPage, public ParameterGrp::ObserverType
{
    Q_OBJECT
public:
    DlgIconBrowser(QWidget *parent = nullptr);
    ~DlgIconBrowser();
    void OnChange(Base::Subject<const char*> &, const char *reason);

public Q_SLOTS:
    void on_btnExport_clicked();
    void on_btnRefresh_clicked();
    void on_btnBrowse_clicked();
    void on_treeWidget_itemChanged(QTreeWidgetItem *, int);
    void on_btnSave_clicked();
    void on_editPrefix_textEdited(const QString &);
    void on_editFilter_textEdited(const QString &);
    void onTimer();
    void onTimerFilter();
    void on_spinBoxIconSize_valueChanged(int value);
    QString getPrefix();

protected:
    virtual void changeEvent(QEvent *e);
    virtual void onAddMacroAction(const QByteArray&) {}
    virtual void onRemoveMacroAction(const QByteArray&) {}
    virtual void onModifyMacroAction(const QByteArray&) {}

private:
    std::unique_ptr<Ui_DlgIconBrowser> ui;
    std::unique_ptr<Gui::PrefWidgetStates> widgetStates;
    std::map<QString, QStringList> keys;
    QTimer timer;
    QTimer timerFilter;
    QTimer timerRefresh;
    ParameterGrp::handle hGrp;
};

} // namespace Dialog
} // namespace Gui

#endif // GUI_DIALOG_DLGICONBROWSER_H
