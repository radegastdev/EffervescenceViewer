/** 
 * @file llfloatercustomize.h
 * @brief The customize avatar floater, triggered by "Appearance..."
 *
 * $LicenseInfo:firstyear=2002&license=viewergpl$
 * 
 * Copyright (c) 2002-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at
 * http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#ifndef LL_LLFLOATERCUSTOMIZE_H
#define LL_LLFLOATERCUSTOMIZE_H

#include <map>

#include "llfloater.h"
#include "llstring.h"
#include "v3dmath.h"
#include "lltimer.h"
#include "llundo.h"
#include "llviewermenu.h"
#include "llwearable.h"
#include "lliconctrl.h"
#include "llsingleton.h"

class LLButton;
class LLIconCtrl;
class LLColorSwatchCtrl;
class LLGenePool;
class LLInventoryObserver;
class LLJoint;
class LLLineEditor;
class LLMakeOutfitDialog;
class LLRadioGroup;
class LLScrollContainer;
class LLScrollingPanelList;
class LLTabContainer;
class LLTextBox;
class LLTextureCtrl;
class LLViewerJointMesh;
class LLViewerVisualParam;
class LLVisualParam;
class LLVisualParamReset;
class LLViewerWearable;
class LLWearableSaveAsDialog;
class LLPanelEditWearable;
class AIFilePicker;

/////////////////////////////////////////////////////////////////////
// LLFloaterCustomize

class LLFloaterCustomize : public LLFloater, public LLSingleton<LLFloaterCustomize>
{
public:
	// Ctor/Dtor
	LLFloaterCustomize();
	virtual ~LLFloaterCustomize();

	// Inherted methods
	/*virtual*/ BOOL 	postBuild();
	/*virtual*/ void	onClose(bool app_quitting);
	/*virtual*/ void	draw();
	
	// Creation procedures
	static void		editWearable(LLViewerWearable* wearable, bool disable_camera_switch);
	static void		show();


private:
	// Initilization	
	void			initWearablePanels();
	void			initScrollingPanelList();

	// Deinitilization
	void			delayedClose(bool proceed, bool app_quitting);

	// Setters/Getters
	void			setCurrentWearableType(LLWearableType::EType type, bool disable_camera_switch);
public:
	LLWearableType::EType getCurrentWearableType()	const	{ return mCurrentWearableType; }
	LLPanelEditWearable* getCurrentWearablePanel()	const	{ return mWearablePanelList[ mCurrentWearableType ]; }
	LLScrollingPanelList* getScrollingPanelList()	const	{ return mScrollingPanelList; }	//LLPanelEditWearable needs access to this.

	// Updates
	void			wearablesChanged(LLWearableType::EType type);
public:
	void			updateScrollingPanelList();
	void			updateVisiblity(bool force_disable_camera_switch = false);
private:
	void			updateInventoryUI();

	// Utility
	void			fetchInventory();
	bool			isWearableDirty() const;

public:
	void			askToSaveIfDirty( boost::function<void (BOOL)> cb );
	void			saveCurrentWearables();
	void			switchToDefaultSubpart();

private:
	// Callbacks
	void			onBtnOk();
	void			onBtnMakeOutfit();
	void			onBtnImport();
	static void		onBtnImport_continued(AIFilePicker* filepicker);
	void			onBtnExport();
	static void		onBtnExport_continued(AIFilePicker* filepicker);
	void			onTabChanged( const LLSD& param );
	bool			onTabPrecommit( LLUICtrl* ctrl, const LLSD& param );
	bool			onSaveDialog(const LLSD& notification, const LLSD& response);
	void			onCommitChangeTab(BOOL proceed, LLTabContainer* ctrl, std::string panel_name, LLWearableType::EType type);
	
	// LLCallbackMap callback.
	static void*	createWearablePanel(void* userdata);

	// Member variables
	LLPanelEditWearable*			mWearablePanelList[ LLWearableType::WT_COUNT ];

	LLWearableType::EType			mCurrentWearableType;

	LLScrollingPanelList*			mScrollingPanelList;
	LLScrollContainer*		mScrollContainer;
	LLPointer<LLVisualParamReset>	mResetParams;

	LLInventoryObserver*			mInventoryObserver;

	boost::signals2::signal<void (bool proceed)> mNextStepAfterSaveCallback;
};

#endif  // LL_LLFLOATERCUSTOMIZE_H
