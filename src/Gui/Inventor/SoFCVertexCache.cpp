/**************************************************************************\
 * Copyright (c) 2020 Zheng, Lei (realthunder) <realthunder.dev@gmail.com>
 * Copyright (c) Kongsberg Oil & Gas Technologies AS
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 * 
 * Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 * 
 * Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
\**************************************************************************/

#include "PreCompiled.h"

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cfloat>

#include <Inventor/C/glue/gl.h>
#include <Inventor/C/tidbits.h>
#include <Inventor/SbVec3f.h>
#include <Inventor/SoPrimitiveVertex.h>
#include <Inventor/details/SoFaceDetail.h>
#include <Inventor/details/SoLineDetail.h>
#include <Inventor/details/SoPointDetail.h>
#include <Inventor/elements/SoBumpMapCoordinateElement.h>
#include <Inventor/elements/SoViewVolumeElement.h>
#include <Inventor/elements/SoModelMatrixElement.h>
#include <Inventor/elements/SoGLCacheContextElement.h>
#include <Inventor/elements/SoCacheElement.h>
#include <Inventor/elements/SoLazyElement.h>
#include <Inventor/elements/SoCacheHintElement.h>
#include <Inventor/elements/SoMultiTextureCoordinateElement.h>
#include <Inventor/elements/SoMultiTextureEnabledElement.h>
#include <Inventor/elements/SoCoordinateElement.h>
#include <Inventor/elements/SoNormalElement.h>
#include <Inventor/elements/SoGLVBOElement.h>
#include <Inventor/sensors/SoNodeSensor.h>
#include <Inventor/nodes/SoNode.h>
#include <Inventor/fields/SoMFInt32.h>
#include <Inventor/fields/SoSFNode.h>
#include <Inventor/actions/SoGLRenderAction.h>
#include <Inventor/actions/SoCallbackAction.h>
#include <Inventor/lists/SbList.h>
#include <Inventor/lists/SbPList.h>
#include <Inventor/system/gl.h>
#include <Inventor/SbPlane.h>
#include <Inventor/SbBox3f.h>
#include <Inventor/errors/SoDebugError.h>
#include <Inventor/misc/SoGLDriverDatabase.h>
#include <Inventor/threads/SbMutex.h>

#include "SoFCVertexCache.h"
#include "SoFCDiffuseElement.h"
#include "SoFCVBO.h"
#include "SoFCVertexArrayIndexer.h"
#include "COWData.h"

using namespace Gui;

// *************************************************************************
typedef SoFCVertexAttribute<SbVec2f> Vec2Array;
typedef SoFCVertexAttribute<SbVec3f> Vec3Array;
typedef SoFCVertexAttribute<SbVec4f> Vec4Array;
typedef SoFCVertexAttribute<uint8_t> ByteArray;
typedef CoinPtr<SoFCVertexCache> VertexCachePtr;

#define PRIVATE(obj) ((obj)->pimpl)
#define PUBLIC(obj) ((obj)->master)

static SbName * PartIndexField;
static SbName * SeamIndicesField;
static SbName * HighlightIndicesField;
static SbName * ElementSelectableField;
static SbName * OnTopPatternField;
static SbName * ShapeInstanceField;

class SoFCVertexCacheP {
public:
  enum Arrays {
    NORMAL = SoFCVertexCache::NORMAL,
    TEXCOORD = SoFCVertexCache::TEXCOORD,
    COLOR = SoFCVertexCache::COLOR,
    SORTED_ARRAY = SoFCVertexCache::SORTED_ARRAY,
    FULL_SORTED_ARRAY = SoFCVertexCache::FULL_SORTED_ARRAY,
    NON_SORTED_ARRAY = SoFCVertexCache::NON_SORTED_ARRAY,
    ALL = SoFCVertexCache::ALL,
  };

  static void initClass()
  {
    PartIndexField = new SbName("partIndex");
    SeamIndicesField = new SbName("seamIndices");
    HighlightIndicesField = new SbName("highlightIndices");
    ElementSelectableField = new SbName("elementSelectable");
    OnTopPatternField = new SbName("onTopPattern");
    ShapeInstanceField = new SbName("shapeInstance");
  }

  static void cleanup()
  {
    delete PartIndexField;
    PartIndexField = nullptr;
    delete SeamIndicesField;
    SeamIndicesField = nullptr;
    delete HighlightIndicesField;
    HighlightIndicesField = nullptr;
    delete ElementSelectableField;
    ElementSelectableField = nullptr;
    delete OnTopPatternField;
    OnTopPatternField = nullptr;
    delete ShapeInstanceField;
    ShapeInstanceField = nullptr;
  }

  struct Vertex {
  public:
    SbVec3f vertex;
    SbVec3f normal;
    SbVec4f texcoord0;
    SbVec2f bumpcoord;
    uint32_t color;
    int texcoordidx;
    mutable std::size_t _hash = 0;

    bool operator==(const Vertex & v) const {
      return
        (this->vertex == v.vertex) &&
        (this->normal == v.normal) &&
        (this->texcoord0 == v.texcoord0) &&
        (this->bumpcoord == v.bumpcoord) &&
        (this->texcoordidx == v.texcoordidx) &&
        (this->color == v.color);
    }
  };

  struct VertexHasher {
    // copied from boost::hash_combine. 
    template <class S, class T>
    static inline void hash_combine(S& seed, const T& v)
    {
      std::hash<T> hasher;
      seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    }

    unsigned long operator()(const Vertex &v) const {
      if (!v._hash) {
        hash_combine(v._hash, v.color);
        hash_combine(v._hash, v.vertex[0]);
        hash_combine(v._hash, v.vertex[1]);
        hash_combine(v._hash, v.vertex[2]);
        hash_combine(v._hash, v.normal[0]);
        hash_combine(v._hash, v.normal[1]);
        hash_combine(v._hash, v.normal[2]);
        hash_combine(v._hash, v.texcoord0[0]);
        hash_combine(v._hash, v.texcoord0[1]);
        hash_combine(v._hash, v.texcoord0[2]);
        hash_combine(v._hash, v.texcoord0[3]);
        hash_combine(v._hash, v.bumpcoord[0]);
        hash_combine(v._hash, v.bumpcoord[1]);
        hash_combine(v._hash, v.texcoordidx);
      }
      return v._hash;
    }
  };

  struct TempStorage {
    std::unordered_map<Vertex, int32_t, VertexHasher> vhash;

    const SbVec2f * bumpcoords;
    int numbumpcoords;

    const uint32_t * packedptr;
    const SbColor * diffuseptr;
    const float * transpptr;

    int numdiffuse;
    int numtransp;

    const SoMultiTextureCoordinateElement * multielem;
    SoState * state = nullptr;

    const int32_t *partindices = nullptr;
    int partcount = 0;

    CoinPtr<SoFCVertexArrayIndexer::IndexArray> prevtriangleindices;
    CoinPtr<SoFCVertexArrayIndexer::IndexArray> prevlineindices;
    CoinPtr<SoFCVertexArrayIndexer::IndexArray> prevpointindices;
  };

  SoFCVertexCacheP(SoFCVertexCache * m,
                   SoFCVertexCache * prev,
                   SbFCUniqueId id)
    : master(m),
      prevcache(prev),
      prevattached(false),
      nodeid(id),
      diffuseid(0),
      transpid(0),
      triangleindexer(nullptr),
      lineindexer(nullptr),
      noseamindexer(nullptr),
      pointindexer(nullptr)
  {
    this->tmp = new TempStorage;
    if (prev) {
      if (PRIVATE(prev)->triangleindexer)
        this->tmp->prevtriangleindices = PRIVATE(prev)->triangleindexer->getIndexArray();
      if (PRIVATE(prev)->lineindexer)
        this->tmp->prevlineindices = PRIVATE(prev)->lineindexer->getIndexArray();
      if (PRIVATE(prev)->pointindexer)
        this->tmp->prevpointindices = PRIVATE(prev)->pointindexer->getIndexArray();
    }
  }

  ~SoFCVertexCacheP()
  {
    delete tmp;
    delete triangleindexer;
    delete lineindexer;
    delete pointindexer;
    delete noseamindexer;
  }

  uint32_t getColor(const SoFCVertexArrayIndexer * indexer, int part) const;

  void getBoundingBox(const SbMatrix * matrix,
                      SbBox3f & bbox,
                      const SoFCVertexArrayIndexer *indexer,
                      int part) const;


  template<class FacesT, class FindT> void
  addTriangles(const FacesT & faces, FindT && find)
  {
    assert(!this->triangleindexer && this->prevcache);

    this->prevattached = true;

    auto prevcache = this->prevcache.get();
    assert(PRIVATE(prevcache)->triangleindexer);

    auto indexer = PRIVATE(prevcache)->triangleindexer;
    this->triangleindexer = new SoFCVertexArrayIndexer(*indexer, faces, master->getNumVertices());
    this->tmp->prevtriangleindices.reset();

    if (!indexer->getNumParts() || this->triangleindexer->getNumParts() == indexer->getNumParts())
      return;

    int typesize = this->triangleindexer->useShorts() ? 2 : 4;
    const int * parts = this->triangleindexer->getPartOffsets();

    // Check if we need to adjust opaque indices in case of partial rendering
    if (this->transppartindices.size()) {
      int j = 0;
      int idx = 0;
      for (int i : this->transppartindices) {
        for (;j<i; ++j) {
          if (!find(faces, j))
            continue;
          int prev = j == 0 ? 0 : parts[j-1];
          this->opaquepartarray.compareAndSet(idx, prev * typesize);
          this->opaquepartcounts.compareAndSet(idx, parts[j] - prev);
          ++idx;
        }
      }
      this->opaquepartarray.resize(idx);
      this->opaquepartcounts.resize(idx);
    }
  }

  template<class IndicesT> void
  addLines(const IndicesT & lineindices)
  {
    assert(!this->lineindexer && this->prevcache);

    this->prevattached = true;

    auto prevcache = this->prevcache.get();
    assert(PRIVATE(prevcache)->lineindexer);

    auto indexer = PRIVATE(prevcache)->lineindexer;
    this->lineindexer = new SoFCVertexArrayIndexer(* indexer, lineindices, master->getNumVertices());
    this->tmp->prevlineindices.reset();
  }

