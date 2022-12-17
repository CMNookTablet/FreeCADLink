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
# include <QApplication>
# include <QFileInfo>
# include <QMessageBox>
# include <QInputDialog>
#endif

#include "Application.h"
#include "PythonWrapper.h"
#include "FileDialogPy.h"
#include "FileDialogPy.cpp"

using namespace Gui;

std::string FileDialogPy::representation(void) const
{
    return std::string("<FileDialog object>");
}

static bool getOptions(PyObject *pyOption, QFileDialog::Options &options)
{
#if defined(FORCE_USE_QT_FILEDIALOG)
    options = QFileDialog::DontUseNativeDialog;
#else
    options = QFileDialog::Options();
#endif
    if (pyOption == Py_None)
        return true;
    long value = PyLong_AsLong(pyOption);
    if (PyErr_Occurred()) {
        PyErr_SetString(PyExc_TypeError, "Invalid QFileDialog options");
        return false;
    }
    options |= QFileDialog::Options(QFileDialog::Option(value));
    return true;
}

PyObject* FileDialogPy::getOpenFileName(PyObject *args, PyObject *kwd)
{
    PyObject *pyParent = Py_None, *pyOptions = Py_None;
    char *caption = nullptr, *dir = nullptr, *filter = nullptr, *selectedFilter = nullptr;
    static char *kwlist[] = {"parent","caption","dir","filter","selectedFilter","options",nullptr};
    if (!PyArg_ParseTupleAndKeywords(
                args, kwd, "|OzzzzO", kwlist,
                &pyParent, &caption, &dir, &filter, &selectedFilter, &pyOptions))
        return nullptr;

    if (!caption)
        caption = "";
    if (!dir)
        dir = "";
    if (!filter)
        filter = "";
    if (!selectedFilter)
        selectedFilter = "";

    PythonWrapper wrap;
    if (!wrap.loadCoreModule() ||
        !wrap.loadGuiModule() ||
        !wrap.loadWidgetsModule()) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to load Python wrapper for Qt");
        return nullptr;
    }

    QWidget *parent = nullptr;
    if (pyParent != Py_None) {
        parent = qobject_cast<QWidget*>(wrap.toQObject(Py::Object(pyParent)));
        if (!parent) {
            PyErr_SetString(PyExc_TypeError, "Invalid parent");
            return nullptr;
        }
    }

    QFileDialog::Options options;
    if (!getOptions(pyOptions, options))
        return nullptr;

    QString _selectedFilter = QString::fromUtf8(selectedFilter);
    QString res = FileDialog::getOpenFileName(
                                 parent,
                                 QString::fromUtf8(caption),
                                 QString::fromUtf8(dir),
                                 QString::fromUtf8(filter),
                                 &_selectedFilter,
                                 options);
    Py::TupleN tuple(Py::String(res.toUtf8().constData()),
                     Py::String(_selectedFilter.toUtf8().constData()));
    return Py::new_reference_to(tuple);
}

PyObject* FileDialogPy::getOpenFileNames(PyObject *args, PyObject *kwd)
{
    PyObject *pyParent = Py_None, *pyOptions = Py_None;
    char *caption = nullptr, *dir = nullptr, *filter = nullptr, *selectedFilter = nullptr;
    static char *kwlist[] = {"parent","caption","dir","filter","selectedFilter","options",nullptr};
    if (!PyArg_ParseTupleAndKeywords(
                args, kwd, "|OzzzzO", kwlist,
                &pyParent, &caption, &dir, &filter, &selectedFilter, &pyOptions))
        return nullptr;

    if (!caption)
        caption = "";
    if (!dir)
        dir = "";
    if (!filter)
        filter = "";
    if (!selectedFilter)
        selectedFilter = "";

    PythonWrapper wrap;
    if (!wrap.loadCoreModule() ||
        !wrap.loadGuiModule() ||
        !wrap.loadWidgetsModule()) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to load Python wrapper for Qt");
        return nullptr;
    }

    QWidget *parent = nullptr;
    if (pyParent != Py_None) {
        parent = qobject_cast<QWidget*>(wrap.toQObject(Py::Object(pyParent)));
        if (!parent) {
            PyErr_SetString(PyExc_TypeError, "Invalid parent");
            return nullptr;
        }
    }

    QFileDialog::Options options;
    if (!getOptions(pyOptions, options))
        return nullptr;

    QString _selectedFilter = QString::fromUtf8(selectedFilter);
    QStringList res = FileDialog::getOpenFileNames(
                                 parent,
                                 QString::fromUtf8(caption),
                                 QString::fromUtf8(dir),
                                 QString::fromUtf8(filter),
                                 &_selectedFilter,
                                 options);
    Py::List list;
    for (const auto &s : res)
        list.append(Py::String(s.toUtf8().constData()));
    Py::TupleN tuple(list, Py::String(_selectedFilter.toUtf8().constData()));
    return Py::new_reference_to(tuple);
}

