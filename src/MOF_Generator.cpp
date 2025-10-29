#include "MOF_Generator.h"

// @note add the possibility of exporting multiple meshes affected by the same skeleton
MStatus MOF_Generator::ExportMesh(std::string& path, std::string& format, bool deduplicate)
{
    auto start = std::chrono::high_resolution_clock::now();

    MStatus                         status;
    Type                            meshType = Type::Static;
    MDagPath                        selection_DagPath;
    MSelectionList                  selectionList;
    std::vector<Vertex>             finalVertices;
    std::vector<int>                indices;
    std::unordered_map<Vertex, int> hashedVertices;
    int                             counter            = 0;
    int                             duplicatedVertices = 0;

    // ==========================================================================================================
    // Extract Mesh from selection
    // ==========================================================================================================
    MGlobal::getActiveSelectionList(selectionList);
    if (selectionList.length() != 1)  { return Status("Select just on object", MStatus::kFailure); }

    MItSelectionList iter(selectionList, MFn::kMesh, &status);
    if (status != MStatus::kSuccess) { return status; }

    iter.getDagPath(selection_DagPath);
    selection_DagPath.extendToShape();

    MFnMesh mesh(selection_DagPath, &status);
    if (status != MStatus::kSuccess) { return Status("Failed to access selected mesh", status); }
    

    // ==========================================================================================================
    // Get components: Positions, Normals, UVs, Colors, Weights and Influence IDs
    // ==========================================================================================================
    MFloatVectorArray normals;
    mesh.getNormals(normals, MSpace::kWorld);
    
    MString uvSetName;
    mesh.getCurrentUVSetName(uvSetName);
    MFloatArray uArray, vArray;
    mesh.getUVs(uArray, vArray, &uvSetName);
    
    MString colorSetName;
    if (mesh.numColorSets() > 0) { mesh.getCurrentColorSetName(colorSetName); }

    std::vector<Vertex> verticesWithWeightsAndIDs;   
    status = Skinner::FindMeshWeightsAndInfluences(selection_DagPath, verticesWithWeightsAndIDs);
    // if (status == MStatus::kFailure) { return status; }
    if (verticesWithWeightsAndIDs.size() != 0) { meshType = Type::Animated; }
    // ==========================================================================================================
    // Iterate through the mesh and create vertices to then generate the .mof file
    // ==========================================================================================================
    MObject        meshObj = selection_DagPath.node();
    MItMeshPolygon polyIt(meshObj, &status);

    if (status != MStatus::kSuccess) { return Status("Failed to create polygon iterator", status); }

    for (; !polyIt.isDone(); polyIt.next())
    {
        // Triangulated vertex indices
        MPointArray triPoints;
        MIntArray   triVertIds;
        polyIt.getTriangles(triPoints, triVertIds);

        // Polygon vertex indices
        MIntArray polygonVertices;
        polyIt.getVertices(polygonVertices);
        
        int faceIndex = polyIt.index();

        int triIdx = 0;
        for (unsigned int i = 0; i < triVertIds.length(); ++i)
        {
            int globalVertexId = triVertIds[triIdx];
            triIdx++;

            Vertex vert{};            
            mesh.getPoint(globalVertexId, vert.position, MSpace::kObject);
            vert.color  = MColor(1.0f, 1.0f, 1.0f);
            vert.normal = { 0.0f, 0.0f, 0.0f };
            vert.u      = 0.0f;
            vert.v      = 0.0f;


            // Local vertex index
            int localIndex = -1;
            for (unsigned int k = 0; k < polygonVertices.length(); ++k) 
            {
                if (polygonVertices[k] == globalVertexId) 
                {
                    localIndex = k; 
                    break; 
                }
            }
                                             
            if (localIndex >= 0)
            {                               
                unsigned int nIdx = polyIt.normalIndex(localIndex, &status);
                vert.normal = normals[nIdx];                                              

                int uvIndex = -1;
                if (polyIt.getUVIndex(localIndex, uvIndex, &uvSetName) == MStatus::kSuccess)
                {
                    if (uvIndex >= 0 && uvIndex < (int)uArray.length()) 
                    {
                        vert.u = uArray[uvIndex];
                        vert.v = vArray[uvIndex];
                    }
                }
             
                if (mesh.numColors() > 0)
                {
                    int colorIndex = -1;
                    if (polyIt.getColorIndex(localIndex, colorIndex, &colorSetName) == MStatus::kSuccess)
                    {
                        mesh.getColor(colorIndex, vert.color, &colorSetName);
                    }
                }
            }

            if (deduplicate) 
            {            
                if (!hashedVertices.contains(vert)) 
                {
                    hashedVertices[vert] = counter;
                    finalVertices.emplace_back(vert);
                    counter++;
                }
                else
                {            
                    duplicatedVertices++;
                }               

                indices.emplace_back(hashedVertices[vert]);
            }
            else 
            {
                if (!hashedVertices.contains(vert))
                {
                    hashedVertices[vert] = duplicatedVertices;
                    duplicatedVertices++;
                }

                finalVertices.emplace_back(vert);                
                indices.emplace_back(counter);
                counter++;                
            }
        }
    }

    // ==========================================================================================================
    // Assign the influence IDs and their respective weights
    // ==========================================================================================================   

    MGlobal::displayInfo("NAN");

    for (size_t fvIdx = 0; fvIdx < finalVertices.size(); fvIdx++)
    {
        for (size_t wIdvIdx = 0; wIdvIdx < verticesWithWeightsAndIDs.size(); wIdvIdx++)
        {
            if (finalVertices[fvIdx].Compare(verticesWithWeightsAndIDs[wIdvIdx]))
            {
                // Cop-Cop Copy
                for (size_t cpyIdx = 0; cpyIdx < 4; cpyIdx++)
                {   
                    finalVertices[fvIdx].jointID[cpyIdx] = verticesWithWeightsAndIDs[wIdvIdx].jointID[cpyIdx];
                    finalVertices[fvIdx].weight[cpyIdx]  = verticesWithWeightsAndIDs[wIdvIdx].weight [cpyIdx];
                }
            }
        }
    }

    // ==========================================================================================================
    // Get Skeleton bones IDs 
    // ==========================================================================================================    
    std::vector<Joint> skeleton{};
    Root               root;
    MAF_Helper::GetAnimationData(selection_DagPath, root, skeleton, AnimationGatheringInformation::JOINT_HIERARCHY);

    // ==========================================================================================================
    // Write file and display the time that it took to process the model export
    // ==========================================================================================================   
    WriteFile(finalVertices, indices, skeleton, root, path, format, meshType);

    Print("Unique Vertices [", counter, "]  Duplicated Vertices [", duplicatedVertices, "]", -1);

    auto end       = std::chrono::high_resolution_clock::now();
    float duration = std::chrono::duration<float>(end - start).count();

    Print("Processed ", (float)finalVertices.size(), " vertices in ", duration, " seconds", -1.0f);

    return MStatus::kSuccess;
}

