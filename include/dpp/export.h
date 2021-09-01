#pragma once

#ifdef DPP_BUILD
	#ifdef _WIN32
		#define CoreExport __declspec(dllexport)
	#else
		#define CoreExport
	#endif
#else
	#ifdef _WIN32
		#define CoreExport __declspec(dllimport)
	#else
		#define CoreExport
	#endif
#endif