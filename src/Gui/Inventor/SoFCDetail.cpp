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

#include "PreCompiled.h"

#include "SoFCDetail.h"

SO_DETAIL_SOURCE(SoFCDetail);

SoFCDetail::SoFCDetail()
{
}

SoFCDetail::~SoFCDetail()
{
}

void
SoFCDetail::initClass()
{
  SO_DETAIL_INIT_CLASS(SoFCDetail, SoDetail);
}

SoDetail *
SoFCDetail::copy() const
{
  SoFCDetail *copy = new SoFCDetail();
  copy->indexArray = this->indexArray;
  copy->ctx = this->ctx;
  return copy;
}

void
SoFCDetail::setIndices(Type type, std::set<int> &&indices)
{
  if(type >= 0 && type < TypeMax)
    indexArray[type] = std::move(indices);
}

bool
SoFCDetail::addIndex(Type type, int index)
{
  if(type >= 0 && type < TypeMax)
    return indexArray[type].insert(index).second;
  return false;
}

bool
SoFCDetail::removeIndex(Type type, int index)
{
  if(type >= 0 && type < TypeMax)
    return indexArray[type].erase(index);
  return false;
}

const std::set<int> &
SoFCDetail::getIndices(Type type, void **ctx) const
{
  if(type < 0 || type >= TypeMax) {
    static std::set<int> none;
    return none;
  }
  if (ctx)
    *ctx = this->ctx[type];
  return indexArray[type];
}

void *
SoFCDetail::getContext(Type type) const
{
  if(type >= 0 && type < TypeMax)
    return this->ctx[type];
  return nullptr;
}

void
SoFCDetail::setContext(Type type, void *ctx)
{
  if(type >= 0 && type < TypeMax)
    this->ctx[type] = ctx;
}

//////////////////////////////////////////////////////////////

SO_DETAIL_SOURCE(SoFCFaceDetail);

void
SoFCFaceDetail::initClass()
{
  SO_DETAIL_INIT_CLASS(SoFCFaceDetail, SoFaceDetail);
}

SoDetail *
SoFCFaceDetail::copy() const
{
  SoFCFaceDetail *copy = new SoFCFaceDetail();
  if (this->getNumPoints()) {
    copy->setNumPoints(this->getNumPoints());
    for (int i = 0; i < this->getNumPoints(); i++) {
      copy->setPoint(i, this->getPoint(i));
    }
  }
  copy->setFaceIndex(this->getFaceIndex());
  copy->setPartIndex(this->getPartIndex());
  copy->setContext(this->getContext());
  return copy;
}

void
SoFCFaceDetail::setContext(void *ctx)
{
  this->ctx = ctx;
}

//////////////////////////////////////////////////////////////

SO_DETAIL_SOURCE(SoFCLineDetail);

void
SoFCLineDetail::initClass()
{
  SO_DETAIL_INIT_CLASS(SoFCLineDetail, SoLineDetail);
}

SoDetail *
SoFCLineDetail::copy() const
{
  SoFCLineDetail *copy = new SoFCLineDetail();
  *copy = *this;
  return copy;
}

void
SoFCLineDetail::setContext(void *ctx)
{
  this->ctx = ctx;
}

//////////////////////////////////////////////////////////////

SO_DETAIL_SOURCE(SoFCPointDetail);

void
SoFCPointDetail::initClass()
{
  SO_DETAIL_INIT_CLASS(SoFCPointDetail, SoPointDetail);
}

SoDetail *
SoFCPointDetail::copy() const
{
  SoFCPointDetail *copy = new SoFCPointDetail();
  *copy = *this;
  return copy;
}

void
SoFCPointDetail::setContext(void *ctx)
{
  this->ctx = ctx;
}
// vim: noai:ts=2:sw=2