void MOF_Generator::WriteFile(std::vector<Vertex>& finalVertices, std::vector<int>& indices, std::vector<Joint> skeleton, Root& root, std::string& path, std::string& format, Type meshType)
{	
	std::ofstream file;
    MFnIkJoint   rootJnt(root.rootObj);

	if (!format.compare("Binary"))
	{
		int vCount = (int)finalVertices.size();
		int stride = 11;
        if (meshType == Type::Animated) { stride = 19; }

		file.open(path, std::ios::out | std::ios::binary);

		file.write(reinterpret_cast<char*>(&vCount), sizeof(int));
		file.write(reinterpret_cast<char*>(&stride), sizeof(int));

		for (size_t v = 0; v < finalVertices.size(); v++)
		{
			Vertex tmpVertex = finalVertices[v];

			float pX = (float)tmpVertex.position.x;
			float pY = (float)tmpVertex.position.y;
			float pZ = (float)tmpVertex.position.z;

			file.write(reinterpret_cast<char*>(&pX), sizeof(float));
			file.write(reinterpret_cast<char*>(&pY), sizeof(float));
			file.write(reinterpret_cast<char*>(&pZ), sizeof(float));

			float cR = (float)tmpVertex.color.r;
			float cG = (float)tmpVertex.color.g;
			float cB = (float)tmpVertex.color.b;

			file.write(reinterpret_cast<char*>(&cR), sizeof(float));
			file.write(reinterpret_cast<char*>(&cG), sizeof(float));
			file.write(reinterpret_cast<char*>(&cB), sizeof(float));

			float nX = (float)tmpVertex.normal.x;
			float nY = (float)tmpVertex.normal.y;
			float nZ = (float)tmpVertex.normal.z;

			file.write(reinterpret_cast<char*>(&nX), sizeof(float));
			file.write(reinterpret_cast<char*>(&nY), sizeof(float));
			file.write(reinterpret_cast<char*>(&nZ), sizeof(float));

			file.write(reinterpret_cast<char*>(&tmpVertex.u), sizeof(float));
			file.write(reinterpret_cast<char*>(&tmpVertex.v), sizeof(float));

            if (meshType == Type::Animated)
            {
                float nIdx0 = (tmpVertex.jointID[0] + 1);
                float nIdx1 = (tmpVertex.jointID[1] + 1);
                float nIdx2 = (tmpVertex.jointID[2] + 1);
                float nIdx3 = (tmpVertex.jointID[3] + 1);
			    file.write(reinterpret_cast<char*>(&nIdx0), sizeof(int));
			    file.write(reinterpret_cast<char*>(&nIdx1), sizeof(int));
			    file.write(reinterpret_cast<char*>(&nIdx2), sizeof(int));
			    file.write(reinterpret_cast<char*>(&nIdx3), sizeof(int));
            
                file.write(reinterpret_cast<char*>(&tmpVertex.weight[0]), sizeof(float));
                file.write(reinterpret_cast<char*>(&tmpVertex.weight[1]), sizeof(float));
                file.write(reinterpret_cast<char*>(&tmpVertex.weight[2]), sizeof(float));
                file.write(reinterpret_cast<char*>(&tmpVertex.weight[3]), sizeof(float));
            }
		}

		int iCount = indices.size();				
		file.write(reinterpret_cast<char*>(&iCount), sizeof(int));
		for (size_t i = 0; i < indices.size(); i++)
		{
			file.write(reinterpret_cast<char*>(&indices[i]), sizeof(int));			
		}

        if (meshType == Type::Animated)
        {         
            MAnimControl::setCurrentTime(MAnimControl::animationStartTime());

            // @note the +1 is the root.
            int size = skeleton.size() + 1;
            file.write(reinterpret_cast<const char*>(&size), sizeof(int));
            
            WriteRoot(file, root);
            
            for (size_t jnt = 0; jnt < skeleton.size(); jnt++)
            {   
                Joint tmpJoint = skeleton[jnt];
                WriteJoint(file, tmpJoint);
            }
        }

	}
	else // Just for debuggin purposes
	{
        int stride = 11;
        if (meshType == Type::Animated) { stride = 19; }

		file.open(path, std::ios::out);

		file << finalVertices.size() << "\n";
		file << stride << "\n";
		
		for (size_t i = 0; i < finalVertices.size(); i++)
		{
			Vertex tmpVertex = finalVertices[i];
			file << tmpVertex.position.x << ", ";
			file << tmpVertex.position.y << ", ";
			file << tmpVertex.position.z << ", ";

			file << tmpVertex.color.r << ", ";
			file << tmpVertex.color.g << ", ";
			file << tmpVertex.color.b << ", ";

			file << tmpVertex.normal.x << ", ";
			file << tmpVertex.normal.y << ", ";
			file << tmpVertex.normal.z << ", ";

			file << tmpVertex.u << ", ";
			file << tmpVertex.v << ", ";

            if (meshType == Type::Animated)
            {
                // Need to add 1 because, the Root is the index 0 of the skeleton joints...
                file << tmpVertex.jointID[0] + 1 << ", ";
                file << tmpVertex.jointID[1] + 1 << ", ";
                file << tmpVertex.jointID[2] + 1 << ", ";
                file << tmpVertex.jointID[3] + 1 << ", ";

                file << tmpVertex.weight[0] << ", ";
                file << tmpVertex.weight[1] << ", ";
                file << tmpVertex.weight[2] << ", ";
                file << tmpVertex.weight[3] << ", \n";
            }
            else
            {
                file << "\n";
            }
		}

		file << indices.size() << ", \n";

		int nLine = -1;

		for (size_t i = 0; i < indices.size(); i++)
		{
			file << indices[i] << ", ";
			if ((nLine = (nLine + 1) % 3) == 2)
			{
				file << "\n";
			}
		}

        if (meshType == Type::Animated)
        {
            // @important 
            // @note The first frame of animation is the one that will be placed as the default A/T pose. Export the MOF file with an A pose as a base mesh
            // if the model is animated
            //
            JointTransform transform{};
            MAnimControl::setCurrentTime(MAnimControl::animationStartTime());

            // @warning Right now I don't have the root children!!!
            // @note the +1 is the root.
            file << skeleton.size() + 1 << "\n";
            file << rootJnt.name() << " --- Idx [0] | Parent Idx [0] --- Children [ " << root.childrenIDs.size() << " ] | ";

            for (unsigned int cI = 0; cI < root.childrenIDs.size(); cI++)
            {
                file << root.childrenIDs[cI] + 1 << " ";
            }

            file << "\n";

            MAF_Helper::GetTransform(rootJnt, transform);
            file << "Position [ " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << "]\n";
            file << "Rotation [ " << transform.rotation.x << ", " << transform.rotation.y << ", " << transform.rotation.z << ", " << transform.rotation.w << "]\n";
            file << "Scale    [ " << transform.scale.x << ", " << transform.scale.y << ", " << transform.scale.z << "]\n";
            file << "Shear    [ " << transform.shear.x << ", " << transform.shear.y << ", " << transform.shear.z << "]\n\n";

            for (size_t jnt = 0; jnt < skeleton.size(); jnt++)
            {

                file << skeleton[jnt].name << " --- Idx [" << skeleton[jnt].influenceID + 1 << "] | Parent Idx [" << skeleton[jnt].parentID + 1 << "] --- ";
                
                file << "Children [" << skeleton[jnt].childrenIDs.size() << "] | ";
                for (unsigned int cI = 0; cI < skeleton[jnt].childrenIDs.size(); cI++)
                {
                    file << skeleton[jnt].childrenIDs[cI] + 1 << " ";
                }
                file << "\n";

                MFnIkJoint joint = skeleton[jnt].GetThisJoint();
                MAF_Helper::GetTransform(joint, transform);
                file << "Position [ " << transform.position.x << ", " << transform.position.y << ", " << transform.position.z << "]\n";
                file << "Rotation [ " << transform.rotation.x << ", " << transform.rotation.y << ", " << transform.rotation.z << ", " << transform.rotation.w << "]\n";
                file << "Scale    [ " << transform.scale.x << ", " << transform.scale.y << ", " << transform.scale.z << "]\n";
                file << "Shear    [ " << transform.shear.x << ", " << transform.shear.y << ", " << transform.shear.z << "]\n\n";

            }
        }
	}

	file.close();
}


