#pragma once

#include <vector>

#include <maya/MItDependencyGraph.h>
#include <maya/MDagPath.h>
#include <maya/MArgList.h>
#include <maya/MItSelectionList.h>
#include <maya/MGlobal.h>
#include <maya/MDagPathArray.h>
#include <maya/MFnSkinCluster.h>
#include <maya/MFloatArray.h>
#include <maya/MFnIkJoint.h>
#include <maya/MItGeometry.h>
#include <maya/MItDependencyNodes.h>
#include <maya/MFnWeightGeometryFilter.h>
#include <maya/MFnSet.h>

#include "Utilities.h"
#include "Types.h"

// Some of the code has been taken from https://help.autodesk.com/view/MAYAUL/2024/ENU/?guid=MAYA_API_REF_cpp_ref_skin_cluster_weights_2skin_cluster_weights_8cpp_example_html
namespace Skinner
{
    MStatus FindMeshWeightsAndInfluences(MDagPath dagPath, std::vector<Vertex>& verticesWithWeightsAndIDs);
   
    bool    IsSkinClusterIncluded(MObjectArray& skinClusterArray, MObject& node);
    MObject FindSkinCluster(MDagPath& dagPath);       
}