  template<class IndicesT> void
  addPoints(const IndicesT & pointindices)
  {
    assert(!this->pointindexer && this->prevcache);

    this->prevattached = true;

    auto prevcache = this->prevcache.get();
    assert(PRIVATE(prevcache)->pointindexer);

    auto indexer = PRIVATE(prevcache)->pointindexer;
    this->pointindexer = new SoFCVertexArrayIndexer(*indexer, pointindices, master->getNumVertices());
    this->tmp->prevpointindices.reset();
  }

  SbBool depthSortTriangles(SoState * state, bool fullsort, const SbPlane *sortplane);

  void addVertex(const Vertex & v);
  void initColor(int n);

  void open(SoState *);
  void close(SoState *);
  void checkTransparency();

  void render(SoState *state,
              SoFCVertexArrayIndexer *indexer,
              int arrays,
              int part,
              int unit);

  void render(SoState *state,
              SoFCVertexArrayIndexer * indexer,
              const std::vector<int> &partindices,
              int arrays,
              int unit);

  void render(SoState * state,
              SoFCVertexArrayIndexer *indexer,
              const int arrays,
              const intptr_t * offsets = NULL,
              const int32_t * counts = NULL,
              int32_t drawcount = 0);

  void renderImmediate(const cc_glglue * glue,
                       const GLint * indices,
                       const int numindices,
                       const SbBool color, const SbBool normal,
                       const SbBool texture, const SbBool * enabled,
                       const int lastenabled);

  void enableArrays(const cc_glglue * glue,
                    const SbBool color, const SbBool normal,
                    const SbBool texture, const SbBool * enabled,
                    const int lastenabled);

  void disableArrays(const cc_glglue * glue,
                     const SbBool color, const SbBool normal,
                     const SbBool texture, const SbBool * enabled,
                     const int lastenabled);

  void enableVBOs(SoState * state,
                  const cc_glglue * glue,
                  const uint32_t contextid,
                  const SbBool color, const SbBool normal,
                  const SbBool texture, const SbBool * enabled,
                  const int lastenabled);

  void disableVBOs(const cc_glglue * glue,
                   const SbBool color, const SbBool normal,
                   const SbBool texture, const SbBool * enabled,
                   const int lastenabled);

  unsigned long countVBOSize(const cc_glglue * glue,
                             const uint32_t contextid,
                             const SbBool color, const SbBool normal,
                             const SbBool texture, const SbBool * enabled,
                             const int lastenabled);

  void prepare();

  SoFCVertexCache *master;
  CoinPtr<SoFCVertexCache> prevcache;
  bool prevattached;

  int idxoffset = 0;
  SoNode *node = nullptr;
  SoNode *proxy = nullptr;
  SbFCUniqueId nodeid;
  SbFCUniqueId diffuseid;
  SbFCUniqueId transpid;

  CoinPtr<Vec3Array> vertexarray;
  CoinPtr<Vec3Array> normalarray;
  CoinPtr<Vec4Array> texcoord0array;
  CoinPtr<Vec2Array> bumpcoordarray;
  CoinPtr<ByteArray> colorarray;
  CoinPtr<ByteArray> prevcolorarray;
  std::vector<CoinPtr<Vec4Array> > multitexarray;

  TempStorage* tmp = nullptr;

  int numtranspparts;

  bool hastransp;
  int colorpervertex;
  uint32_t firstcolor;

  int drawstate = 0;

  const SbBool * texenabled = nullptr;
  int lasttexenabled;
  int lastenabled;
  SbPlane prevsortplane;

  struct SortEntry {
    float depth;
    int index;
    int index2;
    int index3;
    SortEntry(int i)
      :index(i)
    {}
    SortEntry(int i, int j, int k)
      :index(i), index2(j), index3(k)
    {}
  };
  std::vector<SortEntry> deptharray;

  std::vector<intptr_t> sortedpartarray;
  std::vector<int32_t> sortedpartcounts;

  COWVector<std::vector<int> > transppartindices;
  COWVector<std::vector<intptr_t> > opaquepartarray;
  COWVector<std::vector<int32_t> > opaquepartcounts;

  COWVector<std::vector<SbVec3f> > partcenters;
  COWVector<std::vector<int32_t> > nonflatparts;
  COWVector<std::vector<int32_t> > seamindices;

  SoFCVertexArrayIndexer * triangleindexer;
  SoFCVertexArrayIndexer * lineindexer;
  SoFCVertexArrayIndexer * noseamindexer;
  SoFCVertexArrayIndexer * pointindexer;

  bool elementselectable;
  bool ontoppattern;

  SbBox3f boundbox;
};

// *************************************************************************

SoFCVertexCache::SoFCVertexCache(SoState * state, SoNode * node, SoFCVertexCache * prev)
  : SoCache(state),
    pimpl(new SoFCVertexCacheP(this, prev, node->getNodeId()))
{
  auto tmp = PRIVATE(this)->tmp;
  PRIVATE(this)->node = node;
  tmp->state = state;
  PRIVATE(this)->elementselectable = false;
  PRIVATE(this)->ontoppattern = false;

  const SoField * field = node->getField(*PartIndexField);
  if (field && field->isOfType(SoMFInt32::getClassTypeId())) {
    const SoMFInt32 * indices = static_cast<const SoMFInt32*>(field);
    tmp->partindices = indices->getValues(0);
    tmp->partcount = indices->getNum();
  }

  field = node->getField(*SeamIndicesField);
  if (field && field->isOfType(SoMFInt32::getClassTypeId())) {
    const SoMFInt32 * indices = static_cast<const SoMFInt32*>(field);
    if (indices->getNum()) {
      PRIVATE(this)->seamindices.resize(indices->getNum());
      auto seamindices = PRIVATE(this)->seamindices.at(0);
      memcpy(seamindices, indices->getValues(0), indices->getNum()*4);
      std::sort(seamindices, seamindices + indices->getNum());
    }
  }

  field = node->getField(*ElementSelectableField);
  if (field && field->isOfType(SoSFBool::getClassTypeId()))
    PRIVATE(this)->elementselectable = static_cast<const SoSFBool*>(field)->getValue();

  field = node->getField(*OnTopPatternField);
  if (field && field->isOfType(SoSFBool::getClassTypeId()))
    PRIVATE(this)->ontoppattern = static_cast<const SoSFBool*>(field)->getValue();
}

SoFCVertexCache::SoFCVertexCache(SoFCVertexCache & prev)
  : SoCache(nullptr),
    pimpl(new SoFCVertexCacheP(this, &prev, PRIVATE(&prev)->nodeid))
{
  auto pprev = &prev;

  PRIVATE(this)->node = PRIVATE(pprev)->node;
  PRIVATE(this)->diffuseid = PRIVATE(pprev)->diffuseid;
  PRIVATE(this)->transpid = PRIVATE(pprev)->transpid;

  PRIVATE(this)->vertexarray = PRIVATE(pprev)->vertexarray;
  PRIVATE(this)->normalarray = PRIVATE(pprev)->normalarray;
  PRIVATE(this)->texcoord0array = PRIVATE(pprev)->texcoord0array;
  PRIVATE(this)->bumpcoordarray = PRIVATE(pprev)->bumpcoordarray;
  PRIVATE(this)->colorarray = PRIVATE(pprev)->colorarray;
  PRIVATE(this)->prevcolorarray.reset();
  PRIVATE(this)->multitexarray = PRIVATE(pprev)->multitexarray;

  PRIVATE(this)->numtranspparts = PRIVATE(pprev)->numtranspparts;
  PRIVATE(this)->hastransp = PRIVATE(pprev)->hastransp;
  PRIVATE(this)->firstcolor = PRIVATE(pprev)->firstcolor;
  PRIVATE(this)->colorpervertex = PRIVATE(pprev)->colorpervertex;

  PRIVATE(this)->partcenters = PRIVATE(pprev)->partcenters;
  PRIVATE(this)->nonflatparts = PRIVATE(pprev)->nonflatparts;

  PRIVATE(this)->transppartindices = PRIVATE(pprev)->transppartindices;
  PRIVATE(this)->opaquepartarray = PRIVATE(pprev)->opaquepartarray;
  PRIVATE(this)->opaquepartcounts = PRIVATE(pprev)->opaquepartcounts;

  PRIVATE(this)->seamindices = PRIVATE(pprev)->seamindices;

  PRIVATE(this)->elementselectable = PRIVATE(pprev)->elementselectable;
  PRIVATE(this)->ontoppattern = PRIVATE(pprev)->ontoppattern;
}

SoFCVertexCache::SoFCVertexCache(const SbBox3f &bbox)
  : SoCache(nullptr),
    pimpl(new SoFCVertexCacheP(this, nullptr, 0xD2A25905))
{
  PRIVATE(this)->vertexarray = new Vec3Array(PRIVATE(this)->nodeid, nullptr);
  float minx, miny, minz, maxx, maxy, maxz;
  bbox.getBounds(minx, miny, minz, maxx, maxy, maxz);
  for (int i = 0; i < 8; i++) {
    PRIVATE(this)->vertexarray->append(
        SbVec3f((i&1) ? minx : maxx,
                (i&2) ? miny : maxy,
                (i&4) ? minz : maxz));

  }
  auto verts = PRIVATE(this)->vertexarray->getArrayPtr();
  static const int indices[][2] = {
    {0,1},
    {1,3},
    {3,2},
    {2,0},
    {4,5},
    {5,7},
    {7,6},
    {6,4},
    {0,4},
    {1,5},
    {2,6},
    {3,7},
  };
  for (int i=0; i<12; ++i) {
    int a = indices[i][0];
    int b = indices[i][1];
    if ((verts[a]-verts[b]).sqrLength() < 1e-12)
      continue;
    if (!PRIVATE(this)->lineindexer)
      PRIVATE(this)->lineindexer = new SoFCVertexArrayIndexer(PRIVATE(this)->nodeid, nullptr);
    PRIVATE(this)->lineindexer->addLine(a,b,0);
  }
  PRIVATE(this)->vertexarray = PRIVATE(this)->vertexarray->attach();
  if (PRIVATE(this)->lineindexer) {
    PRIVATE(this)->lineindexer->close();
  } else {
    PRIVATE(this)->pointindexer = new SoFCVertexArrayIndexer(PRIVATE(this)->nodeid, nullptr);
    PRIVATE(this)->pointindexer->addPoint(0);
    PRIVATE(this)->pointindexer->close();
  }
}

SoFCVertexCache::~SoFCVertexCache()
{
  delete pimpl;
}

