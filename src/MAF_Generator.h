#pragma once

#include <iostream>
#include <chrono>
#include <fstream>

#include <maya/MGlobal.h>
#include <maya/MItGeometry.h>

#include "MAF_Helper.h"
#include "Utilities.h"

// @note this is the cousing of the MOF format. Used to store animation data, Skeleton attributes, and keyframes.
namespace MAF_Generator
{
	MStatus ExportAnimation(std::string& path, std::string& format, bool deduplicate);
	MStatus WriteFile(std::string& path, std::string& format, Root& root, std::vector<Joint>& finalJoints);

	void SerializeJointFrameTransform(std::ofstream& file, JointTransform& transform);

}