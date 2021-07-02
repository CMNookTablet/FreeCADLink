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

#include <iostream>
#include <unordered_map>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/elements/SoTextureEnabledElement.h>
#include <Inventor/elements/SoOverrideElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoLinePatternElement.h>
#include <Inventor/elements/SoLineWidthElement.h>
#include <Inventor/elements/SoPointSizeElement.h>
#include <Inventor/elements/SoDrawStyleElement.h>
#include <Inventor/elements/SoMaterialBindingElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoPolygonOffsetElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoTextureUnitElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoMultiTextureImageElement.h>
#include <Inventor/elements/SoMultiTextureMatrixElement.h>
#include <Inventor/elements/SoShapeHintsElement.h>
#include <Inventor/elements/SoLightModelElement.h>
#include <Inventor/elements/SoDepthBufferElement.h>
#include <Inventor/annex/FXViz/elements/SoShadowStyleElement.h>
#include <Inventor/nodes/SoGroup.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoDepthBuffer.h>
#include <Inventor/nodes/SoLight.h>
#include <Inventor/nodes/SoClipPlane.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/SbBox3f.h>

#include "../InventorBase.h"
#include "../ViewParams.h"
#include "../SoFCUnifiedSelection.h"
#include "SoFCRenderCache.h"
#include "SoFCVertexCache.h"
#include "SoFCDetail.h"
#include "SoFCDiffuseElement.h"
#include "SoFCDisplayModeElement.h"
#include "SoFCShapeInfo.h"

#include <Gui/ViewProviderLink.h>

#include "ColorDiff/ColorUtils.cpp"

using namespace Gui;

typedef CoinPtr<SoFCVertexCache> VertexCachePtr;
typedef CoinPtr<SoFCRenderCache> RenderCachePtr;
typedef SoFCRenderCache::Material Material;
typedef SoFCRenderCache::VertexCacheEntry VertexCacheEntry;

struct CacheEntry {
  RenderCachePtr cache;
  VertexCachePtr vcache;
  Material material;
  SbMatrix matrix;
  bool resetmatrix;
  bool identity;
  CoinPtr<SoNode> proxy;
  int idxstart;
  int idxend;

  CacheEntry(const SbMatrix & m,
             bool iden, bool reset,
             SoFCRenderCache * c,
             SoFCVertexCache *vc,
             SoNode *proxy = nullptr,
             int istart = 0,
             int iend = 0)
    :cache(c), vcache(vc), resetmatrix(reset), identity(iden)
    ,proxy(proxy), idxstart(istart), idxend(iend)
  {
    if (!identity) this->matrix = m;
  }
};

class SoFCRenderCacheP {
public:
  SoFCRenderCacheP()
  {
  }

  ~SoFCRenderCacheP()
  {
  }

  void captureMaterial(SoState * state);

  Material mergeMaterial(const SbMatrix &matrix,
                         bool &identity,
                         const Material &parent,
                         const Material &child);

  void finalizeMaterial(Material & material);

  void addChildCache(SoState *state,
                     SoFCRenderCache * cache,
                     SoFCVertexCache *vcache,
                     bool opencache);

  const SoShapeHintsElement * shapehintselement;
  const SoShadowStyleElement * shadowstyleelement;
  const SoLinePatternElement * linepatternelement;
  const SoLineWidthElement * linewidthelement;
  const SoPointSizeElement * pointsizeelement;
  const SoPolygonOffsetElement * polygonoffsetelement;
  const SoDrawStyleElement * drawstyleelement;
  const SoMaterialBindingElement * materialbindingelement;

  SoFCRenderCache::VertexCacheMap vcachemap;

  std::vector<CacheEntry> caches;
  SbFCUniqueId nodeid;
  SoFCSelectionRoot *selnode = nullptr;

#ifdef FCCOIN_TRACE_CACHE_NAME
  SbName nodename;
#endif

  Material material;
  uint32_t facecolor;
  uint32_t hiddenlinecolor;
  float facetransp;
  bool resetmatrix;
  bool resetclip = false;
};

template<class T>
static inline const T * constElement(SoState * state)
{
  // calling SoState::getConstElement() instead of SoElement::getConstElement()
  // to avoid cache dependency
  return static_cast<const T *>(state->getConstElement(T::getClassStackIndex()));
}

#define PRIVATE(obj) ((obj)->pimpl)

SoFCRenderCache::SoFCRenderCache(SoState *state, SoNode *node)
  : SoCache(state), pimpl(new SoFCRenderCacheP)
{
  PRIVATE(this)->nodeid = node->getNodeId();
  if (node && node->isOfType(SoFCSelectionRoot::getClassTypeId())) {
    // DO NOT add reference. The cache will be monitored by a node sensor
    // inside SoFCRenderCacheManager, which is supposed to release the cache if
    // the node is destroyed. So must not add reference here.
    PRIVATE(this)->selnode = static_cast<SoFCSelectionRoot*>(node);
  }

#ifdef FCCOIN_TRACE_CACHE_NAME
  PRIVATE(this)->nodename = node->getName();
  if (PRIVATE(this)->nodename.getLength() == 0) {
    auto obj = ViewProviderLink::linkedObjectByNode(node);
    if (obj) {
      std::string name("_");
      name += obj->getNameInDocument();
      PRIVATE(this)->nodename = name.c_str();
    }
  }
#endif
}

SoFCRenderCache::~SoFCRenderCache()
{
  delete pimpl;
}

static const SbMatrix matrixidentity(SbMatrix::identity());

void SoFCRenderCache::initClass()
{
  SO_ENABLE(SoCallbackAction, SoShadowStyleElement);
  SoFCDiffuseElement::initClass();
}

void SoFCRenderCache::resetNode()
{
  PRIVATE(this)->selnode = nullptr;
}

void SoFCRenderCache::cleanup()
{
  SoFCDiffuseElement::cleanup();
}

static inline std::bitset<32>
getOverrideFlags(SoState * state)
{
  std::bitset<32> res;
  uint32_t flags = SoOverrideElement::getFlags(state);
  if (flags & SoOverrideElement::AMBIENT_COLOR)
    res.set(Material::FLAG_AMBIENT);
  if (flags & SoOverrideElement::DIFFUSE_COLOR)
    res.set(Material::FLAG_DIFFUSE);
  if (flags & SoOverrideElement::DRAW_STYLE)
    res.set(Material::FLAG_DRAW_STYLE);
  if (flags & SoOverrideElement::EMISSIVE_COLOR)
    res.set(Material::FLAG_EMISSIVE);
  if (flags & SoOverrideElement::LIGHT_MODEL)
    res.set(Material::FLAG_LIGHT_MODEL);
  if (flags & SoOverrideElement::LINE_PATTERN)
    res.set(Material::FLAG_LINE_PATTERN);
  if (flags & SoOverrideElement::LINE_WIDTH)
    res.set(Material::FLAG_LINE_WIDTH);
  if (flags & SoOverrideElement::MATERIAL_BINDING)
    res.set(Material::FLAG_MATERIAL_BINDING);
  if (flags & SoOverrideElement::POINT_SIZE)
    res.set(Material::FLAG_POINT_SIZE);
  if (flags & SoOverrideElement::SHAPE_HINTS)
    res.set(Material::FLAG_SHAPE_HINTS);
  if (flags & SoOverrideElement::SHININESS)
    res.set(Material::FLAG_SHININESS);
  if (flags & SoOverrideElement::SPECULAR_COLOR)
    res.set(Material::FLAG_SPECULAR);
  if (flags & SoOverrideElement::POLYGON_OFFSET)
    res.set(Material::FLAG_POLYGON_OFFSET);
  if (flags & SoOverrideElement::TRANSPARENCY)
    res.set(Material::FLAG_TRANSPARENCY);
  return res;
}