void
SoFCVertexCache::setFaceColors(const std::vector<std::pair<int, uint32_t> > &colors,
                              SbFCUniqueId id)
{
  if (!PRIVATE(this)->triangleindexer)
    return;

  int numparts = PRIVATE(this)->triangleindexer->getNumParts();
  if (!numparts)
    return;

  if (colors.empty()) {
    PRIVATE(this)->colorarray.reset();
    PRIVATE(this)->colorpervertex = 0;
    return;
  }

  auto prev = PRIVATE(this)->prevcache.get();
  if (!PRIVATE(this)->colorarray || !prev || !PRIVATE(prev)->colorarray) {
    PRIVATE(this)->colorarray = new ByteArray(id, nullptr);
    PRIVATE(this)->prevcolorarray.reset();
    PRIVATE(this)->initColor(PRIVATE(this)->vertexarray->getLength()*4);
  } else {
    PRIVATE(this)->colorarray = new ByteArray(id, PRIVATE(prev)->colorarray);
    PRIVATE(this)->colorarray->copyFromProxy();
  }

  PRIVATE(this)->colorpervertex = 1;
  auto indices = PRIVATE(this)->triangleindexer->getIndices();
  const int *offsets = PRIVATE(this)->triangleindexer->getPartOffsets();
  PRIVATE(this)->hastransp = false;
  for (auto &v : colors) {
    int start, end;
    if (v.first < 0) {
      start = 0;
      end = PRIVATE(this)->vertexarray->getLength();
    } else {
      start = v.first ? offsets[v.first-1] : 0;
      end = offsets[v.first];
    }
    uint8_t r,g,b,a;
    r = (v.second >> 24) & 0xff;
    g = (v.second >> 16) & 0xff;
    b = (v.second >> 8) & 0xff;
    a = (v.second) & 0xff;
    if (a != 0xff)
      PRIVATE(this)->hastransp = true;
    auto &array = *PRIVATE(this)->colorarray;
    for (int i=start; i<end; ++i) {
      int idx = indices[i] * 4;
      array[idx] = r;
      array[idx+1] = g;
      array[idx+2] = b;
      array[idx+3] = a;
    }
  }
  PRIVATE(this)->colorarray = PRIVATE(this)->colorarray->attach();
  PRIVATE(this)->checkTransparency();
}

void
SoFCVertexCache::resetNode()
{
  PRIVATE(this)->node = nullptr;
}

bool
SoFCVertexCache::isElementSelectable() const
{
  return PRIVATE(this)->elementselectable;
}

bool
SoFCVertexCache::allowOnTopPattern() const
{
  return PRIVATE(this)->ontoppattern;
}

void
SoFCVertexCache::open(SoState * state)
{
  assert(!PRIVATE(this)->prevattached);

  SoCacheElement::set(state, this);

  // TODO: It would be ideal if we can get access to the captured elements
  // indside ourself (i.e. SoCache), but it is stored in private class at the
  // moment. Because not all shapes uses SoCoordinateElement/SoNormalElement,
  // for example, primitive shapes like SoCube. In these cases, the correct
  // way is to key on shape's nodeid.

  SbFCUniqueId id;
  SoFCVertexCache *prev = PRIVATE(this)->prevcache;

  auto delem = static_cast<const SoFCDiffuseElement*>(
      state->getConstElement(SoFCDiffuseElement::getClassStackIndex()));
  if (delem->getDiffuseId() || delem->getTransparencyId()) {
    // calling get() below to capture the element in cache
    PRIVATE(this)->diffuseid = SoFCDiffuseElement::get(state, &PRIVATE(this)->transpid);
  }

  const SoCoordinateElement *celem = SoCoordinateElement::getInstance(state);
  id = celem->getNum() ? celem->getNodeId() : (getNodeId() + 0xc9edfc95);
  PRIVATE(this)->vertexarray = new Vec3Array(id, prev ? PRIVATE(prev)->vertexarray : NULL);

  const SoNormalElement *nelem = SoNormalElement::getInstance(state);
  id = nelem->getNum() ? nelem->getNodeId() : (getNodeId() + 0xc3e4eff4d);
  PRIVATE(this)->normalarray =  new Vec3Array(id, prev ? PRIVATE(prev)->normalarray : NULL);

  const SoBumpMapCoordinateElement * belem =
    SoBumpMapCoordinateElement::getInstance(state);

  PRIVATE(this)->tmp->numbumpcoords = belem->getNum();
  PRIVATE(this)->tmp->bumpcoords = belem->getArrayPtr();
  if (PRIVATE(this)->tmp->numbumpcoords) {
    PRIVATE(this)->bumpcoordarray =
      new Vec2Array(belem->getNodeId(), prev ? PRIVATE(prev)->bumpcoordarray : NULL);
  }

  SoLazyElement * lelem = SoLazyElement::getInstance(state);

  PRIVATE(this)->tmp->numdiffuse = lelem->getNumDiffuse();
  PRIVATE(this)->numtranspparts = 0;
  PRIVATE(this)->tmp->numtransp = lelem->getNumTransparencies();
  if (lelem->isPacked()) {
    PRIVATE(this)->tmp->packedptr = lelem->getPackedPointer();
    PRIVATE(this)->tmp->diffuseptr = NULL;
    PRIVATE(this)->tmp->transpptr = NULL;
  }
  else {
    PRIVATE(this)->tmp->packedptr = NULL;
    PRIVATE(this)->tmp->diffuseptr = lelem->getDiffusePointer();
    PRIVATE(this)->tmp->transpptr = lelem->getTransparencyPointer();
  }

  // set up variables to test if we need to supply color per vertex
  if (PRIVATE(this)->tmp->numdiffuse <= 1 && PRIVATE(this)->tmp->numtransp <= 1)
    PRIVATE(this)->colorpervertex = 0;
  else {
    PRIVATE(this)->colorpervertex = -1;
    if (prev)
      PRIVATE(this)->prevcolorarray = PRIVATE(prev)->colorarray;
  }

  // just store diffuse color with index 0
  if (PRIVATE(this)->tmp->packedptr) {
    PRIVATE(this)->firstcolor = PRIVATE(this)->tmp->packedptr[0];
  }
  else {
    SbColor tmpc = PRIVATE(this)->tmp->diffuseptr[0];
    float tmpt = PRIVATE(this)->tmp->transpptr[0];
    PRIVATE(this)->firstcolor = tmpc.getPackedValue(tmpt);
  }
  PRIVATE(this)->hastransp = (PRIVATE(this)->firstcolor & 0xff)!=0xff;

  // set up for multi texturing
  PRIVATE(this)->lastenabled = -1;
  SoMultiTextureEnabledElement::getEnabledUnits(state, PRIVATE(this)->lastenabled);
  PRIVATE(this)->tmp->multielem = NULL;

}

SbFCUniqueId
SoFCVertexCache::getNodeId() const
{
  return PRIVATE(this)->nodeid;
}

SoNode *
SoFCVertexCache::getNode() const
{
  return PRIVATE(this)->node;
}

SbBool 
SoFCVertexCache::isValid(const SoState * state) const
{
  if (PRIVATE(this)->prevattached && PRIVATE(this)->prevcache)
    return PRIVATE(this)->prevcache->isValid(state);
  return inherited::isValid(state);
}

void 
SoFCVertexCache::close(SoState * state)
{
  (void)state;
  PRIVATE(this)->close(state);
}

void
SoFCVertexCacheP::close(SoState * state)
{
  if (this->vertexarray)
    this->vertexarray = this->vertexarray->attach();
  if (this->normalarray) {
    if (!this->triangleindexer) {
      const SoNormalElement *nelem = SoNormalElement::getInstance(state);
      if (nelem->getNum() == 0)
        this->normalarray.reset();
    }
    if (this->normalarray)
      this->normalarray = this->normalarray->attach();
  }
  if (this->texcoord0array)
    this->texcoord0array = this->texcoord0array->attach();
  if (this->bumpcoordarray)
    this->bumpcoordarray = this->bumpcoordarray->attach();
  if (this->colorarray)
    this->colorarray = this->colorarray->attach();
  for (auto & entry : this->multitexarray) {
    if (entry)
      entry = entry->attach();
  }
  if (this->triangleindexer)
    this->triangleindexer->close(this->tmp->partindices, this->tmp->partcount);
  if (this->lineindexer)
    this->lineindexer->close();
  if (this->pointindexer)
    this->pointindexer->close();

  if (this->triangleindexer && this->triangleindexer->getNumParts()) {
    const int *parts = this->triangleindexer->getPartOffsets();
    const GLint *indices = this->triangleindexer->getIndices();
    const SbVec3f *vertices = this->vertexarray->getArrayPtr();
    int prev = 0;
    int numparts = this->triangleindexer->getNumParts();
    this->partcenters.reserve(numparts);
    for (int i=0; i<numparts; ++i) {
      SbBox3f bbox;
      int n = parts[i]-prev;
      for (int k=0; k<n; ++k)
        bbox.extendBy(vertices[indices[k+prev]]);
      float dx,dy,dz;
      bbox.getSize(dx, dy, dz);
      if (dx > 1e-6f && dy > 1e-6f && dz > 1e-6)
        this->nonflatparts.push_back(i);
      this->partcenters.push_back(bbox.getCenter());
      prev = parts[i];
    }
  }

  this->checkTransparency();
  delete this->tmp;
  this->tmp = nullptr;
}

void
SoFCVertexCacheP::checkTransparency()
{
  if (!this->triangleindexer) return;
  int numparts = this->triangleindexer->getNumParts();
  if (!numparts) return;

  int transpidx = 0;
  const int *parts = this->triangleindexer->getPartOffsets();

  if (this->colorpervertex <= 0)
    this->numtranspparts = this->hastransp ? numparts : 0;
  else if (this->hastransp) {
    const GLint *indices = this->triangleindexer->getIndices();
    int prev = 0;
    for (int i=0; i<numparts; ++i) {
      bool transp = false;
      int n = parts[i]-prev;
      for (int k=0; k<n; ++k) {
        if ((*this->colorarray)[indices[k+prev]*4 + 3] != 0xff) {
          transp = true;
          break;
        }
      }
      if (transp)
        this->transppartindices.compareAndSet(transpidx++, i);
      prev = parts[i];
    }
    this->numtranspparts = (int)this->transppartindices.size();
  }
  this->transppartindices.resize(transpidx);

  int opaqueidx = 0;
  if (this->numtranspparts && this->numtranspparts != numparts) {
    int typesize = this->triangleindexer->useShorts() ? 2 : 4;
    int prev = 0;
    for (int i : this->transppartindices) {
      if (i != prev) {
        this->opaquepartarray.compareAndSet(opaqueidx, (prev ? parts[prev-1] : 0) * typesize);
        this->opaquepartcounts.compareAndSet(opaqueidx, parts[i-1] - (prev ? parts[prev-1] : 0));
        ++opaqueidx;
      }
      prev = i+1;
    }
    if (prev < numparts) {
      this->opaquepartarray.compareAndSet(opaqueidx, (prev ? parts[prev-1] : 0) * typesize);
      this->opaquepartcounts.compareAndSet(opaqueidx, parts[numparts-1] - (prev ? parts[prev-1] : 0));
      ++opaqueidx;
    }
  }
  this->opaquepartarray.resize(opaqueidx);
  this->opaquepartcounts.resize(opaqueidx);
}

