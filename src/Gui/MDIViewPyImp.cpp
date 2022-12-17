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

#ifndef _PreComp_
# include <sstream>
#endif

#include <App/DocumentObjectPy.h>
#include <Gui/MDIViewPy.h>
#include <Gui/MDIViewPy.cpp>

using namespace Gui;

// returns a string which represent the object e.g. when printed in python
std::string MDIViewPy::representation(void) const
{
    return std::string("<MDIView>");
}

PyObject *MDIViewPy::getCustomAttributes(const char* attr) const
{
    return App::PropertyContainerPy::getCustomAttributes(attr);
}

int MDIViewPy::setCustomAttributes(const char* /*attr*/, PyObject * /*obj*/)
{
    return 0;
}

PyObject* MDIViewPy::message(PyObject *args)
{
    const char **ppReturn = 0;
    char *psMsgStr;
    if (!PyArg_ParseTuple(args, "s;Message string needed (string)",&psMsgStr))
        return nullptr;

    try {
        getMDIViewPtr()->onMsg(psMsgStr,ppReturn);
        Py_Return;
    } PY_CATCH
}

PyObject* MDIViewPy::fitAll(PyObject *args)
{
    if (!PyArg_ParseTuple(args, ""))
        return nullptr;

    try {
        getMDIViewPtr()->viewAll();
        Py_Return;
    } PY_CATCH
}

PyObject* MDIViewPy::setActiveObject(PyObject *args)
{
    PyObject* docObject = Py_None;
    char* name;
    char *subname = 0;
    if (!PyArg_ParseTuple(args, "s|Os", &name, &docObject, &subname))
        return nullptr;

    try {
        if (docObject == Py_None) {
            getMDIViewPtr()->setActiveObject(0, name);
        }
        else {
            if (!PyObject_TypeCheck(docObject, &App::DocumentObjectPy::Type))
                throw Py::TypeError("Expect the second argument to be a document object or None");

            App::DocumentObject* obj = static_cast<App::DocumentObjectPy*>(docObject)->getDocumentObjectPtr();
            getMDIViewPtr()->setActiveObject(obj, name, subname);
        }
        Py_Return;
    } PY_CATCH
}

PyObject* MDIViewPy::getActiveObject(PyObject *args)
{
    const char* name;
    PyObject *resolve = Py_True;
    if (!PyArg_ParseTuple(args, "s|O", &name,&resolve))
        return nullptr;

    App::DocumentObject *parent = nullptr;
    std::string subname;
    App::DocumentObject* obj = nullptr;
    try {
        obj = getMDIViewPtr()->getActiveObject<App::DocumentObject*>(name,&parent,&subname);
        if (!obj)
            Py_Return;
        if (PyObject_IsTrue(resolve))
            return obj->getPyObject();
        return Py::new_reference_to(Py::TupleN(
                Py::asObject(obj->getPyObject()),
                parent ? Py::asObject(parent->getPyObject()) : Py::Object(),
                Py::String(subname.c_str())));
    } PY_CATCH
}