void
SoFCRenderCache::Material::init(SoState * state)
{
  this->resetclip = false;
  this->depthtest = true;
  this->depthclamp = false;
  this->depthfunc = SoDepthBuffer::LEQUAL;
  this->depthwrite = true;
  this->order = 0;
  this->annotation = 0;
  this->diffuse = 0xff;
  this->hiddenlinecolor = 0;
  this->ambient = 0xff;
  this->emissive = 0xff;
  this->specular = 0xff;
  this->linewidth = 1;
  this->pointsize = 1;
  this->shininess = 0.f;
  this->polygonoffsetstyle = 0;
  this->polygonoffsetunits = 0.f;
  this->polygonoffsetfactor = 0.f;
  this->linepattern = 0xffff;
  this->type = 0;
  this->indexer = 0;
  this->materialbinding = 0;
  this->pervertexcolor = false;
  this->transptexture = false;
  this->lightmodel = SoLazyElement::PHONG;
  this->vertexordering = SoLazyElement::CCW;
  this->culling = false;
  this->twoside = false;
  this->drawstyle = 0;
  this->shadowstyle = SoShadowStyleElement::CASTS_SHADOW_AND_SHADOWED; 
  this->texturematrices.clear();
  this->textures.clear();
  this->lights.clear();
  this->partialhighlight = 0;
  this->selectstyle = Material::Full;
  this->outline = false;
  this->shapetype = SoShapeHintsElement::UNKNOWN_SHAPE_TYPE;

  if (!state)
    return;

  this->overrideflags = getOverrideFlags(state);

  float t;
  t = SoLazyElement::getTransparency(state, 0);
  this->diffuse = SoLazyElement::getDiffuse(state, 0).getPackedValue(t);

  t = 0.0f;
  this->emissive = SoLazyElement::getEmissive(state).getPackedValue(t);

  this->ambient = SoLazyElement::getAmbient(state).getPackedValue(t);

  this->specular = SoLazyElement::getSpecular(state).getPackedValue(t);

  this->shininess = SoLazyElement::getShininess(state);

  this->lightmodel = SoLazyElement::getLightModel(state);

  SoShapeHintsElement::VertexOrdering ordering;
  SoShapeHintsElement::ShapeType shapetype;
  SoShapeHintsElement::FaceType facetype;
  SoShapeHintsElement::get(state, ordering, shapetype, facetype);
  this->shapetype = shapetype;
  this->vertexordering = ordering == SoShapeHintsElement::CLOCKWISE ?
                                          SoLazyElement::CW : SoLazyElement::CCW;
  // this->twoside = ordering != SoShapeHintsElement::UNKNOWN_ORDERING
  //                     && shapetype == SoShapeHintsElement::UNKNOWN_SHAPE_TYPE;
  this->culling = ordering != SoShapeHintsElement::UNKNOWN_ORDERING
                      && shapetype == SoShapeHintsElement::SOLID;
  this->twoside = SoLazyElement::getTwoSidedLighting(state);

  this->materialbinding = SoMaterialBindingElement::get(state);

  this->linepattern = SoLinePatternElement::get(state);

  this->linewidth = SoLineWidthElement::get(state);
  if (this->linewidth < 1.0f)
    this->linewidth = 1.0f;

  this->pointsize = SoPointSizeElement::get(state);
  if (this->pointsize < 1.0f)
    this->pointsize = 1.0f;

  SbBool on;
  SoPolygonOffsetElement::Style style;
  SoPolygonOffsetElement::get(state,
                                this->polygonoffsetfactor,
                                this->polygonoffsetunits,
                                style,
                                on);
    if (!on)
      this->polygonoffsetstyle = 0;
    else
      this->polygonoffsetstyle = style;

  this->drawstyle = SoDrawStyleElement::get(state);
}

template<class T>
static inline const T *
_checkMaterial(SoState *state, Material &m, Material::FlagBits flag, const T *element)
{
  if (!m.overrideflags.test(flag)) {
    auto curelement = constElement<T>(state);
    if (element != curelement) {
      // Must not preserve the changed element, as the data inside element may
      // change without changing the element pointer
      //
      // element = curelement;

      m.maskflags.set(flag);
      return curelement;
    }
  }
  return nullptr;
}

void
SoFCRenderCacheP::captureMaterial(SoState * state)
{
  Material & m = this->material;

  if (_checkMaterial(state, m, Material::FLAG_MATERIAL_BINDING, this->materialbindingelement))
    m.materialbinding = SoMaterialBindingElement::get(state);

  if (_checkMaterial(state, m, Material::FLAG_LINE_PATTERN, this->linepatternelement))
    m.linepattern = SoLinePatternElement::get(state);

  if (_checkMaterial(state, m, Material::FLAG_LINE_WIDTH, this->linewidthelement)) {
    m.linewidth = SoLineWidthElement::get(state);
    if (m.linewidth < 1.0f)
      m.linewidth = 1.0f;
  }

  if (_checkMaterial(state, m, Material::FLAG_POINT_SIZE, this->pointsizeelement)) {
    m.pointsize = SoPointSizeElement::get(state);
    if (m.pointsize < 1.0f)
      m.pointsize = 1.0f;
  }

  if (_checkMaterial(state, m, Material::FLAG_POLYGON_OFFSET, this->polygonoffsetelement)) {
    SbBool on;
    SoPolygonOffsetElement::Style style;
    SoPolygonOffsetElement::get(state,
                                m.polygonoffsetfactor,
                                m.polygonoffsetunits,
                                style,
                                on);
    if (!on)
      m.polygonoffsetstyle = 0;
    else
      m.polygonoffsetstyle = style;
  }

  if (_checkMaterial(state, m, Material::FLAG_DRAW_STYLE, this->drawstyleelement))
    m.drawstyle = SoDrawStyleElement::get(state);

  if (_checkMaterial(state, m, Material::FLAG_SHADOW_STYLE, this->shadowstyleelement))
    m.shadowstyle = SoShadowStyleElement::get(state);

  if (_checkMaterial(state, m, Material::FLAG_SHAPE_HINTS, this->shapehintselement)) {
    m.maskflags.set(Material::FLAG_CULLING);
    m.maskflags.set(Material::FLAG_VERTEXORDERING);
    m.maskflags.set(Material::FLAG_TWOSIDE);
    SoShapeHintsElement::VertexOrdering ordering;
    SoShapeHintsElement::ShapeType shapetype;
    SoShapeHintsElement::FaceType facetype;
    SoShapeHintsElement::get(state, ordering, shapetype, facetype);
    m.shapetype = shapetype;
    m.vertexordering = ordering == SoShapeHintsElement::CLOCKWISE ?
                                            SoLazyElement::CW : SoLazyElement::CCW;
    m.twoside = ordering != SoShapeHintsElement::UNKNOWN_ORDERING
                       && shapetype == SoShapeHintsElement::UNKNOWN_SHAPE_TYPE;
    m.culling = ordering != SoShapeHintsElement::UNKNOWN_ORDERING
                       && shapetype == SoShapeHintsElement::SOLID;
  }
  m.overrideflags |= getOverrideFlags(state);
}

void
SoFCRenderCache::setLightModel(SoState * state, const SoLightModel * lightmode)
{
  if (PRIVATE(this)->material.overrideflags.test(Material::FLAG_LIGHT_MODEL))
    return;

  if (lightmode->isOverride())
    PRIVATE(this)->material.overrideflags.test(Material::FLAG_LIGHT_MODEL);
  PRIVATE(this)->material.maskflags.set(Material::FLAG_LIGHT_MODEL);
  PRIVATE(this)->material.lightmodel = SoLightModelElement::get(state);
}

template<class N, class T>
static inline bool
testMaterial(SoFCRenderCache::Material &m, N node, T field, int flag, int mask)
{
  if (!(node->*field).isIgnored() && (node->*field).getNum() && !m.overrideflags.test(flag)) {
    if (node->isOverride())
      m.overrideflags.set(flag);
    m.maskflags.set(mask);
    return true;
  }
  return false;
}