int
SoFCVertexCache::getNumNonFlatParts() const
{
  return (int)PRIVATE(this)->nonflatparts.size();
}

const int *
SoFCVertexCache::getNonFlatParts() const
{
  return getNumNonFlatParts() ? &PRIVATE(this)->nonflatparts[0] : nullptr;
}

int
SoFCVertexCache::getNumFaceParts() const
{
  if (PRIVATE(this)->triangleindexer)
    return PRIVATE(this)->triangleindexer->getNumParts();
  return 0;
}

void
SoFCVertexCacheP::render(SoState *state,
                         SoFCVertexArrayIndexer *indexer,
                         int arrays,
                         int part,
                         int unit)
{
  if (!indexer)
    return;

  intptr_t offset;
  int32_t count;

  if (!indexer->getNumParts()) {
    count = unit;
    if ((part+1) * unit > indexer->getNumIndices())
      return;
    offset = part * unit * (indexer->useShorts() ? 2 : 4);
  }
  else if (part >= indexer->getNumParts())
    return;
  else {
    const int * parts = indexer->getPartOffsets();
    if (part == 0) {
      count = parts[0];
      offset = 0;
    }
    else {
      count = parts[part] - parts[part-1];
      offset = parts[part-1];
      offset *= indexer->useShorts() ? 2 : 4;
    }
  }
  render(state, indexer, arrays, &offset, &count, 1);
}

void
SoFCVertexCache::beginDraw(SoState *state, int arrays, int target)
{
  if (PRIVATE(this)->drawstate)
    return;

  PRIVATE(this)->lasttexenabled = -1;
  PRIVATE(this)->texenabled = nullptr;
  const SbBool normal = (arrays & NORMAL) != 0;
  const SbBool texture = (arrays & TEXCOORD) != 0;
  const SbBool color = this->colorPerVertex() && ((arrays & COLOR) != 0);
  if (texture) {
    PRIVATE(this)->texenabled =
      SoMultiTextureEnabledElement::getEnabledUnits(state, PRIVATE(this)->lasttexenabled);
    if (PRIVATE(this)->lasttexenabled > PRIVATE(this)->lastenabled)
      PRIVATE(this)->lasttexenabled = PRIVATE(this)->lastenabled;
  }

  const uint32_t contextid = SoGLCacheContextElement::get(state);
  const cc_glglue * glue = cc_glglue_instance(static_cast<int>(contextid));
  int vnum = PRIVATE(this)->vertexarray->getLength();

  if (SoFCVBO::shouldCreateVBO(state, contextid, vnum)) {
    PRIVATE(this)->enableVBOs(state, glue, contextid, color, normal, texture,
        PRIVATE(this)->texenabled, PRIVATE(this)->lasttexenabled);
    PRIVATE(this)->drawstate = 1;
  } else if (SoFCVBO::shouldRenderAsVertexArrays(state, contextid, vnum)) {
    PRIVATE(this)->enableArrays(glue, color, normal, texture,
        PRIVATE(this)->texenabled, PRIVATE(this)->lasttexenabled);
    PRIVATE(this)->drawstate = 2;
  } else {
    // fall back to immediate mode rendering
    PRIVATE(this)->drawstate = 3;
    glBegin(target);
  }
  return;
}

void
SoFCVertexCache::endDraw(SoState *state, int arrays)
{
  if (!PRIVATE(this)->drawstate)
    return;

  const SbBool normal = (arrays & NORMAL) != 0;
  const SbBool texture = (arrays & TEXCOORD) != 0;
  const SbBool color = this->colorPerVertex() && ((arrays & COLOR) != 0);

  const uint32_t contextid = SoGLCacheContextElement::get(state);
  const cc_glglue * glue = cc_glglue_instance(static_cast<int>(contextid));

  if (PRIVATE(this)->drawstate == 1)
    PRIVATE(this)->disableVBOs(
        glue, color, normal, texture, PRIVATE(this)->texenabled, PRIVATE(this)->lasttexenabled);
  else if (PRIVATE(this)->drawstate == 2)
    PRIVATE(this)->disableArrays(
        glue, color, normal, texture, PRIVATE(this)->texenabled, PRIVATE(this)->lasttexenabled);
  else
    glEnd();

  PRIVATE(this)->drawstate = 0;
}

void
SoFCVertexCacheP::render(SoState * state,
                         SoFCVertexArrayIndexer *indexer,
                         const int arrays,
                         const intptr_t * offsets,
                         const int32_t * counts,
                         int32_t drawcount)
{
  if (!indexer || !indexer->getNumIndices()) return;
  if (!this->vertexarray || !this->vertexarray->getLength()) return;

  const SbBool normal = (arrays & NORMAL) != 0;
  const SbBool texture = (arrays & TEXCOORD) != 0;
  const SbBool color = PUBLIC(this)->colorPerVertex() && ((arrays & COLOR) != 0);
  const uint32_t contextid = SoGLCacheContextElement::get(state);
  const cc_glglue * glue = cc_glglue_instance(static_cast<int>(contextid));

  int vnum = this->vertexarray->getLength();

  bool resetdrawstate = this->drawstate == 0;
  if (resetdrawstate) {
    this->texenabled = nullptr;
    this->lasttexenabled = -1;
    if (texture) {
      this->texenabled = SoMultiTextureEnabledElement::getEnabledUnits(state, this->lasttexenabled);
      if (this->lasttexenabled > this->lastenabled)
        this->lasttexenabled = this->lastenabled;
    }
    if (SoFCVBO::shouldCreateVBO(state, contextid, vnum)) {
      enableVBOs(state, glue, contextid, color, normal, texture,
          this->texenabled, this->lasttexenabled);
      this->drawstate = 1;
    } else if (SoFCVBO::shouldRenderAsVertexArrays(state, contextid, vnum)) {
      enableArrays(glue, color, normal, texture, this->texenabled, this->lasttexenabled);
      this->drawstate = 2;
    } else {
      // fall back to immediate mode rendering
      this->drawstate = 3;
      glBegin(indexer->getTarget());
    }
  }

  if (this->drawstate == 1) {
    indexer->render(state, glue, TRUE, contextid, offsets, counts, drawcount);
  } else if (this->drawstate == 2) {
    if (!drawcount)
      indexer->render(state, glue, FALSE, contextid);
    else {
      int typeshift = indexer->useShorts() ? 1 : 2;
      for (int i=0; i<drawcount; ++i) {
        int32_t count = counts[i];
        intptr_t offset = offsets[i] >> typeshift;
        offset = (intptr_t)(indexer->getIndices() + offset);
        indexer->render(state, glue, FALSE, contextid, &offset, &count, 1);
      }
    }
  }
  else {
    if (!drawcount) {
      this->renderImmediate(glue,
                            indexer->getIndices(),
                            indexer->getNumIndices(),
                            color, normal, texture, this->texenabled, this->lasttexenabled);
    }
    else {
      int typeshift = indexer->useShorts() ? 1 : 2;
      for (int i=0; i<drawcount; ++i) {
        int count = counts[i];
        intptr_t offset = offsets[i] >> typeshift;
        this->renderImmediate(glue,
                              indexer->getIndices() + offset, count,
                              color, normal, texture, this->texenabled, this->lasttexenabled);
      }
    }
  }

  if (resetdrawstate) {
    if (this->drawstate == 1)
      this->disableVBOs(glue, color, normal, texture, this->texenabled, this->lasttexenabled);
    else if (this->drawstate == 2)
      this->disableArrays(glue, color, normal, texture, this->texenabled, this->lasttexenabled);
    else
      glEnd();
    this->drawstate = 0;
  }
}

SbBool
SoFCVertexCache::hasOpaqueParts() const
{
  if (!PRIVATE(this)->triangleindexer) return TRUE;
  if (!PRIVATE(this)->numtranspparts) return !PRIVATE(this)->hastransp;
  return PRIVATE(this)->numtranspparts < PRIVATE(this)->triangleindexer->getNumParts();
}

SbBool
SoFCVertexCache::hasTransparency() const
{
  return PRIVATE(this)->hastransp;
}

void
SoFCVertexCache::renderTriangles(SoState * state, const int arrays, int part, const SbPlane *viewplane)
{
  if (part >= 0) {
    PRIVATE(this)->render(state, PRIVATE(this)->triangleindexer, arrays, part, 3);
    return;
  }

  int drawcount = 0;
  const intptr_t * offsets = NULL;
  const int32_t * counts = NULL;

  if (arrays & (SORTED_ARRAY | FULL_SORTED_ARRAY)) {
    if (PRIVATE(this)->depthSortTriangles(state, (arrays & FULL_SORTED_ARRAY) ? true : false, viewplane)) {
      offsets = &PRIVATE(this)->sortedpartarray[0];
      counts = &PRIVATE(this)->sortedpartcounts[0];
      drawcount = (int)PRIVATE(this)->sortedpartarray.size();
    }
  }
  else if (!(arrays & NON_SORTED_ARRAY) && PRIVATE(this)->opaquepartarray.size()) {
    offsets = &PRIVATE(this)->opaquepartarray[0];
    counts = &PRIVATE(this)->opaquepartcounts[0];
    drawcount = (int)PRIVATE(this)->opaquepartarray.size();
  }

  PRIVATE(this)->render(state, PRIVATE(this)->triangleindexer, arrays, offsets, counts, drawcount);
}

void
SoFCVertexCache::renderTriangles(SoState * state,
                                 const std::vector<int> &partindices,
                                 const int arrays)
{
  PRIVATE(this)->render(state,
                        PRIVATE(this)->triangleindexer,
                        partindices,
                        arrays,
                        3);
}

