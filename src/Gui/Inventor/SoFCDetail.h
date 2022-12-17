/****************************************************************************
 *   Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>*
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

#ifndef FC_SOFCDETAIL_H
#define FC_SOFCDETAIL_H

#include <set>
#include <array>
#include <Inventor/details/SoSubDetail.h>
#include <Inventor/details/SoDetail.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>

class GuiExport SoFCDetail : public SoDetail
{
  SO_DETAIL_HEADER(SoFCDetail);

public:
  SoFCDetail();
  virtual ~SoFCDetail();

  static void initClass();
  virtual SoDetail * copy() const;

  enum Type {
    Vertex,
    Edge,
    Face,
    TypeMax,
  };
  const std::set<int> &getIndices(Type type, void **ctx=nullptr) const;
  void *getContext(Type type) const;
  void setContext(Type type, void *ctx);
  void setIndices(Type type, std::set<int> &&indices);
  bool addIndex(Type type, int index);
  bool removeIndex(Type type, int index);

private:
  std::array<std::set<int>, TypeMax> indexArray;
  std::array<void*, TypeMax> ctx = {};
};

class GuiExport SoFCFaceDetail : public SoFaceDetail
{
  SO_DETAIL_HEADER(SoFCFaceDetail);
public:
  static void initClass();
  virtual SoDetail * copy() const;
  void setContext(void* ctx);
  void* getContext() const {return ctx;}

private:
  void* ctx = nullptr;
};

class GuiExport SoFCLineDetail : public SoLineDetail
{
  SO_DETAIL_HEADER(SoFCLineDetail);
public:
  static void initClass();
  virtual SoDetail * copy() const;
  void setContext(void* ctx);
  void* getContext() const {return ctx;}

private:
  void* ctx = nullptr;
};

class GuiExport SoFCPointDetail : public SoPointDetail
{
  SO_DETAIL_HEADER(SoFCPointDetail);
public:
  static void initClass();
  virtual SoDetail * copy() const;
  void setContext(void* ctx);
  void* getContext() const {return ctx;}

private:
  void* ctx = nullptr;
};

#endif // FC_SOFCDETAIL_H
// vim: noai:ts=2:sw=2
