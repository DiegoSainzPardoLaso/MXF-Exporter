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
	std::ofstream   file;
	MFnIkJoint     root(rootObj.rootObj);	
	MTime          endFrame;
	Print(root.childCount());
	endFrame = MAnimControl::animationEndTime();

	int   jointCount = finalJoints.size() + 1;						 
	int   frameCount = finalJoints[0].transformPerFrame.size(); 
	float frameRate  = GetFrameRate();



	if (!format.compare("Binary")) 
	{

		file.open(path, std::ios::out | std::ios::binary);

		file.write(reinterpret_cast<char*>(&jointCount), sizeof(int));
		file.write(reinterpret_cast<char*>(&frameCount), sizeof(int));
		file.write(reinterpret_cast<char*>(&frameRate),  sizeof(float));
	

		// ===========================================================================
		// Serialize
		//
		for (unsigned int fI = 0; fI <= endFrame.value(); fI++)
		{	
			MString message = "Frame ";
			message += fI;
			MGlobal::displayError(message);

			// The root always first
			JointTransform rootTransform;
			MAF_Helper::GetTransformInFrameX(root, rootTransform, fI);
			SerializeJointFrameTransform(file, rootTransform);
			 
			// And then each joint
			for (unsigned int jI = 0; jI < finalJoints.size(); jI++)
			{
				JointTransform transform;
				MFnIkJoint j = finalJoints[jI].GetThisJoint();
				MAF_Helper::GetTransformInFrameX(j, transform, fI);

				float posX = (float)transform.position.x;
				float posY = (float)transform.position.y;
				float posZ = (float)transform.position.z;
				
				file.write(reinterpret_cast<char*>(&posX), sizeof(float));
				file.write(reinterpret_cast<char*>(&posY), sizeof(float));
				file.write(reinterpret_cast<char*>(&posZ), sizeof(float));

			 
				float rotX = transform.rotation.x;
				float rotY = transform.rotation.y;
				float rotZ = transform.rotation.z;
				float rotW = transform.rotation.w;
				// Print("Rotation ", rotX, rotY, rotZ);
				file.write(reinterpret_cast<char*>(&rotX), sizeof(float));
				file.write(reinterpret_cast<char*>(&rotY), sizeof(float));
				file.write(reinterpret_cast<char*>(&rotZ), sizeof(float));
				file.write(reinterpret_cast<char*>(&rotW), sizeof(float));
		 
				float scaX = (float)transform.scale.x;
				float scaY = (float)transform.scale.y;
				float scaZ = (float)transform.scale.z;
				// Print("Scale ", scaX, scaY, scaZ);
				file.write(reinterpret_cast<char*>(&scaX), sizeof(float));
				file.write(reinterpret_cast<char*>(&scaY), sizeof(float));
				file.write(reinterpret_cast<char*>(&scaZ), sizeof(float));

				float shrX = (float)transform.shear.x;
				float shrY = (float)transform.shear.y;
				float shrZ = (float)transform.shear.z;
				// Print("Shear ", shrX, shrY, shrZ);
				file.write(reinterpret_cast<char*>(&shrX), sizeof(float));
				file.write(reinterpret_cast<char*>(&shrY), sizeof(float));
				file.write(reinterpret_cast<char*>(&shrZ), sizeof(float));


				// SerializeJointFrameTransform(file, finalJoints[jI].transformPerFrame[fI]);
				MGlobal::displayInfo("\n");
			}
			MGlobal::displayInfo("\n");
			MGlobal::displayInfo("\n");
			MGlobal::displayInfo("\n");
		}
		// ===========================================================================
		file.close();
	}
	else
	{
		file.open(path, std::ios::out);		 

		file << "Joint Count [ " << jointCount << " ] \n"; 
		file << "Frame Count [ " << frameCount << " ] \n";
		file << "Frame Rate  [ " << frameRate  << " ] \n";					

		for (unsigned int cFrame = 0; cFrame <= endFrame.value(); cFrame++)
		{			
			file << "Frame " << cFrame << "\n{\n";

			JointTransform transform;
			MAF_Helper::GetTransformInFrameX(root, transform, cFrame);

			file << "\n\t{\n";
			file << "\t\tPosition [ " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << " ]\n";
			file << "\t\tRotation [ " << transform.rotation.x << ", " << transform.rotation.y << ", " << transform.rotation.z << ", " << transform.rotation.w << " ]\n";
			// file << "\t\tRotation [ " << transform.eulerRotation.x << ", " << transform.eulerRotation.y << ", " << transform.eulerRotation.z << " ] <-- Euler (Radians)\n";
			// file << "\t\tRotation [ " << transform.eulerRotation.x * (180 / (float)3.1415926535f) << ", " << transform.eulerRotation.y * (180 / (float)3.1415926535f) << ", " << transform.eulerRotation.z * (180 / (float)3.1415926535f) << " ] <-- Euler (Degrees)\n";
			file << "\t\tScale    [ " << transform.scale.x << ", " << transform.scale.y << ", " << transform.scale.z << " ]\n";
			file << "\t\tShear    [ " << transform.shear.x << ", " << transform.shear.y << ", " << transform.shear.z << " ]\n";
			file << "\t} \n";

			for (unsigned int jointIdx = 0; jointIdx < finalJoints.size(); jointIdx++)
			{
				JointTransform transform = finalJoints[jointIdx].transformPerFrame[cFrame];
				file << "\n\t{\n";
				file << "\t\tPosition [ " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << " ]\n";								
				file << "\t\tRotation [ " << transform.rotation.x << ", " << transform.rotation.y << ", " << transform.rotation.z << ", " << transform.rotation.w << " ]\n";
				// file << "\t\tRotation [ " << transform.eulerRotation.x << ", " << transform.eulerRotation.y << ", " << transform.eulerRotation.z << " ] <-- Euler (Radians)\n";
				// file << "\t\tRotation [ " << transform.eulerRotation.x * (180 / (float)3.1415926535f) << ", " << transform.eulerRotation.y * (180 / (float)3.1415926535f) << ", " << transform.eulerRotation.z * (180 / (float)3.1415926535f) << " ] <-- Euler (Degrees)\n";
				file << "\t\tScale    [ " << transform.scale.x << ", " << transform.scale.y << ", " << transform.scale.z << " ]\n";
				file << "\t\tShear    [ " << transform.shear.x << ", " << transform.shear.y << ", " << transform.shear.z << " ]\n";
				file << "\t} \n";
			}

			file << "} \n\n";
		}		

		file.close();
	}


	return MStatus::kSuccess;
}