PyObject* FileDialogPy::getSaveFileName(PyObject *args, PyObject *kwd)
{
    PyObject *pyParent = Py_None, *pyOptions = Py_None;
    char *caption = nullptr, *dir = nullptr, *filter = nullptr, *selectedFilter = nullptr;
    static char *kwlist[] = {"parent","caption","dir","filter","selectedFilter","options",nullptr};
    if (!PyArg_ParseTupleAndKeywords(
                args, kwd, "|OzzzzO", kwlist,
                &pyParent, &caption, &dir, &filter, &selectedFilter, &pyOptions))
        return nullptr;

    if (!caption)
        caption = "";
    if (!dir)
        dir = "";
    if (!filter)
        filter = "";
    if (!selectedFilter)
        selectedFilter = "";

    PythonWrapper wrap;
    if (!wrap.loadCoreModule() ||
        !wrap.loadGuiModule() ||
        !wrap.loadWidgetsModule()) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to load Python wrapper for Qt");
        return nullptr;
    }

    QWidget *parent = nullptr;
    if (pyParent != Py_None) {
        parent = qobject_cast<QWidget*>(wrap.toQObject(Py::Object(pyParent)));
        if (!parent) {
            PyErr_SetString(PyExc_TypeError, "Invalid parent");
            return nullptr;
        }
    }

    QFileDialog::Options options;
    if (!getOptions(pyOptions, options))
        return nullptr;

    QString _selectedFilter = QString::fromUtf8(selectedFilter);
    QString res = FileDialog::getSaveFileName(
                                 parent,
                                 QString::fromUtf8(caption),
                                 QString::fromUtf8(dir),
                                 QString::fromUtf8(filter),
                                 &_selectedFilter,
                                 options);
    Py::TupleN tuple(Py::String(res.toUtf8().constData()),
                     Py::String(_selectedFilter.toUtf8().constData()));
    return Py::new_reference_to(tuple);
}

PyObject* FileDialogPy::getExistingDirectory(PyObject *args, PyObject *kwd)
{
    PyObject *pyParent = Py_None, *pyOptions = Py_None;
    char *caption = nullptr, *dir = nullptr;
    static char *kwlist[] = {"parent","caption","dir","options",nullptr};
    if (!PyArg_ParseTupleAndKeywords(
                args, kwd, "|OzzO", kwlist,
                &pyParent, &caption, &dir, &pyOptions))
        return nullptr;

    if (!caption)
        caption = "";
    if (!dir)
        dir = "";

    PythonWrapper wrap;
    if (!wrap.loadCoreModule() ||
        !wrap.loadGuiModule() ||
        !wrap.loadWidgetsModule()) {
        PyErr_SetString(PyExc_RuntimeError, "Failed to load Python wrapper for Qt");
        return nullptr;
    }

    QWidget *parent = nullptr;
    if (pyParent != Py_None) {
        parent = qobject_cast<QWidget*>(wrap.toQObject(Py::Object(pyParent)));
        if (!parent) {
            PyErr_SetString(PyExc_TypeError, "Invalid parent");
            return nullptr;
        }
    }

    QFileDialog::Options options;
    if (!getOptions(pyOptions, options))
        return nullptr;

    QString res = FileDialog::getExistingDirectory(
                                 parent,
                                 QString::fromUtf8(caption),
                                 QString::fromUtf8(dir),
                                 options);

    return Py::new_reference_to(Py::String(res.toUtf8().constData()));
}

PyObject *FileDialogPy::getCustomAttributes(const char* /*attr*/) const
{
    return 0;
}

int FileDialogPy::setCustomAttributes(const char* /*attr*/, PyObject* /*obj*/)
{
    return 0;
}