void
SoFCVertexCache::renderLines(SoState * state,
                             const std::vector<int> &partindices,
                             const int arrays)
{
  PRIVATE(this)->render(state,
                        PRIVATE(this)->lineindexer,
                        partindices,
                        arrays,
                        2);
}

void
SoFCVertexCache::renderPoints(SoState * state,
                              const std::vector<int> &partindices,
                              const int arrays)
{
  PRIVATE(this)->render(state,
                        PRIVATE(this)->pointindexer,
                        partindices,
                        arrays,
                        2);
}

void
SoFCVertexCacheP::render(SoState *state,
                         SoFCVertexArrayIndexer * indexer,
                         const std::vector<int> &partindices,
                         int arrays,
                         int unit)
{
  if (partindices.empty()) {
    render(state, indexer, arrays);
    return;
  }
  if (partindices.size() == 1) {
    render(state, indexer, arrays, partindices[0], unit);
    return;
  }

  if (!indexer)
    return;
  int numparts = indexer->getNumParts();
  if (!numparts)
    return;

  static FC_COIN_THREAD_LOCAL std::vector<intptr_t> offsets;
  static FC_COIN_THREAD_LOCAL std::vector<int32_t> counts;

  offsets.clear();
  counts.clear();

  const int * parts = indexer->getPartOffsets();
  for (int part : partindices) {
    if (part < 0 || part >= numparts)
      continue;
    int prev = part ? parts[part-1] : 0;
    offsets.push_back(prev * (indexer->useShorts() ? 2 : 4));
    counts.push_back(parts[part] - prev);
  }

  render(state, indexer, arrays, &offsets[0], &counts[0], (int)offsets.size());
}

void
SoFCVertexCache::renderLines(SoState * state, const int arrays, int part, bool noseam)
{
  if (part >= 0) {
    PRIVATE(this)->render(state, PRIVATE(this)->lineindexer, arrays, part, 2);
    return;
  }
  if (noseam && PRIVATE(this)->seamindices.size() && PRIVATE(this)->lineindexer) {
    if (!PRIVATE(this)->noseamindexer) {
      PRIVATE(this)->noseamindexer = new SoFCVertexArrayIndexer(
          *PRIVATE(this)->lineindexer, PRIVATE(this)->seamindices.getData(), -1, true); 
    }
    PRIVATE(this)->render(state, PRIVATE(this)->noseamindexer, arrays);
  } else
    PRIVATE(this)->render(state, PRIVATE(this)->lineindexer, arrays);
}

void
SoFCVertexCache::renderPoints(SoState * state, const int arrays, int part)
{
  if (part >= 0) {
    PRIVATE(this)->render(state, PRIVATE(this)->pointindexer, arrays, part, 1);
    return;
  }
  PRIVATE(this)->render(state, PRIVATE(this)->pointindexer, arrays);
}

class MyMultiTextureCoordinateElement : public SoMultiTextureCoordinateElement
{
public:
  SbFCUniqueId getNodeId(int unit) const { return getUnitData(unit).nodeid; }
};

inline void
SoFCVertexCacheP::prepare()
{
  assert(!this->prevattached);

  if (!this->tmp->multielem && this->lastenabled >= 0) {
    this->tmp->multielem = SoMultiTextureCoordinateElement::getInstance(this->tmp->state);
    SbFCUniqueId id = static_cast<const MyMultiTextureCoordinateElement*>(this->tmp->multielem)->getNodeId(0);
    this->texcoord0array =
      new Vec4Array(id, this->prevcache ? PRIVATE(this->prevcache)->texcoord0array : NULL);

    if (this->lastenabled > 0) {
      this->multitexarray.resize(this->lastenabled+1);
      for (int i = 1; i < this->lastenabled+1; ++i) {
        id = static_cast<const MyMultiTextureCoordinateElement*>(this->tmp->multielem)->getNodeId(i);
        Vec4Array *prev = NULL;
        if (this->prevcache && PRIVATE(this->prevcache)->lastenabled >= i)
          prev = PRIVATE(this->prevcache)->multitexarray[i];
        this->multitexarray[i] = new Vec4Array(id, prev);
      }
    }
  }

  this->prevcache.reset();
}

void
SoFCVertexCache::addTriangles(const std::map<int, int> & faces)
{
  PRIVATE(this)->addTriangles(faces,
    [](const std::map<int, int> & faces, int idx) {
      return faces.count(idx)!=0;
    });
}

void
SoFCVertexCache::addTriangles(const std::set<int> & faces)
{
  PRIVATE(this)->addTriangles(faces,
    [](const std::set<int> & faces, int idx) {
      return faces.count(idx)!=0;
    });
}

void
SoFCVertexCache::addTriangles(const std::vector<int> & faces)
{
  PRIVATE(this)->addTriangles(faces,
    [](const std::vector<int> & faces, int idx) {
      return std::find(faces.begin(), faces.end(), idx) != faces.end();
    });
}

void
SoFCVertexCache::addTriangle(const SoPrimitiveVertex * v0,
                             const SoPrimitiveVertex * v1,
                             const SoPrimitiveVertex * v2,
                             const int * pointdetailidx,
                             const int * texindices)
{
  PRIVATE(this)->prepare();

  const SoPrimitiveVertex *vp[3] = { v0, v1, v2 };

  int32_t triangleindices[3];

  const SoFaceDetail *fd = nullptr;

  for (int i = 0; i < 3; i++) {
    SoFCVertexCacheP::Vertex v;
    v.vertex = vp[i]->getPoint();
    v.normal = vp[i]->getNormal();
    const SbVec4f & tmp = vp[i]->getTextureCoords();
    v.bumpcoord = SbVec2f(tmp[0], tmp[1]);
    v.texcoord0 = tmp;
    v.texcoordidx = -1;

    if (PRIVATE(this)->colorpervertex == 0) {
      v.color = PRIVATE(this)->firstcolor;
    }
    else {
      int midx = vp[i]->getMaterialIndex();
      if (midx < 0)
        v.color = vp[i]->getPackedColor();
      else if (PRIVATE(this)->tmp->packedptr)
        v.color = PRIVATE(this)->tmp->packedptr[SbClamp(midx, 0, PRIVATE(this)->tmp->numdiffuse-1)];
      else {
        SbColor tmpc = PRIVATE(this)->tmp->diffuseptr[SbClamp(midx,0,PRIVATE(this)->tmp->numdiffuse-1)];
        float tmpt = PRIVATE(this)->tmp->transpptr[SbClamp(midx,0,PRIVATE(this)->tmp->numtransp-1)];
        v.color = tmpc.getPackedValue(tmpt);
      }
      if (v.color != PRIVATE(this)->firstcolor) PRIVATE(this)->colorpervertex = 1;
      PRIVATE(this)->hastransp = (PRIVATE(this)->hastransp || (v.color&0xff) != 0xff);
    }

    const SoDetail * d = vp[i]->getDetail();

    if (texindices)
      v.texcoordidx = texindices[i];
    else if (d && d->isOfType(SoFaceDetail::getClassTypeId()) && pointdetailidx) {
      fd = static_cast<const SoFaceDetail *>(d);
      assert(pointdetailidx[i] < fd->getNumPoints());
      const SoPointDetail * pd = static_cast<const SoPointDetail *>(
        fd->getPoint(pointdetailidx[i])
       );
      v.texcoordidx = pd->getTextureCoordIndex();
    }
    if (v.texcoordidx >= 0) {
      if (PRIVATE(this)->tmp->numbumpcoords) {
        v.bumpcoord = PRIVATE(this)->tmp->bumpcoords[SbClamp(
            v.texcoordidx, 0, PRIVATE(this)->tmp->numbumpcoords-1)];
      }
    }

    auto res = PRIVATE(this)->tmp->vhash.insert(
        std::make_pair(v, PRIVATE(this)->vertexarray->getLength()));
    if (res.second) {
      PRIVATE(this)->addVertex(v);
      // update texture coordinates for unit 1-n
      for (int j = 1; j <= PRIVATE(this)->lastenabled; j++) {
        if (v.texcoordidx >= 0 &&
            (PRIVATE(this)->tmp->multielem->getType(j) == SoMultiTextureCoordinateElement::EXPLICIT)) {
          PRIVATE(this)->multitexarray[j]->append(PRIVATE(this)->tmp->multielem->get4(j, v.texcoordidx));
        }
        else if (PRIVATE(this)->tmp->multielem->getType(j) == SoMultiTextureCoordinateElement::FUNCTION) {
          PRIVATE(this)->multitexarray[j]->append(PRIVATE(this)->tmp->multielem->get(j, v.vertex, v.normal));
        }
        else {
          PRIVATE(this)->multitexarray[j]->append(v.texcoord0);
        }
      }
    }
    triangleindices[i] = res.first->second;
  }

  if (!PRIVATE(this)->triangleindexer) {
    PRIVATE(this)->triangleindexer =
      new SoFCVertexArrayIndexer(PRIVATE(this)->nodeid, PRIVATE(this)->tmp->prevtriangleindices);
    PRIVATE(this)->tmp->prevtriangleindices = NULL;
  }
  PRIVATE(this)->triangleindexer->addTriangle(triangleindices[0],
                                              triangleindices[1],
                                              triangleindices[2]);
}

void
SoFCVertexCache::addLines(const std::map<int, int> & lineindices)
{
  PRIVATE(this)->addLines(lineindices);
}

void
SoFCVertexCache::addLines(const std::set<int> & lineindices)
{
  PRIVATE(this)->addLines(lineindices);
}

void
SoFCVertexCache::addLines(const std::vector<int> & lineindices)
{
  PRIVATE(this)->addLines(lineindices);
}

