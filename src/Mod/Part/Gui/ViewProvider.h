/***************************************************************************
 *   Copyright (c) 2004 Jürgen Riegel <juergen.riegel@web.de>              *
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


#ifndef PARTGUI_VIEWPROVIDERPART_H
#define PARTGUI_VIEWPROVIDERPART_H

#include <Gui/ViewProviderBuilder.h>
#include <Mod/Part/Gui/ViewProviderExt.h>

class SoSeparator;

namespace Part { struct ShapeHistory; }

namespace PartGui {

class ViewProviderShapeBuilder : public Gui::ViewProviderBuilder
{
public:
    ViewProviderShapeBuilder(){}
    ~ViewProviderShapeBuilder(){}
    virtual void buildNodes(const App::Property*, std::vector<SoNode*>&) const;
    void createShape(const App::Property*, SoSeparator*) const;
};

class PartGuiExport ViewProviderPart : public ViewProviderPartExt
{
    typedef ViewProviderPartExt inherited;
    PROPERTY_HEADER_WITH_OVERRIDE(PartGui::ViewProviderPart);

public:
    /// constructor
    ViewProviderPart();
    /// destructor
    virtual ~ViewProviderPart();
    virtual bool doubleClicked(void) override;

    virtual QPixmap getTagIcon() const;

    virtual bool canReplaceObject(App::DocumentObject *oldObj, App::DocumentObject *newObj) override;
    virtual bool reorderObjects(const std::vector<App::DocumentObject *> &objs, App::DocumentObject *before) override;
    virtual bool canReorderObject(App::DocumentObject *obj, App::DocumentObject *before) override;
    virtual bool canDropObjects() const override;
    virtual bool canDropObject(App::DocumentObject*) const override;
    virtual void dropObject(App::DocumentObject*) override;
    virtual bool canDragObjects() const override;
    virtual bool canDragObject(App::DocumentObject*) const override;
    virtual void dragObject(App::DocumentObject*) override;

    virtual void getExtraIcons(std::vector<std::pair<QByteArray, QPixmap> > &) const override;
    virtual QString getToolTip(const QByteArray &iconTag) const override;
    virtual std::vector<App::DocumentObject*> claimChildren(void) const override;
    virtual bool iconMouseEvent(QMouseEvent *ev, const QByteArray &iconTag) override;

    virtual void setupContextMenu(QMenu*, QObject*, const char*) override;

    virtual void updateData(const App::Property *) override;

protected:
    void applyColor(const Part::ShapeHistory& hist,
                    const std::vector<App::Color>& colBase,
                    std::vector<App::Color>& colBool);
    void applyTransparency(const float& transparency,
                    std::vector<App::Color>& colors);
};

} // namespace PartGui


#endif // PARTGUI_VIEWPROVIDERPART_H

