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


#ifndef PARTGUI_ViewProviderAddSub_H
#define PARTGUI_ViewProviderAddSub_H

#include <App/DocumentObserver.h>
#include "ViewProvider.h"

namespace PartDesignGui {

class PartDesignGuiExport ViewProviderAddSub : public ViewProvider
{
    PROPERTY_HEADER(PartDesignGui::ViewProviderAddSub);

public:
    App::PropertyColor AddSubColor;

    /// constructor
    ViewProviderAddSub();
    /// destructor
    virtual ~ViewProviderAddSub();
    
    virtual void attach(App::DocumentObject*);
    virtual void reattach(App::DocumentObject *);
    virtual void beforeDelete();
    virtual void updateData(const App::Property*);
    virtual void onChanged(const App::Property *p);
    virtual bool setEdit(int ModNum);
    virtual void unsetEdit(int ModNum);
    bool isPreviewMode() const;
    virtual void finishRestoring();
    
    void setPreviewDisplayMode(bool);

protected: 
    void checkAddSubColor();
    virtual void setAddSubColor(const App::Color &color, float t);
    virtual void updateAddSubShapeIndicator();
    virtual PartGui::ViewProviderPart * getAddSubView();

protected:
    Gui::CoinPtr<SoSeparator>   previewGroup;

private:
    int                         defaultChild;
    std::string                 displayMode;
    App::DocumentObjectT        baseFeature;
    int                         baseTail;
    std::unique_ptr<PartGui::ViewProviderPart> pAddSubView;
};

} // namespace PartDesignGui


#endif // PARTGUI_ViewProviderBoolean_H
