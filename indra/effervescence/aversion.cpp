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

#include "llviewerprecompiledheaders.h"
#include "llversionviewer.h"
#include "aversion.h"

const LLCachedControl<std::string>	*gVersionChannelCC;
const LLCachedControl<U32>			*gVersionMajorCC;
const LLCachedControl<U32>			*gVersionMinorCC;
const LLCachedControl<U32>			*gVersionPatchCC;
const LLCachedControl<U32>			*gVersionBuildCC;

//const S32 gVersionFlat = LL_VERSION_FLAT; // FB flat version

void aVersion::init()
{
	gVersionChannelCC =	new LLCachedControl<std::string>(gSavedSettings,	"SpecifiedChannel");
	gVersionMajorCC =	new LLCachedControl<U32>		(gSavedSettings,	"SpecifiedVersionMaj");
	gVersionMinorCC =	new LLCachedControl<U32>		(gSavedSettings,	"SpecifiedVersionMin");
	gVersionPatchCC =	new LLCachedControl<U32>		(gSavedSettings,	"SpecifiedVersionPatch");
	gVersionBuildCC =	new LLCachedControl<U32>		(gSavedSettings,	"SpecifiedVersionBuild");
}

#if LL_DARWIN
const char* gVersionBundleID = LL_VERSION_BUNDLE_ID;
#endif
