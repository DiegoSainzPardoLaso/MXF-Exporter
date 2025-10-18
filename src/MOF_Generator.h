#pragma once

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <chrono>

#include <maya/MGlobal.h>  

#include <maya/MSelectionList.h>
#include <maya/MItSelectionList.h>
#include <maya/MFnMesh.h>
#include <maya/MPointArray.h>
#include <maya/MFloatVectorArray.h>
#include <maya/MFloatArray.h>
#include <maya/MIntArray.h>
#include <maya/MDagPath.h>
#include <maya/MFnSkinCluster.h> // @note Needs OpenMayaAnim.lib to be included
#include <maya/MItGeometry.h>
#include <maya/MWeight.h>
#include <maya/MItMeshPolygon.h>

#include "Types.h"
#include "Skinner.h"
#include "MAF_Helper.h"
#include "Utilities.h" 

namespace MOF_Generator
{		
	MStatus ExportMesh(std::string& path, std::string& format, bool deduplicate);
	void	WriteFile(std::vector<Vertex>& finalVertices, std::vector<int>& indices, std::vector<Joint> skeleton, Root& root, std::string& path, std::string& format, Type meshType);
	void	WriteJoint(std::fstream& file, Joint& joint);
	void	WriteRoot (std::fstream& file, Root& root);

	template <typename T>
	void Print(MString first, T fVal, MString second, T sVal, MString third, T tVal)
	{
		MString finalSentence = "";
		finalSentence += first;
		finalSentence += fVal;

		finalSentence += second;
		if (sVal != -1)
			finalSentence += sVal;

		finalSentence += third;

		if (tVal != -1)
			finalSentence += tVal;

		MGlobal::displayInfo(finalSentence);
	}
 
}