void
SoFCRenderCache::setMaterial(SoState * state, const SoMaterial * material)
{
  (void)state;
  Material & m = PRIVATE(this)->material;
  float t = 0.f;
  if (testMaterial(m, material, &SoMaterial::diffuseColor, Material::FLAG_DIFFUSE, Material::FLAG_DIFFUSE)) {
    m.diffuse &= 0xff;
    m.diffuse |= material->diffuseColor[0].getPackedValue(t) & 0xffffff00;
    SbFCUniqueId id = material->diffuseColor.getNum() > 1 ? material->getNodeId() : 0;
    SoFCDiffuseElement::set(state, &id, NULL);
  }
  if (testMaterial(m, material, &SoMaterial::transparency, Material::FLAG_TRANSPARENCY, Material::FLAG_TRANSPARENCY)) {
    m.diffuse &= 0xffffff00;
    float alpha = SbClamp(1.0f-material->transparency[0], 0.f, 1.f);
    m.diffuse |= (uint8_t)(alpha * 255);
    SbFCUniqueId id = material->transparency.getNum() > 1 ? material->getNodeId() : 0;
    SoFCDiffuseElement::set(state, NULL, &id);
  }
  if (testMaterial(m, material, &SoMaterial::ambientColor, Material::FLAG_AMBIENT, Material::FLAG_AMBIENT))
    m.ambient = material->ambientColor[0].getPackedValue(t);
  if (testMaterial(m, material, &SoMaterial::emissiveColor, Material::FLAG_EMISSIVE, Material::FLAG_EMISSIVE))
    m.emissive = material->emissiveColor[0].getPackedValue(t);
  if (testMaterial(m, material, &SoMaterial::specularColor, Material::FLAG_SPECULAR, Material::FLAG_SPECULAR))
    m.specular = material->specularColor[0].getPackedValue(t);
  if (testMaterial(m, material, &SoMaterial::shininess, Material::FLAG_SHININESS, Material::FLAG_SHININESS))
    m.shininess = material->shininess[0];
}

void
SoFCRenderCache::setDepthBuffer(SoState * state, const SoDepthBuffer * node)
{
  (void)state;
  Material & m = PRIVATE(this)->material;
  if (!node->test.isIgnored()) {
    m.maskflags.set(Material::FLAG_DEPTH_TEST);
    m.depthtest = node->test.getValue();
  }
  if (!node->write.isIgnored()) {
    m.maskflags.set(Material::FLAG_DEPTH_WRITE);
    m.depthwrite = node->write.getValue();
  }
  if (!node->function.isIgnored()) {
    m.maskflags.test(Material::FLAG_DEPTH_FUNC);
    m.depthfunc = node->function.getValue();
  }
}

static inline bool
canSetMaterial(SoFCRenderCache::Material & res,
               const SoFCRenderCache::Material & parent,
               uint32_t flag, uint32_t mask)
{
  return (parent.overrideflags.test(flag)
      || (!res.maskflags.test(mask) && parent.maskflags.test(mask)));
}

template<class T>
static inline void
copyMaterial(SoFCRenderCache::Material &res,
             const SoFCRenderCache::Material &parent,
             T member,
             uint32_t flag, uint32_t mask)
{
  if (canSetMaterial(res, parent, flag, mask)) {
    res.*member = parent.*member;
    res.maskflags.set(mask);
  }
}

void
SoFCRenderCache::increaseRenderingOrder(int priority)
{
  if (priority)
    PRIVATE(this)->material.annotation += 1000 + priority;
  else
    ++PRIVATE(this)->material.annotation;
}

void
SoFCRenderCache::decreaseRenderingOrder(int priority)
{
  if (priority)
    PRIVATE(this)->material.annotation -= 1000 + priority;
  else
    --PRIVATE(this)->material.annotation;
}

SoFCRenderCache::Material
SoFCRenderCacheP::mergeMaterial(const SbMatrix &matrix,
                                bool &identity,
                                const Material &parent,
                                const Material &child)
{
  // merge material from bottom up

  Material res = child;

  if (parent.order > child.order)
    res.order = parent.order;

  if (parent.annotation > child.annotation)
    res.annotation = parent.annotation;

  if (res.selectstyle != Material::Box && res.selectstyle != Material::Unpickable)
    res.selectstyle = parent.selectstyle;

  if (parent.hiddenlinecolor)
    res.hiddenlinecolor = parent.hiddenlinecolor;

  res.outline |= parent.outline;

  auto mergeNodeInfo = [&](SoFCRenderCache::NodeInfoArray &thisarray,
                           const SoFCRenderCache::NodeInfoArray &other)
  {
    if (identity)
      thisarray.append(other);
    else if (other.getNum()) {
      for (const auto & info : other.getData()) {
        if (info.resetmatrix)
          thisarray.append(info);
        else {
          SoFCRenderCache::NodeInfo copy = info;
          if (copy.identity)
            copy.matrix = matrix;
          else
            copy.matrix.multLeft(matrix);
          copy.identity = false;
          thisarray.append(copy);
        }
      }
    }
  };
  if (child.resetclip)
    res.resetclip = true;
  else {
    res.clippers = parent.clippers;
    mergeNodeInfo(res.clippers, child.clippers);
  }

  res.autozoom = parent.autozoom;
  auto childzoom = child.autozoom;
  if (childzoom.getNum() && !identity) {
    const auto &info = childzoom.get(0);
    if (!info.resetmatrix) {
      auto copy = info;
      if (copy.identity)
        copy.matrix = matrix;
      else
        copy.matrix.multRight(matrix);
      copy.identity = false;
      childzoom.set(0, copy);
    }
    identity = true;
  }
  mergeNodeInfo(res.autozoom, childzoom);
  
  copyMaterial(res, parent, &Material::depthtest, 0, Material::FLAG_DEPTH_TEST);
  copyMaterial(res, parent, &Material::depthfunc, 0, Material::FLAG_DEPTH_FUNC);
  copyMaterial(res, parent, &Material::depthwrite, 0, Material::FLAG_DEPTH_WRITE);

  if (canSetMaterial(res, parent, Material::FLAG_DIFFUSE, Material::FLAG_DIFFUSE)) {
    res.diffuse &= 0xff;
    res.diffuse |= parent.diffuse & 0xffffff00;
    res.maskflags.set(Material::FLAG_DIFFUSE);
  }

  if (canSetMaterial(res, parent, Material::FLAG_TRANSPARENCY, Material::FLAG_TRANSPARENCY)) {
    res.diffuse &= 0xffffff00;
    res.diffuse |= parent.diffuse & 0xff;
    res.maskflags.set(Material::FLAG_TRANSPARENCY);
  }

  if (res.type == Material::Line || res.outline) {
    copyMaterial(res, parent, &Material::linewidth, Material::FLAG_LINE_WIDTH, Material::FLAG_LINE_WIDTH);
    copyMaterial(res, parent, &Material::linepattern, Material::FLAG_LINE_PATTERN, Material::FLAG_LINE_PATTERN);
    if (res.type == Material::Line)
      return res;
  }

  if (res.type == Material::Point) {
    copyMaterial(res, parent, &Material::pointsize, Material::FLAG_POINT_SIZE, Material::FLAG_POINT_SIZE);
    return res;
  }

  copyMaterial(res, parent, &Material::materialbinding, Material::FLAG_MATERIAL_BINDING, Material::FLAG_MATERIAL_BINDING);

  copyMaterial(res, parent, &Material::ambient, Material::FLAG_AMBIENT, Material::FLAG_AMBIENT);
  copyMaterial(res, parent, &Material::emissive, Material::FLAG_EMISSIVE, Material::FLAG_EMISSIVE);
  copyMaterial(res, parent, &Material::specular, Material::FLAG_SPECULAR, Material::FLAG_SPECULAR);
  copyMaterial(res, parent, &Material::shininess, Material::FLAG_SHININESS, Material::FLAG_SHININESS);
  copyMaterial(res, parent, &Material::drawstyle, Material::FLAG_DRAW_STYLE, Material::FLAG_DRAW_STYLE);
  copyMaterial(res, parent, &Material::lightmodel, Material::FLAG_LIGHT_MODEL, Material::FLAG_LIGHT_MODEL);
  copyMaterial(res, parent, &Material::shadowstyle, 0, Material::FLAG_SHADOW_STYLE);

  if (canSetMaterial(res, parent, Material::FLAG_POLYGON_OFFSET, Material::FLAG_POLYGON_OFFSET)) {
    res.polygonoffsetstyle = parent.polygonoffsetstyle;
    res.polygonoffsetfactor = parent.polygonoffsetfactor;
    res.polygonoffsetunits = parent.polygonoffsetunits;
    res.maskflags.set(Material::FLAG_POLYGON_OFFSET);
  }

  if (canSetMaterial(res, parent, Material::FLAG_SHAPE_HINTS, Material::FLAG_SHAPE_HINTS)) {
    res.culling = parent.culling;
    res.vertexordering = parent.vertexordering;
    res.twoside = parent.twoside;
    res.shapetype = parent.shapetype;
    res.maskflags.set(Material::FLAG_SHAPE_HINTS);
    res.maskflags.set(Material::FLAG_CULLING);
    res.maskflags.set(Material::FLAG_VERTEXORDERING);
    res.maskflags.set(Material::FLAG_TWOSIDE);
  }
  else {
    copyMaterial(res, parent, &Material::culling, 0, Material::FLAG_CULLING);
    copyMaterial(res, parent, &Material::vertexordering, 0, Material::FLAG_VERTEXORDERING);
    copyMaterial(res, parent, &Material::twoside, 0, Material::FLAG_TWOSIDE);
  }

  res.texturematrices.combine(parent.texturematrices);

  if (parent.texturematrices.getNum()) {
    for (auto & v : parent.texturematrices.getData()) {
      const SoFCRenderCache::TextureInfo * pinfo = res.textures.get(v.first);
      if (!pinfo) continue;
      SoFCRenderCache::TextureInfo info = * pinfo;
      if (info.identity)
        info.matrix = v.second.matrix;
      else
        info.matrix.multLeft(v.second.matrix);
      info.identity = false;
      res.textures.set(v.first, info);
    }
  }
  res.textures.add(parent.textures, false);

  res.lights = parent.lights;
  mergeNodeInfo(res.lights, child.lights);

  if (!res.transptexture && res.textures.getNum()) {
    for (auto & info : res.textures.getData()) {
      if (info.second.transparent) {
        res.transptexture = true;
        break;
      }
    }
  }

  return res;
}

