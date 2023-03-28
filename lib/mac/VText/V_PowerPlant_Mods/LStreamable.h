// ===========================================================================
//	LStreamable.h					©1995 Metrowerks Inc. All rights reserved.
// ===========================================================================

#ifndef _H_LStreamable
#define _H_LStreamable

#ifdef __MWERKS__
#	pragma once
#endif

// project headers
#include <UTypeInfo.h>


// forward class & structure declarations
class	LStreamable;
class	LStream;


class LStreamable 
{
	PP_DECLARE_TYPEINFO(LStreamable)
	
public:
						LStreamable();
						LStreamable(const LStreamable& inStreamable);
	virtual				~LStreamable();
	
	virtual ClassIDT	GetClassID() const = 0;
	virtual void		WriteObject(LStream &inStream) const;
	virtual void		ReadObject(LStream &inStream);
};


//	Inline function definitions

inline	LStreamable::LStreamable()										{ }
inline	LStreamable::LStreamable(const LStreamable& /* inStreamable */)	{ }


//	¥ Macros

//	Abstract streamable classes

#define PP_DECLARE_ABSTRACT_STREAMABLE(name)						\
	PP_DECLARE_TYPEINFO(name)										\
	public:															\
		virtual void		WriteObject(LStream &inStream) const;	\
		virtual void		ReadObject(LStream &inStream);

#define PP_DEFINE_ABSTRACT_STREAMABLE_1_BASE(name, base1)			\
	PP_DEFINE_TYPEINFO_1_BASE(name, base1)

#define PP_DEFINE_ABSTRACT_STREAMABLE_2_BASE(name, base1, base2)	\
	PP_DEFINE_TYPEINFO_2_BASE(name, base1, base2)

#define PP_DEFINE_ABSTRACT_STREAMABLE_3_BASE(name, base1, base2, base3)	\
	PP_DEFINE_TYPEINFO_3_BASE(name, base1, base2, base3)

#define PP_DEFINE_ABSTRACT_STREAMABLE_4_BASE(name, base1, base2, base3, base4)	\
	PP_DEFINE_TYPEINFO_4_BASE(name, base1, base2, base3, base4)

#define PP_DEFINE_ABSTRACT_STREAMABLE_5_BASE(name, base1, base2, base3, base4, base5)	\
	PP_DEFINE_TYPEINFO_5_BASE(name, base1, base2, base3, base4, base5)


//	Ordinary streamable classes

#define PP_DECLARE_STREAMABLE(name, classID)						\
	PP_DECLARE_ABSTRACT_STREAMABLE(name)							\
	enum {class_ID = classID};										\
	static LStreamable*	Create##name();								\
	virtual ClassIDT	GetClassID() const	{ return (name::class_ID); }

#define PP_DEFINE_STREAMABLE_1_BASE(name, base1)					\
	PP_DEFINE_TYPEINFO_1_BASE(name, base1)							\
	_PP_DEFINE_COMMON_STREAMABLE(name)

#define PP_DEFINE_STREAMABLE_2_BASE(name, base1, base2)				\
	PP_DEFINE_TYPEINFO_2_BASE(name, base1, base2)					\
	_PP_DEFINE_COMMON_STREAMABLE(name)

#define PP_DEFINE_STREAMABLE_3_BASE(name, base1, base2, base3)		\
	PP_DEFINE_TYPEINFO_3_BASE(name, base1, base2, base3)			\
	_PP_DEFINE_COMMON_STREAMABLE(name)

#define PP_DEFINE_STREAMABLE_4_BASE(name, base1, base2, base3, base4)	\
	PP_DEFINE_TYPEINFO_4_BASE(name, base1, base2, base3, base4)			\
	_PP_DEFINE_COMMON_STREAMABLE(name)

#define PP_DEFINE_STREAMABLE_5_BASE(name, base1, base2, base3, base4, base5)	\
	PP_DEFINE_TYPEINFO_5_BASE(name, base1, base2, base3, base4, base5)			\
	_PP_DEFINE_COMMON_STREAMABLE(name)

#define PP_REGISTER_STREAMABLE(name)								\
	URegistrar::RegisterStreamable(name::class_ID, name::Create##name)


//	Abstract hybrid classes

#define PP_DECLARE_ABSTRACT_HYBRID(name)							\
	PP_DECLARE_ABSTRACT_STREAMABLE(name)

#define PP_DEFINE_ABSTRACT_HYBRID_1_BASE(name, base1)				\
	PP_DEFINE_TYPEINFO_1_BASE(name, base1)

#define PP_DEFINE_ABSTRACT_HYBRID_2_BASE(name, base1, base2)		\
	PP_DEFINE_TYPEINFO_2_BASE(name, base1, base2)

#define PP_DEFINE_ABSTRACT_HYBRID_3_BASE(name, base1, base2, base3)	\
	PP_DEFINE_TYPEINFO_3_BASE(name, base1, base2, base3)

#define PP_DEFINE_ABSTRACT_HYBRID_4_BASE(name, base1, base2, base3, base4)	\
	PP_DEFINE_TYPEINFO_4_BASE(name, base1, base2, base3, base4)

#define PP_DEFINE_ABSTRACT_HYBRID_5_BASE(name, base1, base2, base3, base4, base5)	\
	PP_DEFINE_TYPEINFO_5_BASE(name, base1, base2, base3, base4, base5)


//	Ordinary hybrid classes

#define PP_DECLARE_HYBRID(name, classID)																	\
	PP_DECLARE_STREAMABLE(name, classID)																\
	static LStreamable	*Create##name##Stream(LStream *inStream);

#define PP_DEFINE_HYBRID_1_BASE(name, base1)						\
	PP_DEFINE_TYPEINFO_1_BASE(name, base1)							\
	_PP_DEFINE_COMMON_HYBRID(name)
	
#define PP_DEFINE_HYBRID_2_BASE(name, base1, base2)					\
	PP_DEFINE_TYPEINFO_2_BASE(name, base1, base2)					\
	_PP_DEFINE_COMMON_HYBRID(name)

#define PP_DEFINE_HYBRID_3_BASE(name, base1, base2, base3)			\
	PP_DEFINE_TYPEINFO_3_BASE(name, base1, base2, base3)			\
	_PP_DEFINE_COMMON_HYBRID(name)

#define PP_DEFINE_HYBRID_4_BASE(name, base1, base2, base3, base4)	\
	PP_DEFINE_TYPEINFO_4_BASE(name, base1, base2, base3, base4)		\
	_PP_DEFINE_COMMON_HYBRID(name)

#define PP_DEFINE_HYBRID_5_BASE(name, base1, base2, base3, base4, base5)	\
	PP_DEFINE_TYPEINFO_5_BASE(name, base1, base2, base3, base4, base5)		\
	_PP_DEFINE_COMMON_HYBRID(name)

#define PP_REGISTER_HYBRID(name)									\
	URegistrar::RegisterStreamable(name::class_ID, name::Create##name, name::Create##name##Stream)


//	Internal macros

#define _PP_DEFINE_COMMON_STREAMABLE(name)							\
	LStreamable *name::Create##name()	{ return (new name); }

#define _PP_DEFINE_COMMON_HYBRID(name)								\
	_PP_DEFINE_COMMON_STREAMABLE(name)								\
	LStreamable *name::Create##name##Stream(LStream *inStream)		\
		{ return (new name(inStream)); }


#endif	// _H_LStreamable
