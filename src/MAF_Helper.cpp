#include "MAF_Helper.h"

// 
// No real point in returning the status. I dont check anything dah
// @note As with the influence data values written in mof files, the joints
// will only be a part of the skeleton if the exercise any kind of influence
// over any part of the mesh, so to keep IDs consistent, joints with no influence
// over the mesh, will not be exported
// @important	
// @note Root SHOULDN'T have any influence over the mesh. 
// @note Right now I'm assuming that the root is always the parent of the first joint that influences the mesh.
// This could end up failing, I maybe need to recursively look until I get to a joint without parent? 
// 
MStatus MAF_Helper::GetAnimationData(MDagPath dagPath, Root& root, std::vector<Joint>& finalJoints, AnimationGatheringInformation informationToGather)
{
	MStatus				 status	     = MStatus::kSuccess;	

	MObject              skinCluster = Skinner::FindSkinCluster(dagPath);
	MFnSkinCluster       skinClusterFn(skinCluster, &status);
	MDagPathArray        jointsDags;
	unsigned int         jointCount  = skinClusterFn.influenceObjects(jointsDags);	
	
	switch (informationToGather)
	{
		case AnimationGatheringInformation::JOINT_HIERARCHY:
		{
			status = GetJoints(root, finalJoints, jointsDags);
			status = GetJointsParentID(finalJoints);
			GetJointsChildrenIDs(finalJoints);
			GetRootChildren(root, finalJoints);
		} break;

		case AnimationGatheringInformation::JOINT_TRANSFORMATION_OVER_THE_TIMELINE:
		{			
			status = GetJointTransformationsOvertTheTimeline(finalJoints);
		} break;

		case AnimationGatheringInformation::BOTH:
		{
			status = GetJoints(root, finalJoints, jointsDags);
			status = GetJointsParentID(finalJoints);
			GetJointsChildrenIDs(finalJoints);
			GetRootChildren(root, finalJoints);
			status = GetJointTransformationsOvertTheTimeline(finalJoints);
		} break;
	}

	return status;
}


MStatus MAF_Helper::GetJoints(Root& root, std::vector<Joint>& finalJoints, MDagPathArray& jointsDags)
{	
	MStatus status = MStatus::kSuccess;

	bool once = false;

	for (unsigned int i = 0; i < jointsDags.length(); i++)
	{					
		Joint joint{};
		joint.SetDagPath(jointsDags[i]);		
		
		if (!once)
		{
			MFnIkJoint	   firstChild(jointsDags[i]);			
			root.rootObj = firstChild.parent(0);
			once		 = true;
		}

		// @note I think joints are indexed as they appear in the outliner? so I might not need to do anything rather than 
		// assigning each joint id with their position as they appear in the Dag array... maybe...
		// This idea is backup by the fact that the weights in the .mof correspond with the way the joints are printed here. Let's see!
		//
		joint.influenceID = i; 
		
		MFnIkJoint mayaJoint(jointsDags[i]);

		for (unsigned int c = 0; c < mayaJoint.childCount(); c++)
		{
			joint.AddChild(mayaJoint.child(c));
		}

		finalJoints.emplace_back(joint);
	}

	return status;
}


MStatus MAF_Helper::GetJointsParentID(std::vector<Joint>& finalJoints)
{
	bool parentFound = false;
	for (size_t jIdx = 0; jIdx < finalJoints.size(); jIdx++)
	{
		MFnIkJoint parentJoint = finalJoints[jIdx].GetThisJoint().parent(0);

		for (size_t pIdx = 0; pIdx < finalJoints.size(); pIdx++)
		{	
			if (finalJoints[pIdx].GetThisJoint().name() == parentJoint.name())
			{
				finalJoints[jIdx].parentID = pIdx;
				parentFound = true;
			}			
		}

		if (!parentFound)
		{
			// @note If the parent isnt inside the list, their parent must be the root joint
			finalJoints[jIdx].parentID = -1;
		}

		parentFound = false;
	}

	return MStatus::kSuccess;
}


