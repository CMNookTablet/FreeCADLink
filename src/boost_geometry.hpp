#ifndef FREECAD_GEOMETRY_HPP_WORKAROUND
#define FREECAD_GEOMETRY_HPP_WORKAROUND

// Workaround for boost geometry critical bug, fixed in
// https://github.com/boostorg/geometry/pull/886
#define BOOST_ALLOW_DEPRECATED_HEADERS
#include <boost/geometry.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/geometry/geometries/geometries.hpp>
#include <boost/geometry/geometries/register/point.hpp>
#include <boost/geometry/geometries/register/box.hpp>
#undef BOOST_ALLOW_DEPRECATED_HEADERS

#endif // #ifndef FREECAD_GEOMETRY_HPP_WORKAROUND