SbFCUniqueId
SoFCRenderCache::getNodeId() const
{
  return PRIVATE(this)->nodeid;
}

SbBool
SoFCRenderCache::isValid(const SoState * state) const
{
  return inherited::isValid(state);
}

void
SoFCRenderCache::open(SoState *state, int selectstyle, bool initmaterial)
{
  SoCacheElement::set(state, this);

  if (PRIVATE(this)->selnode) {
    PRIVATE(this)->material.resetclip =
      PRIVATE(this)->selnode->resetClipPlane.getValue() ? true : false;
    PRIVATE(this)->selnode->setupColorOverride(state, true);
  }

  PRIVATE(this)->facecolor = 0;
  PRIVATE(this)->hiddenlinecolor = 0;
  PRIVATE(this)->facetransp = -1.f;
  PRIVATE(this)->material.init(initmaterial ? state : nullptr);
  PRIVATE(this)->material.selectstyle = selectstyle;

  SbBool outline = FALSE;
  if (initmaterial && SoFCDisplayModeElement::showHiddenLines(state, &outline)) {
    PRIVATE(this)->material.outline = outline;
    PRIVATE(this)->material.linewidth = SoLineWidthElement::get(state);

    PRIVATE(this)->facetransp = SoFCDisplayModeElement::getTransparency(state);
    const SbColor * color = SoFCDisplayModeElement::getFaceColor(state);
    if (color)
      PRIVATE(this)->facecolor = color->getPackedValue(0.f);
    color = SoFCDisplayModeElement::getLineColor(state);
    if (color)
      PRIVATE(this)->hiddenlinecolor = color->getPackedValue(0.f);
  }

  // Call SoState::getElement() here to force create SoOverrideElement at the
  // current stack level to prevent it being captured in cache, because we want
  // to decouple override settings from child caches.
  state->getElement(SoOverrideElement::getClassStackIndex());

  // Remember the current override flags. If it weren't for the above call to
  // getElement(), this element will be added to our cache dependency.
  //
  // Convert override flags to our own mask, and then reset the flags
  auto flags = getOverrideFlags(state);
  PRIVATE(this)->material.overrideflags = flags;

  if (flags.test(Material::FLAG_AMBIENT))
    SoOverrideElement::setAmbientColorOverride(state, NULL, FALSE);
  if (flags.test(Material::FLAG_DIFFUSE)) {
    SoOverrideElement::setDiffuseColorOverride(state, NULL, FALSE);
    if (!initmaterial) {
      SbFCUniqueId diffuseid = SoFCDiffuseElement::get(state, nullptr);
      if (diffuseid && diffuseid == getNodeId()) {
        uint32_t diffuse = SoLazyElement::getDiffuse(state, 0).getPackedValue(0.0f);
        PRIVATE(this)->material.diffuse &= 0xff;
        PRIVATE(this)->material.diffuse |= diffuse;
        PRIVATE(this)->material.maskflags.set(Material::FLAG_DIFFUSE);
      }
    }
  }
  if (flags.test(Material::FLAG_SPECULAR))
    SoOverrideElement::setSpecularColorOverride(state, NULL, FALSE);
  if (flags.test(Material::FLAG_EMISSIVE))
    SoOverrideElement::setEmissiveColorOverride(state, NULL, FALSE);
  if (flags.test(Material::FLAG_SHININESS))
    SoOverrideElement::setShininessOverride(state, NULL, FALSE);
  if (flags.test(Material::FLAG_TRANSPARENCY)) {
    SoOverrideElement::setTransparencyOverride(state, NULL, FALSE);
    if (!initmaterial) {
      SbFCUniqueId transpid = 0;
      SoFCDiffuseElement::get(state, &transpid);
      if (transpid && transpid == getNodeId()) {
        float t = SoLazyElement::getTransparency(state, 0);
        SbColor color(0,0,0);
        PRIVATE(this)->material.diffuse &= ~0xff;
        PRIVATE(this)->material.diffuse |= color.getPackedValue(t);
        PRIVATE(this)->material.maskflags.set(Material::FLAG_TRANSPARENCY);
      }
    }
  }
  if (flags.test(Material::FLAG_DRAW_STYLE))
    SoOverrideElement::setDrawStyleOverride(state, NULL, FALSE);
  if (flags.test(Material::FLAG_LINE_PATTERN))
    SoOverrideElement::setLinePatternOverride(state, NULL, FALSE);
  if (flags.test(Material::FLAG_LINE_WIDTH))
    SoOverrideElement::setLineWidthOverride(state, NULL, FALSE);
  if (flags.test(Material::FLAG_POINT_SIZE))
    SoOverrideElement::setPointSizeOverride(state, NULL, FALSE);
  if (flags.test(Material::FLAG_MATERIAL_BINDING))
    SoOverrideElement::setMaterialBindingOverride(state, NULL, FALSE);
  if (flags.test(Material::FLAG_POLYGON_OFFSET))
    SoOverrideElement::setPolygonOffsetOverride(state, NULL, FALSE);
  if (flags.test(Material::FLAG_SHAPE_HINTS))
    SoOverrideElement::setShapeHintsOverride(state, NULL, FALSE);
  if (flags.test(Material::FLAG_LIGHT_MODEL))
    SoOverrideElement::setLightModelOverride(state, NULL, FALSE);

  // Capture current relavant elements to detect change happen inside the
  // currnet caching group node. When capturing materials, we only capture
  // elements set within the current node.
  PRIVATE(this)->linepatternelement = constElement<SoLinePatternElement>(state);
  PRIVATE(this)->linewidthelement = constElement<SoLineWidthElement>(state);
  PRIVATE(this)->pointsizeelement = constElement<SoPointSizeElement>(state);
  PRIVATE(this)->polygonoffsetelement = constElement<SoPolygonOffsetElement>(state);
  PRIVATE(this)->drawstyleelement = constElement<SoDrawStyleElement>(state);
  PRIVATE(this)->materialbindingelement = constElement<SoMaterialBindingElement>(state);
  PRIVATE(this)->shadowstyleelement = constElement<SoShadowStyleElement>(state);
  PRIVATE(this)->shapehintselement = constElement<SoShapeHintsElement>(state);

  PRIVATE(this)->resetmatrix = false;
}