void
SoFCVertexCache::addLine(const SoPrimitiveVertex * v0,
                         const SoPrimitiveVertex * v1)
{
  PRIVATE(this)->prepare();

  const SoPrimitiveVertex *vp[2] = { v0,v1 };

  int32_t lineindices[2];

  const SoLineDetail * ld = nullptr;

  for (int i = 0; i < 2; i++) {
    SoFCVertexCacheP::Vertex v;
    v.vertex = vp[i]->getPoint();
    v.normal = vp[i]->getNormal();
    const SbVec4f & tmp = vp[i]->getTextureCoords();
    v.bumpcoord = SbVec2f(tmp[0], tmp[1]);
    v.texcoord0 = tmp;
    v.texcoordidx = -1;

    if (PRIVATE(this)->colorpervertex == 0) {
      v.color = PRIVATE(this)->firstcolor;
    }
    else {
      int midx = vp[i]->getMaterialIndex();
      if (PRIVATE(this)->tmp->packedptr) {
        v.color = PRIVATE(this)->tmp->packedptr[SbClamp(midx, 0, PRIVATE(this)->tmp->numdiffuse-1)];
      }
      else {
        SbColor tmpc = PRIVATE(this)->tmp->diffuseptr[SbClamp(midx,0,PRIVATE(this)->tmp->numdiffuse-1)];
        float tmpt = PRIVATE(this)->tmp->transpptr[SbClamp(midx,0,PRIVATE(this)->tmp->numtransp-1)];
        v.color = tmpc.getPackedValue(tmpt);
      }
      if (v.color != PRIVATE(this)->firstcolor) PRIVATE(this)->colorpervertex = 1;
      PRIVATE(this)->hastransp = (PRIVATE(this)->hastransp || (v.color&0xff) != 0xff);
    }

    const SoDetail * d = vp[i]->getDetail();

    if (d && d->isOfType(SoLineDetail::getClassTypeId())) {
      ld = static_cast<const SoLineDetail *>(d);
      const SoPointDetail * pd;
      if (i == 0) pd = ld->getPoint0();
      else pd = ld->getPoint1();

      int tidx  = v.texcoordidx = static_cast<const SoPointDetail *>(pd)->getTextureCoordIndex();
      if (PRIVATE(this)->tmp->numbumpcoords) {
        v.bumpcoord = PRIVATE(this)->tmp->bumpcoords[SbClamp(tidx, 0, PRIVATE(this)->tmp->numbumpcoords-1)];
      }
    }

    auto res = PRIVATE(this)->tmp->vhash.insert(
        std::make_pair(v, PRIVATE(this)->vertexarray->getLength()));
    if (res.second) {
      PRIVATE(this)->addVertex(v);
      // update texture coordinates for unit 1-n
      for (int j = 1; j <= PRIVATE(this)->lastenabled; j++) {
        if (v.texcoordidx >= 0 &&
            (PRIVATE(this)->tmp->multielem->getType(j) == SoMultiTextureCoordinateElement::EXPLICIT)) {
          PRIVATE(this)->multitexarray[j]->append(PRIVATE(this)->tmp->multielem->get4(j, v.texcoordidx));
        }
        else if (PRIVATE(this)->tmp->multielem->getType(j) == SoMultiTextureCoordinateElement::FUNCTION) {
          PRIVATE(this)->multitexarray[j]->append(PRIVATE(this)->tmp->multielem->get(j, v.vertex, v.normal));
        }
        else {
          PRIVATE(this)->multitexarray[j]->append(v.texcoord0);
        }
      }
    }
    lineindices[i] = res.first->second;
  }

  if (!PRIVATE(this)->lineindexer) {
    PRIVATE(this)->lineindexer =
      new SoFCVertexArrayIndexer(PRIVATE(this)->nodeid, PRIVATE(this)->tmp->prevlineindices);
    PRIVATE(this)->tmp->prevlineindices = NULL;
  }
  PRIVATE(this)->lineindexer->addLine(lineindices[0], lineindices[1], ld ? ld->getLineIndex() : -1);
}

void
SoFCVertexCache::addPoints(const std::map<int, int> & pointindices)
{
  PRIVATE(this)->addPoints(pointindices);
}

void
SoFCVertexCache::addPoints(const std::set<int> & pointindices)
{
  PRIVATE(this)->addPoints(pointindices);
}

void
SoFCVertexCache::addPoints(const std::vector<int> & pointindices)
{
  PRIVATE(this)->addPoints(pointindices);
}

void
SoFCVertexCache::addPoint(const SoPrimitiveVertex * v0)
{
  PRIVATE(this)->prepare();

  SoFCVertexCacheP::Vertex v;
  v.vertex = v0->getPoint();
  v.normal = v0->getNormal();
  const SbVec4f & tmp = v0->getTextureCoords();
  v.bumpcoord = SbVec2f(tmp[0], tmp[1]);
  v.texcoord0 = tmp;
  v.texcoordidx = -1;

  if (PRIVATE(this)->colorpervertex == 0) {
    v.color = PRIVATE(this)->firstcolor;
  }
  else {
    int midx = v0->getMaterialIndex();
    if (PRIVATE(this)->tmp->packedptr) {
      v.color = PRIVATE(this)->tmp->packedptr[SbClamp(midx, 0, PRIVATE(this)->tmp->numdiffuse-1)];
    }
    else {
      SbColor tmpc = PRIVATE(this)->tmp->diffuseptr[SbClamp(midx,0,PRIVATE(this)->tmp->numdiffuse-1)];
      float tmpt = PRIVATE(this)->tmp->transpptr[SbClamp(midx,0,PRIVATE(this)->tmp->numtransp-1)];
      v.color = tmpc.getPackedValue(tmpt);
    }
    if (v.color != PRIVATE(this)->firstcolor) PRIVATE(this)->colorpervertex = 1;
    PRIVATE(this)->hastransp = (PRIVATE(this)->hastransp || (v.color&0xff) != 0xff);
  }

  const SoDetail * d = v0->getDetail();

  if (d && d->isOfType(SoPointDetail::getClassTypeId())) {
    const SoPointDetail * pd = static_cast<const SoPointDetail *>(d);
    int tidx  = v.texcoordidx = pd->getTextureCoordIndex();
    if (PRIVATE(this)->tmp->numbumpcoords) {
      v.bumpcoord = PRIVATE(this)->tmp->bumpcoords[SbClamp(tidx, 0, PRIVATE(this)->tmp->numbumpcoords-1)];
    }
  }

  if (!PRIVATE(this)->pointindexer) {
    PRIVATE(this)->pointindexer =
      new SoFCVertexArrayIndexer(PRIVATE(this)->nodeid, PRIVATE(this)->tmp->prevpointindices);
    PRIVATE(this)->tmp->prevpointindices = NULL;
  }

  auto res = PRIVATE(this)->tmp->vhash.insert(
      std::make_pair(v, PRIVATE(this)->vertexarray->getLength()));
  if (res.second) {
    PRIVATE(this)->addVertex(v);
    // update texture coordinates for unit 1-n
    for (int j = 1; j <= PRIVATE(this)->lastenabled; j++) {
      if (v.texcoordidx >= 0 &&
          (PRIVATE(this)->tmp->multielem->getType(j) == SoMultiTextureCoordinateElement::EXPLICIT)) {
        PRIVATE(this)->multitexarray[j]->append(PRIVATE(this)->tmp->multielem->get4(j, v.texcoordidx));
      }
      else if (PRIVATE(this)->tmp->multielem->getType(j) == SoMultiTextureCoordinateElement::FUNCTION) {
        PRIVATE(this)->multitexarray[j]->append(PRIVATE(this)->tmp->multielem->get(j, v.vertex, v.normal));
      }
      else {
        PRIVATE(this)->multitexarray[j]->append(v.texcoord0);
      }
    }
  }
  PRIVATE(this)->pointindexer->addPoint(res.first->second);
}

SoNode *
SoFCVertexCache::getShapeInstance(const SoNode *node)
{
  if (!node)
    return nullptr;
  auto field = node->getField(*ShapeInstanceField);
  if (field && field->isOfType(SoSFNode::getClassTypeId()))
    return static_cast<const SoSFNode*>(field)->getValue();
  return nullptr;
}

SoFCVertexCache *
SoFCVertexCache::highlightIndices(int * pindex, SoNode * node, int idxstart, int idxend)
{
  if (!node)
    return this;
  auto field = node->getField(*HighlightIndicesField);
  if (!field || !field->isOfType(SoMFInt32::getClassTypeId()))
    return this;
  auto &indicesfield = *static_cast<const SoMFInt32*>(field);
  static FC_COIN_THREAD_LOCAL std::vector<int32_t> indices;
  indices.clear();
  for (int i=0, c=indicesfield.getNum(); i<c; ++i) {
    int idx = indicesfield[i];
    if (idx >= idxstart && (idx < idxend || idxend < 0))
      indices.push_back(idx - idxstart);
  }

  switch(indices.size()) {
  case 0:
    return this;
  case 1:
    if (pindex)
      *pindex = indices[0];
    return this;
  }

  auto cache = new SoFCVertexCache(*this);
  if (PRIVATE(this)->triangleindexer)
    cache->addTriangles(indices);
  else if (PRIVATE(this)->lineindexer)
    cache->addLines(indices);
  else if (PRIVATE(this)->pointindexer)
    cache->addPoints(indices);
  return cache;
}

int
SoFCVertexCache::getNumVertices(void) const
{
  return PRIVATE(this)->vertexarray ? PRIVATE(this)->vertexarray->getLength() : 0;
}

const SbVec3f *
SoFCVertexCache::getVertexArray(void) const
{
  return PRIVATE(this)->vertexarray ? PRIVATE(this)->vertexarray->getArrayPtr() : NULL;
}

const SbVec3f *
SoFCVertexCache::getNormalArray(void) const
{
  return PRIVATE(this)->normalarray ? PRIVATE(this)->normalarray->getArrayPtr() : NULL;
}

const SbVec4f *
SoFCVertexCache::getTexCoordArray(void) const
{
  return PRIVATE(this)->texcoord0array ? PRIVATE(this)->texcoord0array->getArrayPtr() : NULL;
}

const SbVec2f *
SoFCVertexCache::getBumpCoordArray(void) const
{
  return PRIVATE(this)->bumpcoordarray ? PRIVATE(this)->bumpcoordarray->getArrayPtr() : NULL;
}

const uint8_t *
SoFCVertexCache::getColorArray(void) const
{
  return PRIVATE(this)->colorarray ? PRIVATE(this)->colorarray->getArrayPtr() : NULL;
}

int
SoFCVertexCache::getNumTriangleIndices(void) const
{
  return PRIVATE(this)->triangleindexer ? PRIVATE(this)->triangleindexer->getNumIndices() : 0;
}

const GLint *
SoFCVertexCache::getTriangleIndices(void) const
{
  assert(PRIVATE(this)->triangleindexer);
  return PRIVATE(this)->triangleindexer->getIndices();
}

