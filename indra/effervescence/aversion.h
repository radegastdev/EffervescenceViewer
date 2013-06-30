/* Copyright (C) 2012 Apelsin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301 USA */

#ifndef AVERSION_H
#define AVERSION_H

#include "llcontrol.h"
#include "llversionviewer.h"

extern const LLCachedControl<std::string>	*gVersionChannelCC;
extern const LLCachedControl<U32>			*gVersionMajorCC;
extern const LLCachedControl<U32>			*gVersionMinorCC;
extern const LLCachedControl<U32>			*gVersionPatchCC;
extern const LLCachedControl<U32>			*gVersionBuildCC;

extern const S32 gVersionFlat;	// Fun Bun flat version

class aVersion
{
public:
	static void init();
	template <class T> static T CCTryGet(const LLCachedControl<T> *cc, const T alt)
	{
		return cc ? cc->get() : alt;
	}
};

// Well...if I've never made a hack my whole life, I've made one now for sure:
#define	gVersionChannel				aVersion::CCTryGet(gVersionChannelCC, std::string(LL_CHANNEL)).c_str()
#define	gVersionMajor		(S32)	aVersion::CCTryGet(gVersionMajorCC, (U32)LL_VERSION_MAJOR)
#define	gVersionMinor		(S32)	aVersion::CCTryGet(gVersionMinorCC, (U32)LL_VERSION_MINOR)
#define	gVersionPatch		(S32)	aVersion::CCTryGet(gVersionPatchCC, (U32)LL_VERSION_PATCH)
#define	gVersionBuild		(S32)	aVersion::CCTryGet(gVersionBuildCC, (U32)LL_VERSION_BUILD)

#if LL_DARWIN
extern const char* gVersionBundleID;
#endif

#endif