void
SoFCRenderCache::close(SoState *state)
{
  if (PRIVATE(this)->selnode)
    PRIVATE(this)->selnode->resetColorOverride(state);
  PRIVATE(this)->material.init();
}

class MyMultiTextureMatrixElement : public SoMultiTextureMatrixElement
{
public:
  int getNumUnits() const {
    return SoMultiTextureMatrixElement::getNumUnits();
  }

  const UnitData & getUnitData(const int unit) const {
    return SoMultiTextureMatrixElement::getUnitData(unit);
  }
};


void
SoFCRenderCacheP::addChildCache(SoState * state,
                                SoFCRenderCache * cache,
                                SoFCVertexCache * vcache,
                                bool opencache)
{
  auto elem = constElement<SoModelMatrixElement>(state);
  SbMatrix matrix = elem->getModelMatrix();
  bool identity = (matrix == matrixidentity);

  auto proxy = SoFCShapeProxyElement::get(state);
  int idxoffset = SoFCShapeIndexElement::peek(state);
  int idxcount = SoFCShapeCountElement::peek(state);

  if (opencache) {
    if (!identity) {
      // reset to identity matrix to decouple model transformation from child cache
      static_cast<SoModelMatrixElement*>(
          state->getElement(SoModelMatrixElement::getClassStackIndex()))->init(state);
    }

    // reset textrue matrix to identity
    auto elem = constElement<MyMultiTextureMatrixElement>(state);
    for (int i=0, n=elem->getNumUnits(); i<n; ++i) {
      if (elem->getUnitData(i).textureMatrix != matrixidentity)
        SoMultiTextureMatrixElement::set(state, NULL, i, matrixidentity);
    }
  }

  this->caches.emplace_back(matrix,
                            identity,
                            this->resetmatrix,
                            cache,
                            vcache,
                            proxy,
                            idxoffset,
                            idxoffset + idxcount);
  captureMaterial(state);
  Material & m = this->caches.back().material;
  m = this->material;
}

void
SoFCRenderCache::addChildCache(SoState *state, SoFCRenderCache * cache)
{
  PRIVATE(this)->addChildCache(state, cache, NULL, false);
  this->addCacheDependency(state, cache);
}

void
SoFCRenderCache::addChildCache(SoState *state, SoFCVertexCache * cache)
{
  PRIVATE(this)->addChildCache(state, NULL, cache, false);
  this->addCacheDependency(state, cache);
}

void
SoFCRenderCache::beginChildCaching(SoState *state, SoFCRenderCache * cache)
{
  PRIVATE(this)->addChildCache(state, cache, NULL, true);
}

void
SoFCRenderCache::beginChildCaching(SoState *state, SoFCVertexCache * cache)
{
  PRIVATE(this)->addChildCache(state, NULL, cache, true);
}

void
SoFCRenderCache::endChildCaching(SoState * state, SoFCVertexCache * vcache)
{
  if (!vcache->getNumVertices()) {
      if (PRIVATE(this)->caches.size()
          && PRIVATE(this)->caches.back().vcache == vcache)
    {
      PRIVATE(this)->caches.pop_back();
    }
  }
  this->addCacheDependency(state, vcache);
}

void
SoFCRenderCache::endChildCaching(SoState * state, SoFCRenderCache * cache)
{
  if (PRIVATE(cache)->caches.empty()) {
      if (PRIVATE(this)->caches.size()
          && PRIVATE(this)->caches.back().cache == cache)
    {
      PRIVATE(this)->caches.pop_back();
    }
  }
  this->addCacheDependency(state, cache);
}

SbBool
SoFCRenderCache::isEmpty() const
{
  return PRIVATE(this)->caches.empty();
}

class MyMultiTextureImageElement : public SoMultiTextureImageElement
{
public:
  SbBool hasTransparency(const int unit) const {
    return SoMultiTextureImageElement::hasTransparency(unit);
  }
};

void
SoFCRenderCache::addTexture(SoState * state, const SoTexture * texture)
{
  int unit = SoTextureUnitElement::get(state);

  TextureInfo info;
  info.texture = const_cast<SoTexture*>(texture);

  auto elem = constElement<MyMultiTextureImageElement>(state);
  info.transparent = elem->hasTransparency(unit);

  info.identity = true;
  auto melem = constElement<MyMultiTextureMatrixElement>(state);
  if (melem->getNumUnits() > unit) {
    const auto & data = melem->getUnitData(unit);
    if (data.textureMatrix != matrixidentity) {
      info.identity = false;
      info.matrix = data.textureMatrix;
    }
  }

  PRIVATE(this)->material.textures.set(unit, info);  
}

void
SoFCRenderCache::addTextureTransform(SoState * state, const SoNode * node)
{
  (void)node;
  int unit = SoTextureUnitElement::get(state);

  MatrixInfo info;
  bool identity = true;
  auto melem = constElement<MyMultiTextureMatrixElement>(state);
  if (melem->getNumUnits() > unit) {
    const auto & data = melem->getUnitData(unit);
    if (data.textureMatrix != matrixidentity) {
      identity = false;
      info.matrix = data.textureMatrix;
    }
  }
  if (identity)
    PRIVATE(this)->material.texturematrices.erase(unit);
  else
    PRIVATE(this)->material.texturematrices.set(unit, info);
}

void
SoFCRenderCache::addLight(SoState * state, const SoLight * light)
{
  (void)state;
  if (!light->on.getValue() || light->on.isIgnored()) return;

  NodeInfo info;
  info.node = const_cast<SoLight*>(light);
  info.resetmatrix = PRIVATE(this)->resetmatrix;

  auto elem = constElement<SoModelMatrixElement>(state);
  if (elem->getModelMatrix() == matrixidentity)
    info.identity = true;
  else {
    info.identity = false;
    info.matrix = elem->getModelMatrix();
  }

  PRIVATE(this)->material.lights.append(info);
}

void
SoFCRenderCache::addClipPlane(SoState * state, const SoClipPlane * node)
{
  (void)state;
  if (!node->on.getValue() || node->on.isIgnored()) return;

  NodeInfo info;
  info.node = const_cast<SoClipPlane*>(node);
  info.resetmatrix = PRIVATE(this)->resetmatrix;

  auto elem = constElement<SoModelMatrixElement>(state);
  if (elem->getModelMatrix() == matrixidentity)
    info.identity = true;
  else {
    info.identity = false;
    info.matrix = elem->getModelMatrix();
  }

  PRIVATE(this)->material.clippers.append(info);
}

void
SoFCRenderCache::addAutoZoom(SoState * state, const SoAutoZoomTranslation * node)
{
  (void)state;

  NodeInfo info;
  info.node = const_cast<SoAutoZoomTranslation*>(node);
  info.resetmatrix = PRIVATE(this)->resetmatrix;

  auto elem = constElement<SoModelMatrixElement>(state);
  if (elem->getModelMatrix() == matrixidentity)
    info.identity = true;
  else {
    info.identity = false;
    info.matrix = elem->getModelMatrix();
  }

  PRIVATE(this)->material.autozoom.append(info);
}

void
SoFCRenderCache::resetMatrix()
{
  PRIVATE(this)->resetmatrix = true;
  PRIVATE(this)->material.autozoom.clear();
}

inline void
SoFCRenderCacheP::finalizeMaterial(Material & material)
{
  if (material.materialbinding == SoMaterialBindingElement::OVERALL)
    material.pervertexcolor = false;

  if (material.type == Material::Triangle) {
    if (this->facecolor) {
      material.pervertexcolor = false;
      material.diffuse = this->facecolor | (material.diffuse & 0xff);
      material.overrideflags.set(Material::FLAG_DIFFUSE);
    }
    if (this->facetransp >= 0.f && this->facetransp <= 1.f) {
      uint8_t alpha = static_cast<uint8_t>(
          std::max(std::min(1.f-this->facetransp, 1.f), 0.f) * 255.f);
      material.diffuse = (material.diffuse & ~0xff) | alpha;
      material.overrideflags.set(Material::FLAG_TRANSPARENCY);
    }
  }

  if (this->hiddenlinecolor) {
    material.hiddenlinecolor = this->hiddenlinecolor;
    if (material.type != Material::Triangle) {
      material.pervertexcolor = false;
      material.diffuse = material.hiddenlinecolor;
    }
  }

  // if (material.pervertexcolor && material.maskflags.test(Material::FLAG_TRANSPARENCY))
  //   material.overrideflags.set(Material::FLAG_TRANSPARENCY);
}

