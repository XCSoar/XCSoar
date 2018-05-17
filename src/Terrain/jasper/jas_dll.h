#ifndef JAS_DLL_H
#define JAS_DLL_H

#if defined(JAS_DLL)
	#if defined(_WIN32)
		#if defined(JAS_BUILDING_DLL)
			#define JAS_DLLEXPORT __declspec(dllexport)
		#else
			#define JAS_DLLEXPORT __declspec(dllimport)
		#endif
		#define JAS_DLLLOCAL
	#elif defined(JAS_HAVE_VISIBILITY)
		#if defined(JAS_BUILDING_DLL)
			#define JAS_DLLEXPORT __attribute__ ((visibility("default")))
			#define JAS_DLLLOCAL __attribute__ ((visibility("hidden")))
		#else
			#define JAS_DLLEXPORT
			#define JAS_DLLLOCAL
		#endif
	#else
		#define JAS_DLLEXPORT
		#define JAS_DLLLOCAL
	#endif
#else
	#define JAS_DLLEXPORT
	#define JAS_DLLLOCAL
#endif

#endif