void MAF_Generator::SerializeJointFrameTransform(std::ofstream& file, JointTransform& transform)
{	
	float posX = (float)transform.position.x;
	float posY = (float)transform.position.y;
	float posZ = (float)transform.position.z;
	Print("Position ", posX, posY, posZ);
	file.write(reinterpret_cast<char*>(&posX), sizeof(float));
	file.write(reinterpret_cast<char*>(&posY), sizeof(float));
	file.write(reinterpret_cast<char*>(&posZ), sizeof(float));

	float rotX = (float)transform.rotation.x;
	float rotY = (float)transform.rotation.y;
	float rotZ = (float)transform.rotation.z;
	float rotW = (float)transform.rotation.w;
	Print("Rotation ", rotX, rotY, rotZ);
	file.write(reinterpret_cast<char*>(&rotX), sizeof(float));
	file.write(reinterpret_cast<char*>(&rotY), sizeof(float));
	file.write(reinterpret_cast<char*>(&rotZ), sizeof(float));
	file.write(reinterpret_cast<char*>(&rotW), sizeof(float));		
	
	float scaX = (float)transform.scale.x;
	float scaY = (float)transform.scale.y;
	float scaZ = (float)transform.scale.z;		
	Print("Scale ", scaX, scaY, scaZ);
	file.write(reinterpret_cast<char*>(&scaX), sizeof(float));		
	file.write(reinterpret_cast<char*>(&scaY), sizeof(float));		
	file.write(reinterpret_cast<char*>(&scaZ), sizeof(float));		

	float shrX = (float)transform.shear.x;
	float shrY = (float)transform.shear.y;
	float shrZ = (float)transform.shear.z;	
	Print("Shear ", shrX, shrY, shrZ);
	file.write(reinterpret_cast<char*>(&shrX), sizeof(float));		
	file.write(reinterpret_cast<char*>(&shrY), sizeof(float));		
	file.write(reinterpret_cast<char*>(&shrZ), sizeof(float));
}