const SoFCRenderCache::VertexCacheMap &
SoFCRenderCache::getVertexCaches(int depth)
{
  auto & vcachemap = PRIVATE(this)->vcachemap;
  if (!vcachemap.empty())
    return vcachemap;

  std::unordered_map<void *, CacheKeyPtr> keymap;
  CacheKeyPtr selfkey;

  static FC_COIN_THREAD_LOCAL SoSelectionElementAction selaction(
      SoSelectionElementAction::Retrieve, true);
  static FC_COIN_THREAD_LOCAL std::vector<std::pair<int, uint32_t> > selcolors;

  auto checkContext = [](Material &material,
                         const SoFCSelectionContextExPtr &ctx,
                         int idxstart,
                         int idxend,
                         VertexCachePtr &vcache)
  {
    // Check for secondary selection context for color override and partial rendering
    // return 0 if should skip this entry, 1 if procceed with same material, -1
    // if material is changed.
    static FC_COIN_THREAD_LOCAL std::vector<int> indices;
    indices.clear();
    if (!ctx)
      return 1;
    if (ctx->isSelectAll()) {
      if (ctx->colors.empty())
        return 1;
    } else {
      for (auto &v : ctx->selectionIndex) {
        int idx = v.first;
        if (idx >= idxstart && (idx < idxend || idxend < 0))
          indices.push_back(idx - idxstart);
      }
      if (indices.empty())
        return 1;
    }
    switch(material.type) {
      case Material::Triangle: {
        if (!ctx->isSelectAll()) {
          vcache = new SoFCVertexCache(*vcache);
          vcache->addTriangles(indices);
        } else {
          if (ctx->colors.empty())
            return 1;
          vcache = new SoFCVertexCache(*vcache);
          vcache->addTriangles();
        }
        if (ctx->colors.empty())
          return 1;
        if (ctx->colors.begin()->first < 0 && ctx->colors.size() == 1) {
          vcache->setFaceColors({}, 0);
          auto color = ctx->colors.begin()->second;
          color.a = 1.0 - color.a;
          uint32_t diffuse = color.getPackedValue();
          if (diffuse != material.diffuse || material.pervertexcolor) {
            material.diffuse = diffuse;
            material.pervertexcolor = false;
            material.materialbinding = SoMaterialBindingElement::OVERALL;
            return -1;
          }
          return 1;
        }
        selcolors.clear();
        for (auto &v : ctx->colors) {
          if (v.first < idxstart || v.first >= idxstart)
            continue;
          auto color = v.second;
          color.a = 1.0 - color.a;
          selcolors.emplace_back(v.first, color.getPackedValue());
        }
        vcache->setFaceColors(selcolors, reinterpret_cast<intptr_t>(ctx.get()));
        if ((material.pervertexcolor && !vcache->colorPerVertex())
            || (!material.pervertexcolor && vcache->colorPerVertex()))
        {
          material.pervertexcolor = !material.pervertexcolor;
          material.materialbinding = material.pervertexcolor ?
            SoMaterialBindingElement::PER_PART : SoMaterialBindingElement::OVERALL;
          return -1;
        }
        break;
      }
      case Material::Line: {
        if (ctx->isSelectAll())
          return 1;
        vcache = new SoFCVertexCache(*vcache);
        vcache->addLines(indices);
        break;
      }
      case Material::Point: {
        if (ctx->isSelectAll())
          return 1;
        vcache = new SoFCVertexCache(*vcache);
        vcache->addPoints(indices);
        break;
      }
      default:
        return 0;
    }
    return 1;
  };

  for (auto & entry : PRIVATE(this)->caches) {
    if (entry.vcache) {
      if (!selfkey) {
        selfkey.reset(new CacheKey);
        if (PRIVATE(this)->selnode)
          selfkey->push_back(PRIVATE(this)->selnode);
      }

      SoFCSelectionContextExPtr ctx;
      if (PRIVATE(this)->selnode && entry.proxy) {
        SoFCSelectionRoot::setActionStack(&selaction, selfkey.get());
        selaction.setRetrivedContext();
        selaction.apply(entry.proxy);
        ctx = selaction.getRetrievedContext();
      }

      entry.material.pervertexcolor = entry.vcache->colorPerVertex();

      auto vcache = entry.vcache;

      if (entry.vcache->getNumTriangleIndices()) {
        Material material = entry.material;
        material.type = Material::Triangle;
        if (!checkContext(material, ctx, entry.idxstart, entry.idxend, vcache))
          continue;
        if (depth == 0) {
          PRIVATE(this)->finalizeMaterial(material);
          if (!entry.vcache->getNormalArray())
            material.lightmodel = SoLazyElement::BASE_COLOR;
        }
        material.indexer = reinterpret_cast<intptr_t>(vcache.get());
        vcachemap[material].emplace_back(vcache,
                                         entry.matrix,
                                         entry.identity,
                                         entry.resetmatrix,
                                         selfkey,
                                         entry.proxy,
                                         entry.idxstart,
                                         entry.idxend);
      }
      if (entry.vcache->getNumLineIndices()) {
        Material material = entry.material;
        material.type = Material::Line;
        if (!checkContext(material, ctx, entry.idxstart, entry.idxend, vcache))
          continue;
        if (depth == 0) {
          PRIVATE(this)->finalizeMaterial(material);
          if (!entry.vcache->getNormalArray())
            material.lightmodel = SoLazyElement::BASE_COLOR;
        }
        material.indexer = reinterpret_cast<intptr_t>(vcache.get());
        vcachemap[material].emplace_back(vcache,
                                         entry.matrix,
                                         entry.identity,
                                         entry.resetmatrix,
                                         selfkey,
                                         entry.proxy,
                                         entry.idxstart,
                                         entry.idxend);
      }
      if (entry.vcache->getNumPointIndices()) {
        Material material = entry.material;
        material.type = Material::Point;
        if (!checkContext(material, ctx, entry.idxstart, entry.idxend, vcache))
          continue;
        if (depth == 0) {
          PRIVATE(this)->finalizeMaterial(material);
          if (!entry.vcache->getNormalArray())
            material.lightmodel = SoLazyElement::BASE_COLOR;
        }
        material.indexer = reinterpret_cast<intptr_t>(vcache.get());
        vcachemap[material].emplace_back(vcache,
                                         entry.matrix,
                                         entry.identity,
                                         entry.resetmatrix,
                                         selfkey,
                                         entry.proxy,
                                         entry.idxstart,
                                         entry.idxend);
      }
      continue;
    }
    auto it = vcachemap.end();
    const auto & childvcaches = entry.cache->getVertexCaches(depth+1); 
    for (const auto & child : childvcaches) {
      Material material = PRIVATE(this)->mergeMaterial(
            entry.matrix, entry.identity, entry.material, child.first);

      if (depth == 0)
        PRIVATE(this)->finalizeMaterial(material);

      VertexCacheMap::value_type value(material, {});
      bool entrypushed = false;
      for (const VertexCacheEntry & childentry : child.second) {
        std::vector<VertexCacheEntry> *ventries = nullptr;
        if (it != vcachemap.end() && entrypushed)
          ventries = &it->second;
        CacheKeyPtr & key = keymap[childentry.key.get()];
        if (!key) {
          key.reset(new CacheKey);
          if (PRIVATE(this)->selnode) {
            key->reserve(childentry.key->size()+1);
            key->push_back(PRIVATE(this)->selnode);
          }
          key->insert(key->end(), childentry.key->begin(), childentry.key->end());
        }

        auto vcache = childentry.cache;
        SoFCSelectionContextExPtr ctx;
        if (PRIVATE(this)->selnode && childentry.proxy) {
          SoFCSelectionRoot::setActionStack(&selaction, key.get());
          selaction.setRetrivedContext();
          selaction.apply(childentry.proxy);
          ctx = selaction.getRetrievedContext();
        }

        int res = checkContext(material, ctx, childentry.idxstart, childentry.idxend, vcache);
        if (!res)
          continue;

        if (res < 0) {
          ventries = &vcachemap[material];
          material = value.first; // revert back to original material
        } else if (!ventries) {
          it = vcachemap.insert(it, value);
          entrypushed = true;
          ventries = &it->second;
        }

        if (entry.identity || childentry.resetmatrix)
          ventries->emplace_back(vcache,
                                  childentry.matrix,
                                  childentry.identity,
                                  childentry.resetmatrix,
                                  key,
                                  childentry.proxy,
                                  childentry.idxstart,
                                  childentry.idxend);
        else if (childentry.identity)
          ventries->emplace_back(vcache,
                                  entry.matrix,
                                  entry.identity,
                                  false,
                                  key,
                                  childentry.proxy,
                                  childentry.idxstart,
                                  childentry.idxend);
        else {
          ventries->emplace_back(vcache,
                                  entry.matrix,
                                  false,
                                  false,
                                  key,
                                  childentry.proxy,
                                  childentry.idxstart,
                                  childentry.idxend);
          ventries->back().matrix.multLeft(childentry.matrix);
        }
      }
      if (it != vcachemap.end())
        ++it;
    }
  }

#ifdef FCCOIN_TRACE_ACHE_NAME
  std::size_t count = 0;
  for (auto &v : vcachemap)
    count += v.second.size();
  for (int i=0; i<depth; ++i)
    std::cerr << ' ';
  std::cerr << count << ": " << PRIVATE(this)->nodename.getString() << "\n";
#endif

  return vcachemap;
}