int32_t
SoFCVertexCache::getTriangleIndex(const int idx) const
{
  assert(PRIVATE(this)->triangleindexer);
  return PRIVATE(this)->triangleindexer->getIndices()[idx];
}

SbBool
SoFCVertexCache::colorPerVertex(void) const
{
  return PRIVATE(this)->colorpervertex > 0;
}

const SbVec4f *
SoFCVertexCache::getMultiTextureCoordinateArray(const int unit) const
{
  assert(unit <= PRIVATE(this)->lastenabled);
  if (!unit)
    return PRIVATE(this)->texcoord0array ? PRIVATE(this)->texcoord0array->getArrayPtr() : NULL;

  if (unit < (int)PRIVATE(this)->multitexarray.size()
      && PRIVATE(this)->multitexarray[unit]
      && PRIVATE(this)->multitexarray[unit]->getArrayPtr())
    return PRIVATE(this)->multitexarray[unit]->getArrayPtr();

  return NULL;
}

int
SoFCVertexCache::getNumLineIndices(void) const
{
  return PRIVATE(this)->lineindexer ? PRIVATE(this)->lineindexer->getNumIndices() : 0;
}

int
SoFCVertexCache::getNumPointIndices(void) const
{
  return PRIVATE(this)->pointindexer ? PRIVATE(this)->pointindexer->getNumIndices() : 0;
}


const GLint *
SoFCVertexCache::getLineIndices(void) const
{
  assert(PRIVATE(this)->lineindexer);
  return PRIVATE(this)->lineindexer->getIndices();
}

const GLint *
SoFCVertexCache::getPointIndices(void) const
{
  assert(PRIVATE(this)->pointindexer);
  return PRIVATE(this)->pointindexer->getIndices();
}

SbBool
SoFCVertexCacheP::depthSortTriangles(SoState * state, bool fullsort, const SbPlane *plane)
{
  if (!this->vertexarray) return FALSE;
  int numv = this->vertexarray->getLength();
  int numtri = PUBLIC(this)->getNumTriangleIndices() / 3;
  if (numv == 0 || numtri == 0) return FALSE;

  int numparts = this->triangleindexer->getNumParts();

  // must not mess up indices if there are parts
  if (numparts) {
    if (numparts == 1)
      return FALSE;
    if (!fullsort) {
      if (this->numtranspparts == numparts)
        fullsort = true;
      else {
        numparts = this->numtranspparts;
        if (!numparts)
          return FALSE;
      }
    }
  }

  SbPlane sortplane = plane?*plane:SoViewVolumeElement::get(state).getPlane(0.0);
  // move plane into object space
  sortplane.transform(SoModelMatrixElement::get(state).inverse());

  const SbVec3f * vptr = this->vertexarray->getArrayPtr();

  // If having parts, sort the parts (i.e. group of triangles) instead of
  // individual triangles
  if (numparts) {
    if (numparts == (int)this->deptharray.size()
        && sortplane.getNormal() == this->prevsortplane.getNormal())
      return TRUE;

    this->deptharray.clear();
    this->deptharray.reserve(numparts);
    if (fullsort) {
      if (this->triangleindexer->getPartialIndices().size()) {
        for (int i : this->triangleindexer->getPartialIndices())
          this->deptharray.emplace_back(i);
      }
      else {
        for (int i=0; i<numparts; ++i)
          this->deptharray.emplace_back(i);
      }
    }
    else if (this->triangleindexer->getPartialIndices().size()) {
      auto it = this->transppartindices.begin();
      auto itEnd = this->transppartindices.end();
      if (it != itEnd) {
        for (int i : this->triangleindexer->getPartialIndices()) {
          if (i < *it)
            continue;
          if (i == *it)
            this->deptharray.emplace_back(i);
          if (++it == itEnd)
            break;
        }
      }
    } else {
      for (int i : this->transppartindices)
        this->deptharray.emplace_back(i);
    }

    this->prevsortplane = sortplane;
    const int *parts = this->triangleindexer->getPartOffsets();
#ifdef FC_RENDER_SORT_NEAREST
    const GLint *indices = this->triangleindexer->getIndices();
    const SbVec3f *vertices = this->vertexarray->getArrayPtr();
    for(auto & entry : this->deptharray) {
      int prev = entry.index == 0 ? 0 : parts[entry.index-1];
      int n = parts[entry.index]-prev;
      entry.depth = -FLT_MAX;
      for (int k=0; k<n; ++k) {
        const SbVec3f & vertex = vertices[indices[k+prev]];
        float d = sortplane.getDistance(vertex);
        if (d > entry.depth)
          entry.depth = d;
      }
    }
#else
    for(auto & entry : this->deptharray)
      entry.depth = sortplane.getDistance(this->partcenters[entry.index]);
#endif

    std::sort(this->deptharray.begin(), this->deptharray.end(),
        [] (const SortEntry &a, const SortEntry &b) {
          return a.depth < b.depth;
        });

    this->sortedpartarray.resize(this->deptharray.size());
    this->sortedpartcounts.resize(this->deptharray.size());
    int i=0;
    int typesize = this->triangleindexer->useShorts() ? 2 : 4;
    for (auto & entry : this->deptharray) {
      int start = entry.index == 0 ? 0 : parts[entry.index-1];
      int end = parts[entry.index];
      this->sortedpartarray[i] = start * typesize;
      this->sortedpartcounts[i++] = end - start;
    }
    return TRUE;
  }

  // normal sorting without parts
  if (numtri == (int)deptharray.size()
      && sortplane.getNormal() == this->prevsortplane.getNormal())
    return FALSE;

  GLint * iptr = this->triangleindexer->getWriteableIndices();

  this->deptharray.clear();
  this->deptharray.reserve(numtri);
  for (int i=0; i<numtri; ++i) {
    this->deptharray.emplace_back(iptr[i*3], iptr[i*3+1], iptr[i*3+2]);
    double acc = 0.0;
    acc += sortplane.getDistance(vptr[iptr[i*3]]);
    acc += sortplane.getDistance(vptr[iptr[i*3+1]]);
    acc += sortplane.getDistance(vptr[iptr[i*3+2]]);
    this->deptharray.back().depth = (float) (acc / 3.0);
  }

  std::sort(this->deptharray.begin(), this->deptharray.end(),
      [] (const SortEntry &a, const SortEntry &b) {
        return a.depth < b.depth;
      });

  int i = 0;
  for(auto & entry : this->deptharray) {
    iptr[i++] = entry.index;
    iptr[i++] = entry.index2;
    iptr[i++] = entry.index3;
  }
  return FALSE;
}

void
SoFCVertexCacheP::initColor(int n)
{
  uint8_t r,g,b,a;
  r = (this->firstcolor >> 24) & 0xff;
  g = (this->firstcolor >> 16) & 0xff;
  b = (this->firstcolor >> 8) & 0xff;
  a = (this->firstcolor) & 0xff;
  for (int i=0; i<n; i+=4) {
    this->colorarray->append(r);
    this->colorarray->append(g);
    this->colorarray->append(b);
    this->colorarray->append(a);
  }
}

void
SoFCVertexCacheP::addVertex(const Vertex & v)
{
  this->vertexarray->append(v.vertex);
  this->normalarray->append(v.normal);
  if (this->texcoord0array) this->texcoord0array->append(v.texcoord0);
  if (this->bumpcoordarray) this->bumpcoordarray->append(v.bumpcoord);

  if (!this->colorarray && this->colorpervertex > 0) {
    SbFCUniqueId id = this->diffuseid + this->transpid + 0xb68cfe55;
    this->colorarray = new ByteArray(id, this->prevcolorarray);
    this->prevcolorarray.reset();
    initColor(this->vertexarray->getLength()*4 - 4);
  }

  if (this->colorarray) {
    uint8_t r,g,b,a;
    r = (v.color >> 24) & 0xff;
    g = (v.color >> 16) & 0xff;
    b = (v.color >> 8) & 0xff;
    a = (v.color) & 0xff;
    this->colorarray->append(r);
    this->colorarray->append(g);
    this->colorarray->append(b);
    this->colorarray->append(a);
  }
}

void
SoFCVertexCacheP::enableArrays(const cc_glglue * glue,
                               const SbBool color, const SbBool normal,
                               const SbBool texture, const SbBool * enabled,
                               const int lastenabled)
{
  int i;
  if (color && this->colorarray) {
    cc_glglue_glColorPointer(glue, 4, GL_UNSIGNED_BYTE, 0,
                             this->colorarray->getArrayPtr());
    cc_glglue_glEnableClientState(glue, GL_COLOR_ARRAY);
  }

  if (texture && this->texcoord0array) {
    cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0,
                                this->texcoord0array->getArrayPtr());
    cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

    for (i = 1; i <= lastenabled; i++) {
      if (enabled[i] && this->multitexarray[i]) {
        cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
        cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0,
                                    this->multitexarray[i]->getArrayPtr());
        cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);
      }
    }
  }
  if (normal && this->normalarray) {
    cc_glglue_glNormalPointer(glue, GL_FLOAT, 0,
                              this->normalarray->getArrayPtr());
    cc_glglue_glEnableClientState(glue, GL_NORMAL_ARRAY);
  }

  cc_glglue_glVertexPointer(glue, 3, GL_FLOAT, 0,
                            this->vertexarray->getArrayPtr());
  cc_glglue_glEnableClientState(glue, GL_VERTEX_ARRAY);
}


void
SoFCVertexCacheP::disableArrays(const cc_glglue * glue,
                                const SbBool color, const SbBool normal,
                                const SbBool texture, const SbBool * enabled,
                                const int lastenabled)
{
  int i;
  if (normal && this->normalarray) {
    cc_glglue_glDisableClientState(glue, GL_NORMAL_ARRAY);
  }
  if (texture && this->texcoord0array) {
    for (i = 1; i <= lastenabled; i++) {
      if (enabled[i] && this->multitexarray[i]) {
        cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
        cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
      }
    }
    if (lastenabled >= 1) {
      // reset to default
      cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0);
    }
    cc_glglue_glDisableClientState(glue, GL_TEXTURE_COORD_ARRAY);
  }
  if (color && this->colorarray) {
    cc_glglue_glDisableClientState(glue, GL_COLOR_ARRAY);
  }
  cc_glglue_glDisableClientState(glue, GL_VERTEX_ARRAY);
}

