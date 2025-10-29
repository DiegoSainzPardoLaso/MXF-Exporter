#include "Skinner.h"


// @note Influences and weights are retrieved in an ordered way.
// So, to confirm this I have a 12 vertices mesh drived by 5 influence objects (joints)
// If I open the 'Component Editor' and go to the smooth Skins tab this is what I see (I need to be in vertex mode to load the components).
//           joint_1     joint_2     joint_2_end     joint_3     joint_3_end     
// Vtx [0]   0.74010     0.15400     0.010000000     0.09500     0.000000000
// Vtx [1]   .......     .......     ...........     .......     ...........   
// Vtx [2]   .......     .......     ...........     .......     ...........   
// Vtx [3]   .......     .......     ...........     .......     ...........   
// Vtx [4]   .......     .......     ...........     .......     ...........   
// Vtx [5]   .......     .......     ...........     .......     ...........   
// Vtx [6]   .......     .......     ...........     .......     ...........   
// Vtx [7]   .......     .......     ...........     .......     ...........   
// Vtx [8]   .......     .......     ...........     .......     ...........   
// Vtx [9]   .......     .......     ...........     .......     ...........   
// Vtx [10]  .......     .......     ...........     .......     ...........   
// Vtx [11]  0.00000     0.37800     0.378000000     0.12200     0.122000000
//
// If I compare the 'Component Editor' table to the data I gather using this function the results are identical.
// So the plan here is to gather the positions, (No need for the normals, if the positions are the same, the weights should 
// be applied to that vertex even if the normals arent the same)  it's weights, and the influence name.
// positions and normals will be used to get a hash to compare witht the deduplicate vertices list.
//
// Weights and influence names will be used to generate a joint ID's table and assign the weights or do i Have the influence idx? maybe? 
// 
// @note --- Remember that the MAXIMUM INFLUENCES PER VERTEX HAS TO BE 4 AT MAX
MStatus Skinner::FindMeshWeightsAndInfluences(MDagPath dagPath, std::vector<Vertex>& verticesWithWeightsAndIDs)
{    
    MStatus             status = MStatus::kSuccess;
    MString             info   = "";

    MObject             skinCluster = FindSkinCluster(dagPath);
    MFnSkinCluster      skinClusterFn(skinCluster, &status);
    MDagPathArray       influences;
    unsigned int        influenceCount = skinClusterFn.influenceObjects(influences);
    
    if (influenceCount == 0) 
    {
        status = MS::kFailure;
        Print("No influence objects found.", status);
        return status;
    }
    
    unsigned int nGeoms = skinClusterFn.numOutputConnections();      

    for (unsigned int geometryIdx = 0; geometryIdx < nGeoms; ++geometryIdx) 
    {
        unsigned int index = skinClusterFn.indexForOutputConnection(geometryIdx, &status);
        if (status == MStatus::kFailure) { MGlobal::displayError("Failed to get geometry index"); }


        MDagPath skinPath;
        status = skinClusterFn.getPathAtIndex(index, skinPath);
        
        if (status == MStatus::kFailure) 
        {
            MGlobal::displayError("Error getting geometry path"); 
            return status;
        }
        

        // iterate through the components of this geometry        
        MItGeometry geometryIter(skinPath);

        for (; !geometryIter.isDone(); geometryIter.next()) 
        {
            Vertex vert{};
            vert.position = geometryIter.position(MSpace::kObject);            
            MObject comp  = geometryIter.currentItem(&status);
            if (status == MStatus::kFailure) { MGlobal::displayError("Failed to get component"); }        
         
            // Get the weights for this vertex (one per influence object)             
            MDoubleArray wts;
            unsigned int infCount;
            status = skinClusterFn.getWeights(skinPath, comp, wts, infCount);
                      
            if (status == MStatus::kFailure) { MGlobal::displayError("Failed to retrieve the weights"); }        

            if (0 == infCount) 
            {
                status = MS::kFailure;                
                info   = "Error: 0 influence objects atached to vertex number ["; info += geometryIter.index(); info += "]";
                MGlobal::displayError(info);
            }
                    
            // IDs and weights assignations
            int internalDWeightIdx = 0;
            for (unsigned int InfluenceIdx = 0; InfluenceIdx < infCount; InfluenceIdx++)
            {              
                if (wts[InfluenceIdx] > 1.0e-6)
                {
                    vert.jointID[internalDWeightIdx] = InfluenceIdx;
                    vert.weight [internalDWeightIdx] = wts[InfluenceIdx];
                    internalDWeightIdx++;
                }

                // info = "Value of ["; info += InfluenceIdx; info += "] = ["; info += wts[InfluenceIdx]; info += "]";
                // MGlobal::displayInfo(info);
            }

            verticesWithWeightsAndIDs.emplace_back(vert);
        }
    }
   
    return status;
}


MObject Skinner::FindSkinCluster(MDagPath& dagPath)
{    
    MObject            skinCluster;
    MObject            geomNode = dagPath.node();
    MItDependencyGraph dgIt(geomNode, MFn::kSkinClusterFilter, MItDependencyGraph::kUpstream);

    if (!dgIt.isDone()) { skinCluster = dgIt.currentItem(); }

    return skinCluster;
}
 

bool Skinner::IsSkinClusterIncluded(MObjectArray& skinClusterArray, MObject& node)
{
    if (skinClusterArray.length() == 0) return true;

    for (unsigned int i = 0; i < skinClusterArray.length(); i++)
    {
        if (skinClusterArray[i] == node) { return true; }
    }

    return false;
}

 
 