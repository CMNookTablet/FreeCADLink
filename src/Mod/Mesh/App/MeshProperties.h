/***************************************************************************
 *   Copyright (c) Jürgen Riegel <juergen.riegel@web.de>                   *
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


#ifndef MESH_MESHPROPERTIES_H
#define MESH_MESHPROPERTIES_H

#include <vector>
#include <list>
#include <set>
#include <string>
#include <map>

#include <Base/Handle.h>
#include <Base/Matrix.h>
#include <Base/Vector3D.h>

#include <App/PropertyStandard.h>
#include <App/PropertyGeo.h>

#include "Core/MeshKernel.h"
#include "Mesh.h"


namespace Mesh
{

class MeshPy;

/** The normals property class.
 * Note: We need an own class for that to distinguish from the base vector list.
 * @author Werner Mayer
 */
class MeshExport PropertyNormalList: public App::_PropertyVectorList
{
    TYPESYSTEM_HEADER();

public:
    void transformGeometry(const Base::Matrix4D &rclMat);

};

/** Curvature information. */
struct MeshExport CurvatureInfo
{
    float fMaxCurvature, fMinCurvature;
    Base::Vector3f cMaxCurvDir, cMinCurvDir;

    bool operator == (const CurvatureInfo &other) const {
        return fMaxCurvature == other.fMaxCurvature
            && fMinCurvature == other.fMinCurvature
            && cMaxCurvDir == other.cMaxCurvDir
            && cMinCurvDir == other.cMinCurvDir;
    }
};

/** The Curvature property class.
 * @author Werner Mayer
 */
class MeshExport PropertyCurvatureList: public App::PropertyListsT<CurvatureInfo>
{
    typedef PropertyListsT<CurvatureInfo> inherited;

    TYPESYSTEM_HEADER_WITH_OVERRIDE();

public:
    enum { 
        MeanCurvature  = 0,  /**< Mean curvature */
        GaussCurvature = 1,  /**< Gaussian curvature */
        MaxCurvature   = 2,  /**< Maximum curvature */ 
        MinCurvature   = 3,  /**< Minimum curvature */
        AbsCurvature   = 4   /**< Absolute curvature */
    };

public:
    PropertyCurvatureList();
    ~PropertyCurvatureList();

    std::vector<float> getCurvature( int tMode) const;

    void transformGeometry(const Base::Matrix4D &rclMat);

    virtual PyObject* getPyObject(void) override;

    virtual App::Property *Copy(void) const override;
    virtual void Paste(const App::Property &from) override;

protected:
    virtual CurvatureInfo getPyValue(PyObject *) const override;

    virtual void restoreXML(Base::XMLReader &) override;
    virtual bool saveXML(Base::Writer &) const override;
    virtual bool canSaveStream(Base::Writer &) const override { return true; }
    virtual void restoreStream(Base::InputStream &s, unsigned count) override;
    virtual void saveStream(Base::OutputStream &) const override;
};

/** The mesh kernel property class.
 * @author Werner Mayer
 */
class MeshExport PropertyMeshKernel : public App::PropertyComplexGeoData
{
    TYPESYSTEM_HEADER();

public:
    PropertyMeshKernel();
    ~PropertyMeshKernel();

    /** @name Getter/setter */
    //@{
    /** This method references the passed mesh object and takes possession of it,
     * it does NOT create a copy.
     * The currently referenced mesh object gets de-referenced and possibly deleted
     * if its reference counter becomes zero.
     * However, the mesh gets saved before if a transaction is open at this time.
     * @note Make sure not to reference the internal mesh pointer pf this class in
     * client code. This could lead to crashes if not handled properly.
     */
    void setValuePtr(MeshObject* m);
    /** This method sets the mesh by copying the data. */
    void setValue(const MeshObject& m);
    /** This method sets the mesh by copying the data. */
    void setValue(const MeshCore::MeshKernel& m);
    /** Swaps the mesh data structure. */
    void swapMesh(MeshObject&);
    /** Swaps the mesh data structure. */
    void swapMesh(MeshCore::MeshKernel&);
    /** Returns a the attached mesh object by reference. It cannot be modified 
     * from outside.
     */
    const MeshObject &getValue(void) const;
    const MeshObject *getValuePtr(void) const;
    virtual unsigned int getMemSize (void) const;
    //@}

    /** @name Getting basic geometric entities */
    //@{
    const Data::ComplexGeoData* getComplexData() const;
    /** Returns the bounding box around the underlying mesh kernel */
    Base::BoundBox3d getBoundingBox() const;
    //@}

    /** @name Modification */
    //@{
    MeshObject* startEditing();
    void finishEditing();
    /// Transform the real mesh data
    void transformGeometry(const Base::Matrix4D &rclMat);
    void setPointIndices( const std::vector<std::pair<PointIndex, Base::Vector3f> >& );
    //@}

    /** @name Python interface */
    //@{
    /** Returns a Python wrapper for the referenced mesh object. It does NOT 
     * create a copy. However, the Python wrapper is marked as \a immutable so
     * that the mesh object cannot be modified from outside.
     */
    PyObject* getPyObject(void);
    /** This method copies the content, hence creates an new mesh object 
     * to copy the data. The passed argument can be an instance of the Python
     * wrapper for the mesh object or simply a list of triangles, i.e. a list
     * of lists of three floats.
     */
    void setPyObject(PyObject *value);
    //@}

    const char* getEditorName(void) const {
        return "MeshGui::PropertyMeshKernelItem";
    }

    /** @name Save/restore */
    //@{
    void Save (Base::Writer &writer) const;
    void Restore(Base::XMLReader &reader);

    void SaveDocFile (Base::Writer &writer) const;
    void RestoreDocFile(Base::Reader &reader);

    App::Property *Copy(void) const;
    void Paste(const App::Property &from);
    //@}

private:
    Base::Reference<MeshObject> _meshObject;
    MeshPy* meshPyObject;
};

} // namespace Mesh

#endif // MESH_MESHPROPERTIES_H
