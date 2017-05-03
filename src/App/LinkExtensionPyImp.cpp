/****************************************************************************
 *   Copyright (c) 2017 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#include "Link.h"

#include "LinkExtensionPy.h"
#include "LinkExtensionPy.cpp"

using namespace App;

// returns a string which represent the object e.g. when printed in python
std::string LinkExtensionPy::representation(void) const
{
    auto object = getLinkExtensionPtr();
    const char *path = object->LinkedObject.getDocumentPath();
    const char *name = object->LinkedObject.getObjectName();
    std::stringstream str;
    str << "<Link";
    if(path) 
        str << ' ' << path <<'.';
    else
        str << ' ';
    if(name) str << name;
    str << '>';
    return str.str();
}

Py::String LinkExtensionPy::getLinkedObjectName(void) const
{
    auto object = this->getLinkExtensionPtr();
    const char *name = object->LinkedObject.getObjectName();
    return Py::String(std::string(name?name:""));
}

Py::String LinkExtensionPy::getLinkedDocumentPath(void) const
{
    auto object = this->getLinkExtensionPtr();
    const char *name = object->LinkedObject.getDocumentPath();
    return Py::String(std::string(name?name:""));
}

PyObject*  LinkExtensionPy::setLink(PyObject *args)
{
    char *sName,*sPath=0;
    PyObject *relative=Py_True;
    std::string name, path;
    if (!PyArg_ParseTuple(args, "s|sO", &sName,&sPath,&relative))
        return 0;

    auto object = this->getLinkExtensionPtr();
    object->LinkedObject.setValue(sName,sPath,PyObject_IsTrue(relative)?1:0);
    auto linked = object->LinkedObject.getValue();
    if(!linked) Py_Return;
    return linked->getPyObject();
}

PyObject *LinkExtensionPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int LinkExtensionPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0; 
}
