/*

	DebugLog

*/


// Log Level
enum {
	DL_LOW,
	DL_MEDIUM,
	DL_HIGH
};

// XXX Think more about the set of event
// Log Type
enum {
	DL_GENERAL,
	DL_CONFIG,
	DL_LOG,
	DL_INPUT

};


// What level do we want to compile in
#define DL_COMPILE_LEVEL DL_LOW

// What types do we want to compile in
#define DL_COMPILE_TYPE DL_GENERAl & DL_CONFIG & DL_LOG & DL_INPUT

/*

	Check level and type before calling real event, thus removing those

	Then call real function
*/


