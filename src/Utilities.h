#pragma once

#include <stdarg.h>
#include <maya/MGlobal.h>

#include <maya/MTime.h>

template<typename T>
inline void Print(T t)
{
	MString info = "Message: ";
	info += t;
	MGlobal::displayInfo(info);
}

template<typename T>
inline void Print(MString message, T t)
{	
	message += t;
	MGlobal::displayInfo(message);
}

inline void Print(MString message, float a, float b, float c)
{
	message += a;
	message += ", ";
	message += b;
	message += ", ";
	message += c;
	MGlobal::displayInfo(message);
}


inline MStatus Print(MString message, MStatus& status)
{
	switch (status)
	{
		case MStatus::kSuccess: { MGlobal::displayInfo(message);  } break;
		case MStatus::kFailure: { MGlobal::displayError(message); } break;

		default: { MGlobal::displayInfo(message); } break;
	}
	
	return status;
}

inline MStatus Status(MString message, MStatus status)
{
	if (status == MStatus::kFailure) { MGlobal::displayError(message); }
	if (status == MStatus::kSuccess) { MGlobal::displayInfo(message); }

	return status;
}

// @note Use MTime::Unit::uUnit();
inline double GetFrameRate()
{
	MTime::Unit unit;
	unit = MTime::uiUnit();
	double frameRate = 0.0f;

	switch (unit) 
	{
		case MTime::k2FPS:		{ frameRate = 2.0f;     } break;
		case MTime::k3FPS:		{ frameRate = 3.0f;     } break;
		case MTime::k4FPS:		{ frameRate = 4.0f;     } break;
		case MTime::k5FPS:		{ frameRate = 5.0f;     } break;
		case MTime::k6FPS:		{ frameRate = 6.0f;     } break;
		case MTime::k8FPS:		{ frameRate = 8.0f;     } break;
		case MTime::k10FPS:		{ frameRate = 10.0f;    } break;
		case MTime::k12FPS:		{ frameRate = 12.0f;    } break;
		case MTime::kGames:		{ frameRate = 15.0f;    } break;		
		case MTime::k16FPS:		{ frameRate = 16.0f;    } break;		
		case MTime::k20FPS:		{ frameRate = 20.0f;    } break;
		case MTime::k23_976FPS:	{ frameRate = 23.976f;  } break;
		case MTime::kFilm:		{ frameRate = 24.0f;    } break;				
		case MTime::kPALFrame:  { frameRate = 25.0f;    } break;
		case MTime::k29_97FPS:  { frameRate = 29.97f;   } break;
		case MTime::k29_97DF:   { frameRate = 29.97f;   } break;
		case MTime::kNTSCFrame: { frameRate = 30.0f;    } break;
		case MTime::k40FPS:     { frameRate = 40.0f;    } break;
		case MTime::k47_952FPS: { frameRate = 47.952f;  } break;
		case MTime::kShowScan:  { frameRate = 48.0f;    } break;
		case MTime::kPALField:  { frameRate = 50.0f;    } break;
		case MTime::k59_94FPS:  { frameRate = 59.94f;   } break;		
		case MTime::kNTSCField: { frameRate = 60.0f;    } break;
		case MTime::k75FPS:     { frameRate = 75.0f;    } break;
		case MTime::k80FPS:     { frameRate = 80.0f;    } break;
		case MTime::k90FPS:     { frameRate = 90.0f;    } break;

		case MTime::k100FPS:    { frameRate = 100.0f;   } break;
		case MTime::k119_88FPS: { frameRate = 119.88f;  } break;
		case MTime::k120FPS:    { frameRate = 120.0f;   } break;
		case MTime::k125FPS:    { frameRate = 125.0f;   } break;
		case MTime::k150FPS:    { frameRate = 150.0f;   } break;

		case MTime::k200FPS:    { frameRate = 200.0f;   } break;
		case MTime::k240FPS:    { frameRate = 240.0f;   } break;
		case MTime::k250FPS:    { frameRate = 250.0f;   } break;

		case MTime::k300FPS:    { frameRate = 300.0f;   } break;
		case MTime::k375FPS:    { frameRate = 375.0f;   } break;

		case MTime::k400FPS:    { frameRate = 400.0f;   } break;
		case MTime::k500FPS:    { frameRate = 500.0f;   } break;
		case MTime::k600FPS:    { frameRate = 600.0f;   } break;

		case MTime::k750FPS:    { frameRate = 750.0f;   } break;

		case MTime::k1200FPS:   { frameRate = 1200.0f;  } break;
		case MTime::k1500FPS:   { frameRate = 1500.0f;  } break;
		case MTime::k2000FPS:   { frameRate = 2000.0f;  } break;

		case MTime::k3000FPS:   { frameRate = 3000.0f;  } break;
		case MTime::k6000FPS:   { frameRate = 6000.0f;  } break;
		case MTime::k44100FPS:  { frameRate = 44100.0f; } break;
		case MTime::k48000FPS:  { frameRate = 48000.0f; } break;
		
		default:				{ frameRate = 30.0f;    } break;
	}

	return frameRate;
}