void MOF_Generator::WriteJoint(std::ofstream& file, Joint& joint)
{
    MFnIkJoint     mJoint = joint.GetThisJoint();
    JointTransform transform{};
    MAF_Helper::GetTransform(mJoint, transform);

    
    // NAME
    //
    int nameLength = strlen(joint.name.asUTF8());
    file.write(reinterpret_cast<char*>(&nameLength),               sizeof(int));
    file.write(reinterpret_cast<const char*>(joint.name.asUTF8()), sizeof(char) * nameLength); // @note check this ones

    // IDs
    //
    int ownId  = joint.influenceID + 1;
    int parent = joint.parentID    + 1;
    file.write(reinterpret_cast<char*>(&ownId),  sizeof(int));
    file.write(reinterpret_cast<char*>(&parent), sizeof(int));

    // Childrens
    //
    int childCount = joint.childrenIDs.size();
    file.write(reinterpret_cast<char*>(&childCount), sizeof(int));

    for (unsigned int cI = 0; cI < childCount; cI++)
    {
        int idx = joint.childrenIDs[cI] + 1;
        file.write(reinterpret_cast<char*>(&idx), sizeof(int));
    }

    // Transform
    //
    float pX = (float)transform.position.x;
    float pY = (float)transform.position.y;
    float pZ = (float)transform.position.z;
    file.write(reinterpret_cast<char*>(&pX), sizeof(float));
    file.write(reinterpret_cast<char*>(&pY), sizeof(float));
    file.write(reinterpret_cast<char*>(&pZ), sizeof(float));

    float rX = (float)transform.rotation.x;
    float rY = (float)transform.rotation.y;
    float rZ = (float)transform.rotation.z;
    float rW = (float)transform.rotation.w;
    file.write(reinterpret_cast<char*>(&rX), sizeof(float));
    file.write(reinterpret_cast<char*>(&rY), sizeof(float));
    file.write(reinterpret_cast<char*>(&rZ), sizeof(float));
    file.write(reinterpret_cast<char*>(&rW), sizeof(float));

    float sX = (float)transform.scale.x;
    float sY = (float)transform.scale.y;
    float sZ = (float)transform.scale.z;
    file.write(reinterpret_cast<char*>(&sX), sizeof(float));
    file.write(reinterpret_cast<char*>(&sY), sizeof(float));
    file.write(reinterpret_cast<char*>(&sZ), sizeof(float));

    float shX = (float)transform.shear.x;
    float shY = (float)transform.shear.y;
    float shZ = (float)transform.shear.z;
    file.write(reinterpret_cast<char*>(&shX), sizeof(float));
    file.write(reinterpret_cast<char*>(&shY), sizeof(float));
    file.write(reinterpret_cast<char*>(&shZ), sizeof(float));            
}


