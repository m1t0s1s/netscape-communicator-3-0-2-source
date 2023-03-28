#ifndef _MoreMixedMode_
#define _MoreMixedMode_

/*
	In a .h file, global (procedure declaration):
	extern RoutineDescriptor 	foo_info;
	PROCDECL(extern,foo);
	
	In the .h file, class scoped (member declaration):
	static RoutineDescriptor 	foo_info;
	PROCDECL(static,foo);
	
	In the .cp file, global (procedure definition):
	RoutineDescriptor 			foo_info = RoutineDescriptor (foo);
	PROCEDURE(foo,uppFooType);
	
	In the .cp file, class scoped (member definition):
	RoutineDescriptor 			Foo::foo_info = RoutineDescriptor (Bar::foo);
	PROCDEF(Foo::,Bar::,foo,uppFooType);
	
	References to the function:
		foo_info
		PROCPTR(foo)
*/

#include "MixedMode.h"
#if GENERATINGCFM
#	define PROCPTR(NAME)			NAME##_info
#	define PROCDECL(MOD,NAME)		MOD RoutineDescriptor PROCPTR(NAME);
#	define PROCDEF(S1,S2,NAME,INFO)	RoutineDescriptor S1##PROCPTR(NAME) = BUILD_ROUTINE_DESCRIPTOR (INFO, S2##NAME);
#	define PROCEDURE(NAME,INFO)		RoutineDescriptor PROCPTR(NAME) = BUILD_ROUTINE_DESCRIPTOR (INFO, NAME);
#else
#	define PROCPTR(NAME)			NAME
#	define PROCDECL(MOD,NAME)
#	define PROCDEF(S1,S2,NAME,INFO)
#	define PROCEDURE(NAME,INFO)	
#endif

#endif // _MoreMixedMode_