void
SoFCVertexCacheP::enableVBOs(SoState *state,
                             const cc_glglue * glue,
                             uint32_t contextid,
                             const SbBool color, const SbBool normal,
                             const SbBool texture, const SbBool * enabled,
                             const int lastenabled)
{
  if (!this->vertexarray) return;

#if 0
  if (!SoGLDriverDatabase::isSupported(glue, SO_GL_VBO_IN_DISPLAYLIST)) {
    SoCacheElement::invalidate(state);
    SoGLCacheContextElement::shouldAutoCache(state,
                                             SoGLCacheContextElement::DONT_AUTO_CACHE);
    state = NULL;
  }
#endif

  int i;
  if (color && this->colorarray) {
    this->colorarray->bindBuffer(state, contextid);
    cc_glglue_glColorPointer(glue, 4, GL_UNSIGNED_BYTE, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_COLOR_ARRAY);
  }
  if (texture && this->texcoord0array) {
    this->texcoord0array->bindBuffer(state, contextid);
    cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);

    for (i = 1; i <= lastenabled; i++) {
      if (!enabled[i] || !this->multitexarray[i]) continue;
      this->multitexarray[i]->bindBuffer(state, contextid);
      cc_glglue_glClientActiveTexture(glue, GL_TEXTURE0 + i);
      cc_glglue_glTexCoordPointer(glue, 4, GL_FLOAT, 0, NULL);
      cc_glglue_glEnableClientState(glue, GL_TEXTURE_COORD_ARRAY);
    }
  }
  if (normal && this->normalarray) {
    this->normalarray->bindBuffer(state, contextid);
    cc_glglue_glNormalPointer(glue, GL_FLOAT, 0, NULL);
    cc_glglue_glEnableClientState(glue, GL_NORMAL_ARRAY);
  }

  this->vertexarray->bindBuffer(state, contextid);
  cc_glglue_glVertexPointer(glue, 3, GL_FLOAT, 0, NULL);
  cc_glglue_glEnableClientState(glue, GL_VERTEX_ARRAY);
}

void
SoFCVertexCacheP::disableVBOs(const cc_glglue * glue,
                              const SbBool color, const SbBool normal,
                              const SbBool texture, const SbBool * enabled,
                              const int lastenabled)
{
  this->disableArrays(glue, color, normal, texture, enabled, lastenabled);
  cc_glglue_glBindBuffer(glue, GL_ARRAY_BUFFER, 0); // Reset VBO binding
}

void
SoFCVertexCacheP::renderImmediate(const cc_glglue * glue,
                                  const GLint * indices,
                                  const int numindices,
                                  const SbBool color, const SbBool normal,
                                  const SbBool texture, const SbBool * enabled,
                                  const int lastenabled)
{
  if (!this->vertexarray) return;

  const unsigned char * colorptr = NULL;
  const SbVec3f * normalptr = NULL;
  const SbVec3f * vertexptr = NULL;
  const SbVec4f * texcoordptr = NULL;
  
  if (color && this->colorarray) {
    colorptr = this->colorarray->getArrayPtr();
  }
  if (normal && this->normalarray) {
    normalptr = this->normalarray->getArrayPtr();
  }
  if (texture && this->texcoord0array) {
    texcoordptr = this->texcoord0array->getArrayPtr();
  }
  vertexptr = this->vertexarray->getArrayPtr();

  for (int i = 0; i < numindices; i++) {
    const int idx = indices[i];
    if (normalptr) {
      glNormal3fv(reinterpret_cast<const GLfloat *>(&normalptr[idx]));
    }
    if (colorptr) {
      glColor4ubv(reinterpret_cast<const GLubyte *>(&colorptr[idx*4]));
    }
    if (texcoordptr) {
      glTexCoord4fv(reinterpret_cast<const GLfloat *>(&texcoordptr[idx]));

      for (int j = 1; j <= lastenabled; j++) {
        if (!enabled[j] || !this->multitexarray[j]) continue;
        const SbVec4f * mt = this->multitexarray[j]->getArrayPtr();
        cc_glglue_glMultiTexCoord4fv(glue,
                                    GL_TEXTURE0 + j,
                                    reinterpret_cast<const GLfloat *>(&mt[idx]));
      }
    }
    glVertex3fv(reinterpret_cast<const GLfloat *>(&vertexptr[idx]));
  }
}

void
SoFCVertexCache::initClass()
{
  SoFCVertexArrayIndexer::initClass();
  SoFCVertexCacheP::initClass();
  SoFCVBO::init();
}

void
SoFCVertexCache::cleanup()
{
  SoFCVertexArrayIndexer::cleanup();
  SoFCVertexCacheP::cleanup();
}

SbVec3f
SoFCVertexCache::getCenter() const
{
  return getBoundingBox().getCenter();
}

const SbBox3f &
SoFCVertexCache::getBoundingBox() const
{
  if (PRIVATE(this)->boundbox.isEmpty())
    getBoundingBox(nullptr, PRIVATE(this)->boundbox);
  return PRIVATE(this)->boundbox;
}

void
SoFCVertexCache::getBoundingBox(const SbMatrix * matrix, SbBox3f & bbox) const
{
  const SbVec3f *vptr = getVertexArray();
  if (PRIVATE(this)->prevattached) {
    // means partial indexing, we need to explicitly iterate over indices
    if (PRIVATE(this)->triangleindexer)
      PRIVATE(this)->triangleindexer->getBoundingBox(matrix, bbox, vptr);
    if (PRIVATE(this)->lineindexer)
      PRIVATE(this)->lineindexer->getBoundingBox(matrix, bbox, vptr);
    if (PRIVATE(this)->pointindexer)
      PRIVATE(this)->pointindexer->getBoundingBox(matrix, bbox, vptr);
    return;
  }

  int num = getNumVertices();
  if (matrix) {
    for (int i=0; i<num; ++i) {
      SbVec3f v;
      matrix->multVecMatrix(vptr[i], v);
      bbox.extendBy(v);
    }
  }
  else {
    for (int i=0; i<num; ++i)
      bbox.extendBy(vptr[i]);
  }
}

void
SoFCVertexCacheP::getBoundingBox(const SbMatrix * matrix,
                                 SbBox3f & bbox,
                                 const SoFCVertexArrayIndexer *indexer,
                                 int part) const
{
  const SbVec3f *vptr = PUBLIC(this)->getVertexArray();
  if (part < 0 || this->prevattached)
    return indexer->getBoundingBox(matrix, bbox, vptr);

  const int * indices = indexer->getIndices();
  int numindices = indexer->getNumIndices();
  int numparts = indexer->getNumParts();
  if (!numparts) {
    int unit;
    switch(indexer->getTarget()) {
    case GL_POINTS:
      unit = 1;
      break;
    case GL_LINES:
      unit = 2;
      break;
    default:
      unit = 3;
    }
    part *= unit;
    if (part >= 0 && part < numindices) {
      SbVec3f v;
      int i = indices[part];
      if (matrix)
        matrix->multVecMatrix(vptr[i], v);
      else
        v = vptr[i];
      bbox.extendBy(v);
      return;
    }
  }
  if (part >= numparts)
    return;

  const GLint * parts = indexer->getPartOffsets();
  for (int i=part?parts[part-1]:0; i<parts[part]; ++i) {
    SbVec3f v;
    if (matrix)
      matrix->multVecMatrix(vptr[indices[i]], v);
    else
      v = vptr[indices[i]];
    bbox.extendBy(v);
  }
}

void
SoFCVertexCache::getTrianglesBoundingBox(const SbMatrix * matrix,
                                        SbBox3f & bbox,
                                        int part) const
{
  PRIVATE(this)->getBoundingBox(matrix, bbox, PRIVATE(this)->triangleindexer, part);
}

void
SoFCVertexCache::getLinesBoundingBox(const SbMatrix * matrix,
                                        SbBox3f & bbox,
                                        int part) const
{
  PRIVATE(this)->getBoundingBox(matrix, bbox, PRIVATE(this)->lineindexer, part);
}

void
SoFCVertexCache::getPointsBoundingBox(const SbMatrix * matrix,
                                       SbBox3f & bbox,
                                       int part) const
{
  PRIVATE(this)->getBoundingBox(matrix, bbox, PRIVATE(this)->pointindexer, part);
}

uint32_t
SoFCVertexCache::getFaceColor(int part) const
{
  return PRIVATE(this)->getColor(PRIVATE(this)->triangleindexer, part);
}

uint32_t
SoFCVertexCache::getLineColor(int part) const
{
  return PRIVATE(this)->getColor(PRIVATE(this)->lineindexer, part);
}

uint32_t
SoFCVertexCache::getPointColor(int part) const
{
  return PRIVATE(this)->getColor(PRIVATE(this)->pointindexer, part);
}

uint32_t
SoFCVertexCacheP::getColor(const SoFCVertexArrayIndexer * indexer, int part) const
{
  uint32_t color = this->firstcolor;
  if (!indexer || part < 0 || !this->colorpervertex || !this->colorarray)
    return color;

  const uint8_t * colors = this->colorarray->getArrayPtr();
  int colorlen = this->colorarray->getLength();
  const int * indices = indexer->getIndices();
  int numindices = indexer->getNumIndices();

  int numparts = indexer->getNumParts();
  if (!numparts) {
    int unit;
    switch(indexer->getTarget()) {
    case GL_POINTS:
      unit = 1;
      break;
    case GL_LINES:
      unit = 2;
      break;
    default:
      unit = 3;
    }
    part *= unit;
    if (part < 0 || part >= numindices)
      return color;

    int idx = indices[part]*4;
    if (idx >= 0 && idx+3 < colorlen) {
      colors += idx;
      color = colors[0] << 24;
      color |= colors[1] << 16;
      color |= colors[2] << 8;
      color |= colors[3];
    }
    return color;
  }
  if (part >= numparts)
    return color;

  const GLint * parts = indexer->getPartOffsets();
  int idx = indices[part ? parts[part-1] : 0] * 4;
  if (idx >= 0 && idx+3 < colorlen) {
    colors += idx;
    color = colors[0] << 24;
    color |= colors[1] << 16;
    color |= colors[2] << 8;
    color |= colors[3];
  }
  return color;
}

SoFCVertexCache *
SoFCVertexCache::getWholeCache() const
{
  if (PRIVATE(this)->prevattached)
    return PRIVATE(this)->prevcache;
  return const_cast<SoFCVertexCache*>(this);
}

#undef PRIVATE
#undef PUBLIC
// vim: noai:ts=2:sw=2