void MOF_Generator::WriteRoot(std::ofstream& file, Root& root)
{    
    MFnIkJoint rootJnt(root.rootObj);
    JointTransform transform{};
    MAF_Helper::GetTransform(rootJnt, transform);
       
    // NAME
    //
    int nameLength = strlen(rootJnt.name().asUTF8());
    file.write(reinterpret_cast<char*>(&nameLength),                sizeof(int));
    file.write(reinterpret_cast<const char*>(rootJnt.name().asUTF8()), sizeof(char) * nameLength); // @note check this ones

    // IDs
    //
    int ids = 0;
    file.write(reinterpret_cast<char*>(&ids), sizeof(int));
    file.write(reinterpret_cast<char*>(&ids), sizeof(int));

    // Childrens
    //
    int childCount = root.childrenIDs.size();
    file.write(reinterpret_cast<char*>(&childCount), sizeof(int));

    for (unsigned int cI = 0; cI < childCount; cI++)
    {
        int idx = root.childrenIDs[cI] + 1;
        file.write(reinterpret_cast<char*>(&idx), sizeof(int));        
    }

    // Transform
    //
    float pX = (float)transform.position.x;
    float pY = (float)transform.position.y;
    float pZ = (float)transform.position.z;
    file.write(reinterpret_cast<char*>(&pX), sizeof(float));
    file.write(reinterpret_cast<char*>(&pY), sizeof(float));
    file.write(reinterpret_cast<char*>(&pZ), sizeof(float));

    float rX = (float)transform.rotation.x;
    float rY = (float)transform.rotation.y;
    float rZ = (float)transform.rotation.z;
    float rW = (float)transform.rotation.w;
    file.write(reinterpret_cast<char*>(&rX), sizeof(float));
    file.write(reinterpret_cast<char*>(&rY), sizeof(float));
    file.write(reinterpret_cast<char*>(&rZ), sizeof(float));
    file.write(reinterpret_cast<char*>(&rW), sizeof(float));

    float sX = (float)transform.scale.x;
    float sY = (float)transform.scale.y;
    float sZ = (float)transform.scale.z;
    file.write(reinterpret_cast<char*>(&sX), sizeof(float));
    file.write(reinterpret_cast<char*>(&sY), sizeof(float));
    file.write(reinterpret_cast<char*>(&sZ), sizeof(float));

    float shX = (float)transform.shear.x;
    float shY = (float)transform.shear.y;
    float shZ = (float)transform.shear.z;
    file.write(reinterpret_cast<char*>(&shX), sizeof(float));
    file.write(reinterpret_cast<char*>(&shY), sizeof(float));
    file.write(reinterpret_cast<char*>(&shZ), sizeof(float));
    
}