void MAF_Helper::GetJointsChildrenIDs(std::vector<Joint>& finalJoints)
{
	for (unsigned int jI = 0; jI < finalJoints.size(); jI++)
	{
		for (unsigned int cI = 0; cI < finalJoints[jI].childrenObjs.size(); cI++)
		{
			MFnIkJoint joint(finalJoints[jI].childrenObjs[cI]);
			MString childName = joint.name();

			for (unsigned int aL = 0; aL < finalJoints.size(); aL++)
			{
				if (finalJoints[aL].GetThisJoint().name() == childName)
				{
					finalJoints[jI].childrenIDs.emplace_back(aL);
					break;
				}
			}
		}
	}
}


void MAF_Helper::GetRootChildren(Root& root, std::vector<Joint>& finalJoints)
{
	
	MFnIkJoint rootJnt(root.rootObj);

	for (unsigned int cI = 0; cI < rootJnt.childCount(); cI++)
	{
		MFnIkJoint joint(rootJnt.child(cI));
		MString    childName = joint.name();

		for (unsigned int aL = 0; aL < finalJoints.size(); aL++)
		{
			if (finalJoints[aL].GetThisJoint().name() == childName)
			{
				root.childrenIDs.emplace_back(aL);
				break;
			}	
		}
	}
}


// @note I think I have to retrieve the inverse matrix of each parent? << Just to note it down >>
MStatus MAF_Helper::GetJointTransformationsOvertTheTimeline(std::vector<Joint>& finalJoints)
{
	MStatus status = MStatus::kSuccess;
	MTime   newTime;
	MTime   endFrame;

	endFrame = MAnimControl::animationEndTime();
	
	for (unsigned int cFrame = 0; cFrame <= endFrame.value(); cFrame++)
	{
		newTime.setValue(cFrame);
		MAnimControl::setCurrentTime(newTime);
	
		for (size_t jointIdx = 0; jointIdx < finalJoints.size(); jointIdx++)
		{
			MFnIkJoint joint = finalJoints[jointIdx].GetThisJoint();

			JointTransform transform{};
			status = GetTransform(joint, transform);

			finalJoints[jointIdx].transformPerFrame.emplace_back(transform);
		}
	}

	return status;
}


MStatus MAF_Helper::GetTransform(MFnIkJoint& joint, JointTransform& transform)
{
	MStatus status;	
	
	transform.position = joint.getTranslation(MSpace::kTransform, &status);
	if (status == MStatus::kFailure)  { Print("Failed to retrieve position information.");  }
	
	status = joint.getRotation(transform.rotation);
	if (status == MStatus::kFailure)  { Print("Failed to retrieve Quaternion information."); }

	status = joint.getRotation(transform.eulerRotation);
	if (status == MStatus::kFailure) { Print("Failed to retrieve Euler information."); }
	
	double scale[3];
	status = joint.getScale(scale);
	if (status == MStatus::kFailure) { Print("Failed to retrieve scale information."); }
	MVector vScale(scale[0], scale[1], scale[2]);
	transform.scale = vScale;

	double shear[3];
	status = joint.getShear(shear);
	if (status == MStatus::kFailure)  { Print("Failed to retrieve shear information."); }
	MVector vShear(shear[0], shear[1], shear[2]);
	transform.shear = vShear;
	
	return status;
}


MStatus MAF_Helper::GetTransformInFrameX(MFnIkJoint& joint, JointTransform& transform, int x)
{
	MTime nTime;
	nTime.setValue(x);

	MAnimControl::setCurrentTime(nTime);
	MStatus status;

	transform.position = joint.getTranslation(MSpace::kTransform, &status);
	if (status == MStatus::kFailure) { Print("Failed to retrieve position information."); }

	status = joint.getRotation(transform.rotation);
	if (status == MStatus::kFailure) { Print("Failed to retrieve Quaternion information."); }

	status = joint.getRotation(transform.eulerRotation);
	if (status == MStatus::kFailure) { Print("Failed to retrieve Euler information."); }

	double scale[3];
	status = joint.getScale(scale);
	if (status == MStatus::kFailure) { Print("Failed to retrieve scale information."); }
	MVector vScale(scale[0], scale[1], scale[2]);
	transform.scale = vScale;

	double shear[3];
	status = joint.getShear(shear);
	if (status == MStatus::kFailure) { Print("Failed to retrieve shear information."); }
	MVector vShear(shear[0], shear[1], shear[2]);
	transform.shear = vShear;

	return status;
}