bool makeDistinctColor(SbColor &res, const SbColor &color, const SbColor &other) {
    double delta = ColorUtils::getColorDeltaE(
          ColorUtils::rgbColor(color[0], color[1], color[2]),
          ColorUtils::rgbColor(other[0], other[1], other[2]));
    if (delta > ViewParams::SelectionColorDifference())
      return false;

    float h,s,v;
    color.getHSVValue(h,s,v);
    h += 0.3f;
    if(h>1.0f)
        h = 1.0f-h;
    if(s<0.2f)
        s = 1.0f-s;
    res.setHSVValue(h,s,1.0f);
    return true;
}

bool makeDistinctColor(uint32_t &res, uint32_t color, uint32_t other) {
    SbColor r, c, o;
    float t;
    o.setPackedValue(other,t);
    c.setPackedValue(color,t);
    if(!makeDistinctColor(r,c,o))
        return false;
    res = r.getPackedValue(t);
    return true;
}

SoFCRenderCache::VertexCacheMap
SoFCRenderCache::buildHighlightCache(std::map<int, VertexCachePtr> &sharedcache,
                                     int order,
                                     const SoDetail * detail,
                                     uint32_t color,
                                     int flags)
{
  VertexCacheMap res;
  uint32_t alpha = color & 0xff;
  color &= 0xffffff00;

  bool checkindices = (flags & CheckIndices) ? true : false;
  bool wholeontop = (flags & WholeOnTop) ? true : false;
  bool preselect = (flags & PreselectHighlight) ? true : false;

  const SoPointDetail * pd = nullptr;
  const SoLineDetail * ld = nullptr;
  const SoFaceDetail * fd = nullptr;
  const SoFCDetail * d = nullptr;
  std::vector<int> faceindices;
  std::vector<int> edgeindices;
  std::vector<int> vertexindices;
  if (detail) {
    if (detail->isOfType(SoPointDetail::getClassTypeId()))
      pd = static_cast<const SoPointDetail*>(detail);
    else if (detail->isOfType(SoLineDetail::getClassTypeId()))
      ld = static_cast<const SoLineDetail*>(detail);
    else if (detail->isOfType(SoFaceDetail::getClassTypeId()))
      fd = static_cast<const SoFaceDetail*>(detail);
    else if (detail->isOfType(SoFCDetail::getClassTypeId())) {
      d = static_cast<const SoFCDetail*>(detail);
      for (int idx : d->getIndices(SoFCDetail::Face))
        faceindices.push_back(idx);
      for (int idx : d->getIndices(SoFCDetail::Edge))
        edgeindices.push_back(idx);
      for (int idx : d->getIndices(SoFCDetail::Vertex))
        vertexindices.push_back(idx);
    }

    // Some shape nodes (e.g. SoBrepFaceSet), support partial highlight on
    // whole object selection. 'checkindices' is used to indicate if we shall
    // check the internal highlight indices of those shapes that support it.
    // However, if we are not doing whole object selection (i.e. detail is not
    // null here), we should not check the indices.
    checkindices = false;
  }

  bool bboxinited = false;
  Material bboxmaterial;
  SbBox3f bbox;
  const VertexCacheEntry *detailentry = nullptr;
  std::vector<int> indices;

  for (auto & child : getVertexCaches()) {
    int selidx = -1;
    std::vector<int> *pindices = nullptr;
    if (detail) {
      switch(child.first.type) {
      case Material::Point:
        if (pd) 
          selidx = pd->getCoordinateIndex();
        else if (d)
          pindices = &vertexindices;
        break;
      case Material::Line:
        if (ld)
          selidx = ld->getLineIndex();
        else if (d)
          pindices = &edgeindices;
        break;
      default:
        if (fd)
          selidx = fd->getPartIndex();
        else if (d)
          pindices = &faceindices;
      }
    }

    if (!wholeontop && detail) {
      // We are doing partial highlight, 'wholeontop' indicates that we shall
      // bring the whole object to top with the original color. So if not
      // 'wholeontop' and there is some highlight detail, it means we are
      // highlight some sub-element of a shape node.

      if (child.first.selectstyle == Material::Unpickable) {
        // Either the parent is not selectable, or the shape is not
        // sub-element selectable (checked in the loop below).
        continue;
      }

      if (pindices) {
        if (pindices->empty())
          continue;
      }
      else if (selidx < 0)
        continue;
    }

    for (auto & ventry : child.second) {
      bool elementselectable = ventry.cache->isElementSelectable()
        && child.first.selectstyle != Material::Unpickable;

      if (!wholeontop && detail && !elementselectable)
          continue;

      indices.clear();

      int partidx = -1;
      if (selidx >= 0) {
        if (selidx < ventry.idxstart || (ventry.idxend >= 0 && selidx >= ventry.idxend))
          continue;
        partidx = selidx - ventry.idxstart;
      } else if (pindices) {
        for (int idx : *pindices) {
          if (idx >= ventry.idxstart && (idx < ventry.idxend || ventry.idxend < 0))
            indices.push_back(idx);
        }
        if (indices.empty())
          continue;
      }

      Material material = child.first;
      material.order = order;
      material.depthfunc = SoDepthBuffer::LEQUAL;

      if (color) {
        if (order <= 0 && detail)
            material.polygonoffsetstyle = 0;
        if (material.type != Material::Triangle)
          material.lightmodel = SoLazyElement::BASE_COLOR;
        if (material.lightmodel != SoLazyElement::BASE_COLOR && detail) {
          material.emissive = color | 0xff;
          makeDistinctColor(material.emissive, material.emissive, material.diffuse);
        } 
        uint32_t c = material.diffuse;
        material.diffuse = color | (material.diffuse & 0xff);
        makeDistinctColor(material.diffuse, material.diffuse, c);
        material.pervertexcolor = false;
      }

      if (color && (material.selectstyle == Material::Box
                    || (ViewParams::getShowSelectionBoundingBox()
                        && (!detail || !preselect))))
      {
        if (!bboxinited) {
          bboxinited = true;
          bboxmaterial = material;
        }
        const SbMatrix *matrix = ventry.identity ? nullptr : &ventry.matrix;
        switch(material.type) {
        case Material::Point:
          if (partidx >= 0) {
            detailentry = &ventry;
            ventry.cache->getPointsBoundingBox(nullptr, bbox, partidx);
          } else if (!detail)
            ventry.cache->getPointsBoundingBox(matrix, bbox);
          break;
        case Material::Line:
          if (partidx >= 0) {
            detailentry = &ventry;
            ventry.cache->getLinesBoundingBox(nullptr, bbox, partidx);
          } else if (!detail)
            ventry.cache->getLinesBoundingBox(matrix, bbox);
          break;
        case Material::Triangle:
          if (partidx >= 0) {
            detailentry = &ventry;
            ventry.cache->getTrianglesBoundingBox(nullptr, bbox, partidx);
          } else if (!detail)
            ventry.cache->getTrianglesBoundingBox(matrix, bbox);
          break;
        }
        continue;
      }

      VertexCacheEntry newentry = ventry;

      switch(material.type) {
      case Material::Point:
        if (!material.order)
          material.order = 1;
        if (!elementselectable) {
          if (!detail)
            res[material].push_back(ventry);
          else {
            Material m = child.first;
            m.order = material.order;
            m.depthfunc = material.depthfunc;
            m.partialhighlight = 1;
            res[m].push_back(ventry);
          }
        }
        else if (partidx >= 0) {
          newentry.partidx = partidx;
        }
        else if (indices.size() == 1) {
          newentry.partidx = *indices.begin();
        } else if (indices.size() > 1) {
          newentry.cache = new SoFCVertexCache(*newentry.cache);
          newentry.cache->addPoints(indices);
        }
        else if (checkindices) {
          auto cache = newentry.cache->highlightIndices(&newentry.partidx,
                                                        newentry.proxy,
                                                        newentry.idxstart,
                                                        newentry.idxend);
          if (newentry.partidx >= 0 || cache != newentry.cache) {
            newentry.cache = cache;
            if (wholeontop) {
              Material m = child.first;
              m.order = material.order;
              m.depthfunc = material.depthfunc;
              m.partialhighlight = 1;
              material.partialhighlight = -1;
              ++material.order;
              res[m].push_back(ventry);
            }
          }
        }
        break;

      case Material::Line:
        if (!elementselectable) {
          if (!detail)
            res[material].push_back(ventry);
          else {
            Material m = child.first;
            m.order = material.order;
            m.depthfunc = material.depthfunc;
            m.partialhighlight = 1;
            res[m].push_back(ventry);
          }
        }
        else if (partidx >= 0) {
          // Because of possible line strip, we do not use partidx for
          // partial rendering
          //
          // newentry.partidx = ld->getLineIndex();
          newentry.cache = new SoFCVertexCache(*newentry.cache);
          newentry.cache->addLines(std::vector<int>(1, partidx));
        }
        else if (indices.size()) {
          newentry.cache = new SoFCVertexCache(*newentry.cache);
          newentry.cache->addLines(indices);
        }
        else if (checkindices) {
          auto cache = newentry.cache->highlightIndices(&newentry.partidx,
                                                        newentry.proxy,
                                                        newentry.idxstart,
                                                        newentry.idxend);
          if (newentry.partidx >= 0 || cache != newentry.cache) {
            newentry.cache = cache;
            if (wholeontop) {
              Material m = child.first;
              m.order = material.order;
              m.depthfunc = material.depthfunc;
              m.partialhighlight = 1;
              material.partialhighlight = -1;
              ++material.order;
              res[m].push_back(ventry);
            }
          }
        }
        break;

      case Material::Triangle:
        if (alpha != 0xff) {
          uint32_t a = (child.first.diffuse & 0xff);
          if (child.first.pervertexcolor && ventry.cache->hasTransparency()) {
            if (a == 0xff)
              a = alpha;
          }
          if (a > alpha)
            a = alpha;
          material.overrideflags.set(Material::FLAG_TRANSPARENCY);
          material.diffuse = (material.diffuse & 0xffffff00) | a;
        }
        if (!elementselectable) {
          if (!detail)
            res[material].push_back(ventry);
          else {
            Material m = child.first;
            m.order = material.order;
            m.depthfunc = material.depthfunc;
            m.partialhighlight = 1;
            res[m].push_back(ventry);
          }
        }
        else if (partidx >= 0)
          newentry.partidx = partidx;
        else if (indices.size() == 1)
          newentry.partidx = *indices.begin() - newentry.idxstart;
        else if (indices.size() > 1) {
          newentry.cache = new SoFCVertexCache(*newentry.cache);
          newentry.cache->addTriangles(indices);
        }
        else if (checkindices) {
          auto cache = newentry.cache->highlightIndices(&newentry.partidx,
                                                        newentry.proxy,
                                                        newentry.idxstart,
                                                        newentry.idxend);
          if (newentry.partidx >= 0 || cache != newentry.cache) {
            newentry.cache = cache;
            if (wholeontop) {
              Material m = child.first;
              m.order = material.order;
              m.depthfunc = material.depthfunc;
              m.partialhighlight = 1;
              material.partialhighlight = -1;
              ++material.order;
              if (alpha != 0xff) {
                m.overrideflags.set(Material::FLAG_TRANSPARENCY);
                m.diffuse = (m.diffuse & 0xffffff00) | (material.diffuse & 0xff);
              }
              res[m].push_back(ventry);
            }
          }
        }
        if (color && newentry.partidx >= 0) {
          uint32_t col = newentry.cache->getFaceColor(newentry.partidx);
          if ((col & 0xff) != 0xff && alpha == 0xff) {
            uint32_t a = static_cast<uint32_t>(ViewParams::getSelectionTransparency() * 255);
            material.diffuse = (material.diffuse & ~0xff) | std::max(a, col&0xff);
          }
          makeDistinctColor(material.diffuse, material.diffuse, col);
          if (material.lightmodel != SoLazyElement::BASE_COLOR)
            material.emissive = material.diffuse | 0xff;
        }
        break;
      }
      res[material].push_back(newentry);
    }
  }

  if (!bbox.isEmpty()) {
    SbVec3f unitsize(1.f, 1.f, 1.f);
    auto size = bbox.getSize();
    int cacheid = 0;
    if (size[0] < 1e-6f) {
      unitsize[0] = 0.f;
      size[0] = 1.f;
      if (size[1] < 1e-6f) {
        unitsize[1] = 0.f;
        size[1] = 1.f;
        if (size[2] < 1e-6f) {
          unitsize[2] = 0.f;
          size[2] = 1.f;
          cacheid = 1;
        } else
          cacheid = 2;
      } else if (size[2] < 1e-6f) {
        unitsize[2] = 0.f;
        size[2] = 1.f;
        cacheid = 3;
      }
    } else if (size[1] < 1e-6f) {
      unitsize[1] = 0.f;
      size[1] = 1.f;
      if (size[2] < 1e-6f) {
        unitsize[2] = 0.f;
        size[2] = 1.f;
        cacheid = 4;
      } else
        cacheid = 5;
    } else if (size[2] < 1e-6f) {
      unitsize[2] = 0.f;
      size[2] = 1.f;
      cacheid = 6;
    } else
      cacheid = 7;
    auto &cache = sharedcache[cacheid];
    if (!cache) 
      cache = new SoFCVertexCache(SbBox3f(SbVec3f(0.f, 0.f, 0.f), unitsize));

    SbMatrix matrix;
    matrix.setTransform(bbox.getMin(), SbRotation(), size, SbRotation());
    if (detailentry && !detailentry->identity)
      matrix.multRight(detailentry->matrix);

    bboxmaterial.type = cache->getNumLineIndices() ? Material::Line : Material::Point;
    bboxmaterial.diffuse = color | 0xff;
    bboxmaterial.linewidth = ViewParams::instance()->getSelectionBBoxLineWidth();
    bboxmaterial.pointsize = bboxmaterial.linewidth * 2;
    bboxmaterial.depthclamp = true;

    res[bboxmaterial].emplace_back(cache, matrix, false, false, CacheKeyPtr());
  }

  return res;
}

// vim: noai:ts=2:sw=2
