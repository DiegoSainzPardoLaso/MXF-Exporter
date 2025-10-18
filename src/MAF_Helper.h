#pragma once

#include <iostream>
#include <vector>

#include <maya/MGlobal.h>
#include <maya/MSelectionList.h>
#include <maya/MDagPath.h>
#include <maya/MItDependencyGraph.h>
#include <maya/MAnimControl.h>


#include "Skinner.h"
#include "Types.h"

namespace MAF_Helper
{
	MStatus GetAnimationData(MDagPath dagPath, Root& rootObj, std::vector<Joint>& finalJoints, AnimationGatheringInformation informationToGather);
	MStatus GetJoints(Root& rootObj, std::vector<Joint>& finalJoints, MDagPathArray& jointDags);
	MStatus GetJointsParentID(std::vector<Joint>& finalJoints);
	void    GetJointsChildrenIDs(std::vector<Joint>& finalJoints);
	void	GetRootChildren(Root& root, std::vector<Joint>& finalJoints);
	MStatus GetJointTransformationsOvertTheTimeline(std::vector<Joint>& finalJoints);
	MStatus GetTransform(MFnIkJoint& joint, JointTransform& transform);
}