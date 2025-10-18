#pragma once

#include <maya/MPoint.h>
#include <maya/MColor.h>
#include <maya/MFloatVector.h>
#include <maya/MFnIkJoint.h>

#include <cmath>

enum AnimationGatheringInformation
{
    JOINT_HIERARCHY,
    JOINT_TRANSFORMATION_OVER_THE_TIMELINE,
    BOTH,
};

enum Type 
{
    Animated,
    Static,
};

struct JointTransform
{
    MVector        position;
    MEulerRotation eulerRotation;
    MQuaternion    rotation;
    MVector        scale;
    MVector        shear;
};

struct Root
{
    MObject          rootObj;
    std::vector<int> childrenIDs;
};

struct Joint
{
    // @note Just an int. It needs to have -1 if the parent bone is the root
    int              parentID;    
    int              influenceID;
    std::vector<int> childrenIDs;
    MString name;

    // @note The positioning inside the vector is directly related to the frame index
    // so idx n = frame n
    //
    std::vector<JointTransform> transformPerFrame; 
    

    // @note Don't really like doing it this way, but I cant store a vector of MFnIkJoints
    //
    MFnIkJoint GetThisJoint()           { return ownDagPath;      }    
    MFnIkJoint GetChild(unsigned int i) { return childrenObjs[i]; }

    void SetDagPath(MDagPath dagPath) 
    {
        MFnIkJoint jnt(dagPath);
        
        name       = jnt.name();
        ownDagPath = dagPath;                 
    }

    void AddChild(MObject child) { childrenObjs.emplace_back(child); }

    MDagPath             ownDagPath;
    std::vector<MObject> childrenObjs;
};


struct Vertex 
{
    MPoint       position;
    MColor       color;
    MFloatVector normal;
    float        u;
    float        v;
    int          jointID[4];               // JointID and Weights are related, so the first value of weigts (The X) corresponds to the index stored in the X component of the vec4
    float        weight [4];

    

    // I'll use this to assign weights as the vertices that I'll retrieve from the Geoemtry iterator only have positions
    // and if the vertices match, then I'll asign them the ID's and weights 
    //
    bool Compare(Vertex& other)
    {
        if (position.isEquivalent(other.position, 1e-6)) { return true; }
                        
        return false;
    }

    
    bool operator == (const Vertex& other) const 
    {
        return position.isEquivalent(other.position, 1e-6) &&
               fabs(color.r - other.color.r)       < 1e-6f &&
               fabs(color.g - other.color.g)       < 1e-6f &&
               fabs(color.b - other.color.b)       < 1e-6f &&               
               normal.isEquivalent(other.normal,     1e-6) &&
               fabs(u - other.u)                   < 1e-6f &&
               fabs(v - other.v)                   < 1e-6f;                                
    }
};


namespace std
{
    template<> 
    struct std::hash<Vertex> 
    {
        std::size_t operator()(const Vertex& v) const
        {        
            auto customHash = [](size_t& seed, size_t value) 
            {
                seed ^= value + 1954435769U + (seed << 6) + (seed >> 2);
            };
    
            std::size_t seed = 0;
    
            customHash(seed, std::hash<double>{}(v.position.x));
            customHash(seed, std::hash<double>{}(v.position.y));
            customHash(seed, std::hash<double>{}(v.position.z));
    
            customHash(seed, std::hash<float>{}(v.color.r));
            customHash(seed, std::hash<float>{}(v.color.g));
            customHash(seed, std::hash<float>{}(v.color.b));
    
            customHash(seed, std::hash<float>{}(v.normal.x));
            customHash(seed, std::hash<float>{}(v.normal.y));
            customHash(seed, std::hash<float>{}(v.normal.z));
    
            customHash(seed, std::hash<float>{}(v.u));
            customHash(seed, std::hash<float>{}(v.v));
    
            return seed;
        }
    };
}