#include "MAF_Generator.h"

// 
// @note I just had a realization. To export the joint transformation for each keyframe, I'm gonna traverse the timeline manually
// So I'll go from frame 0 to frame n and gather the transform information for each joint
//
MStatus MAF_Generator::ExportAnimation(std::string& path, std::string& format, bool deduplicate)
{
	std::chrono::time_point<std::chrono::high_resolution_clock> start = std::chrono::high_resolution_clock::now();

	MStatus			   status = MStatus::kSuccess;
	MDagPath		   selectionDagPath;
	Root     		   root;
	std::vector<Joint> finalJoints;
	MSelectionList	   selectionList;

	MGlobal::getActiveSelectionList(selectionList);

	if (selectionList.length() != 1) { return Status("Select just on object", MStatus::kFailure); }

	MItSelectionList iter(selectionList, MFn::kMesh, &status);
	if (status != MStatus::kSuccess) { return status; }

	iter.getDagPath(selectionDagPath);
	status = MAF_Helper::GetAnimationData(selectionDagPath, root, finalJoints, AnimationGatheringInformation::BOTH);
	status = WriteFile(path, format, root, finalJoints);

	std::chrono::time_point<std::chrono::high_resolution_clock> end = std::chrono::high_resolution_clock::now();

	float duration = std::chrono::duration<float, std::chrono::seconds::period>(end - start).count();

	int frames = finalJoints[0].transformPerFrame.size() - 1;
	MString info = "Time that took to export a [ "; info += frames; info += " ] frames animation: "; info += duration; info += " seconds";
	MGlobal::displayInfo(info);

	return status;
}


MStatus MAF_Generator::WriteFile(std::string& path, std::string& format, Root& rootObj, std::vector<Joint>& finalJoints)
{
	std::fstream   file;
	MFnIkJoint     root(rootObj.rootObj);	
	MTime          endFrame;
	Print(root.childCount());
	endFrame = MAnimControl::animationEndTime();

	if (!format.compare("Binary"))
	{
		file.open(path, std::ios::out | std::ios::binary);
	}
	else
	{
		file.open(path, std::ios::out);		 

		file << "Joint Count [ " << finalJoints.size() + 1 << " ] \n"; // @note +1 the root
		file << "Frame Count [ " << finalJoints[0].transformPerFrame.size() - 1 << " ] \n";
		file << "Frame Rate  [ " << GetFrameRate() << " ] \n";
		

		file << "Root: " << root.name() << " [ -1 ] --- PARENT IDX [ -1 ]" << "\n{\n";
		for (unsigned int rootFrameIdx = 0; rootFrameIdx < endFrame.value(); rootFrameIdx++)
		{
			JointTransform transform{};
			MAF_Helper::GetTransform(root, transform);
		 
		 	file << "\tFrame " << rootFrameIdx << "\n\t{\n";
		 	file << "\t\tPosition [ " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << " ]\n";			 
			file << "\t\tRotation [ " << transform.rotation.x << ", " << transform.rotation.y << ", " << transform.rotation.z << ", " << transform.rotation.w << " ]\n";
		 	// file << "\t\tOrientation [ " << transform.eulerRotation.x << ", " << transform.eulerRotation.y << ", " << transform.eulerRotation.z << " ] <-- Euler (Radians)\n";
		 	// file << "\t\tOrientation [ " << transform.eulerRotation.x * (180 / (float)3.1415926535f) << ", " << transform.eulerRotation.y * (180 / (float)3.1415926535f) << ", " << transform.eulerRotation.z * (180 / (float)3.1415926535f) << " ] <-- Euler (Degrees)\n";
		 	file << "\t\tScale	 [ " << transform.scale.x << ", " << transform.scale.y << ", " << transform.scale.z << " ]\n";
		 	file << "\t\tShear	 [ " << transform.shear.x << ", " << transform.shear.y << ", " << transform.shear.z << " ]\n";
		 	file << "\t} \n";
		}

		file << "} \n";


		for (unsigned int jointIdx = 0; jointIdx < finalJoints.size(); jointIdx++)
		{
			file << "Joint: " << finalJoints[jointIdx].name << " [" << jointIdx << "] --- PARENT IDX [ " << finalJoints[jointIdx].parentID << " ]\n{\n";		
			
			for (unsigned int cFrame = 0; cFrame < endFrame.value(); cFrame++)
			{
				JointTransform transform = finalJoints[jointIdx].transformPerFrame[cFrame];

				file << "\tFrame " << cFrame << "\n\t{\n";
				file << "\t\tPosition [ " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << " ]\n";								
				file << "\t\tRotation [ " << transform.rotation.x << ", " << transform.rotation.y << ", " << transform.rotation.z << ", " << transform.rotation.w << " ]\n";
				// file << "\t\tRotation [ " << transform.eulerRotation.x << ", " << transform.eulerRotation.y << ", " << transform.eulerRotation.z << " ] <-- Euler (Radians)\n";
				// file << "\t\tRotation [ " << transform.eulerRotation.x * (180 / (float)3.1415926535f) << ", " << transform.eulerRotation.y * (180 / (float)3.1415926535f) << ", " << transform.eulerRotation.z * (180 / (float)3.1415926535f) << " ] <-- Euler (Degrees)\n";
				file << "\t\tScale    [ " << transform.scale.x << ", " << transform.scale.y << ", " << transform.scale.z << " ]\n";
				file << "\t\tShear    [ " << transform.shear.x << ", " << transform.shear.y << ", " << transform.shear.z << " ]\n";
				file << "\t} \n";
			}

			file << "} \n";
		}		
	}


	return MStatus::kSuccess;
}

 