/** 
 * @file llfloatercustomize.cpp
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

#include "llviewerprecompiledheaders.h"

#include "llimagejpeg.h"
#include "llfloatercustomize.h"
#include "llfontgl.h"
#include "llbutton.h"
#include "lliconctrl.h"
#include "llresmgr.h"
#include "llmorphview.h"
#include "llfloatertools.h"
#include "llagent.h"
#include "llagentwearables.h"
#include "lltoolmorph.h"
#include "llvoavatarself.h"
#include "llradiogroup.h"
#include "lltoolmgr.h"
#include "llviewermenu.h"
#include "llscrollcontainer.h"
#include "llscrollingpanelparam.h"
#include "llsliderctrl.h"
#include "llviewerwindow.h"
#include "llinventoryfunctions.h"
#include "llinventoryobserver.h"
#include "llinventoryicon.h"
#include "lltextbox.h"
#include "lllineeditor.h"
#include "llviewertexturelist.h"
#include "llfocusmgr.h"
#include "llviewerwindow.h"
#include "llviewercamera.h"
#include "llappearance.h"
#include "imageids.h"
#include "llassetstorage.h"
#include "lltexturectrl.h"
#include "lltextureentry.h"
#include "llwearablelist.h"
#include "llviewerinventory.h"
#include "lldbstrings.h"
#include "llcolorswatch.h"
#include "llglheaders.h"
#include "llui.h"
#include "llviewermessage.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llnotificationsutil.h"
#include "llpaneleditwearable.h"
#include "llmakeoutfitdialog.h"
#include "llagentcamera.h"
#include "llappearancemgr.h"

#include "statemachine/aifilepicker.h"

using namespace LLAvatarAppearanceDefines;

// *TODO:translate : The ui xml for this really needs to be integrated with the appearance paramaters

/////////////////////////////////////////////////////////////////////
// LLFloaterCustomizeObserver

class LLFloaterCustomizeObserver : public LLInventoryObserver
{
public:
	LLFloaterCustomizeObserver(LLFloaterCustomize* fc) : mFC(fc) {}
	virtual ~LLFloaterCustomizeObserver() {}
	virtual void changed(U32 mask) { mFC->getCurrentWearablePanel()->updateScrollingPanelUI(); }
protected:
	LLFloaterCustomize* mFC;
};

////////////////////////////////////////////////////////////////////////////

// Local Constants 

BOOL edit_wearable_for_teens(LLWearableType::EType type)
{
	switch(type)
	{
	case LLWearableType::WT_UNDERSHIRT:
	case LLWearableType::WT_UNDERPANTS:
		return FALSE;
	default:
		return TRUE;
	}
}

////////////////////////////////////////////////////////////////////////////

void updateAvatarHeightDisplay()
{
	if (LLFloaterCustomize::instanceExists() && isAgentAvatarValid())
	{
		F32 avatar_size = (gAgentAvatarp->mBodySize.mV[VZ]) + (F32)0.17; //mBodySize is actually quite a bit off.
		LLFloaterCustomize::getInstance()->getChild<LLTextBox>("HeightTextM")->setValue(llformat("%.2f", avatar_size) + "m");
		F32 feet = avatar_size / 0.3048;
		F32 inches = (feet - (F32)((U32)feet)) * 12.0;
		LLFloaterCustomize::getInstance()->getChild<LLTextBox>("HeightTextI")->setValue(llformat("%d'%d\"", (U32)feet, (U32)inches));
	}
 }

/////////////////////////////////////////////////////////////////////
// LLFloaterCustomize

struct WearablePanelData
{
	WearablePanelData(LLFloaterCustomize* floater, LLWearableType::EType type)
		: mFloater(floater), mType(type) {}
	LLFloaterCustomize* mFloater;
	LLWearableType::EType mType;
};

LLFloaterCustomize::LLFloaterCustomize()
:	LLFloater(std::string("customize")),
	mScrollingPanelList( NULL ),
	mInventoryObserver(NULL),
	mCurrentWearableType(LLWearableType::WT_INVALID)
{
	memset(&mWearablePanelList[0],0,sizeof(char*)*LLWearableType::WT_COUNT); //Initialize to 0

	gSavedSettings.setU32("AvatarSex", (gAgentAvatarp->getSex() == SEX_MALE) );

	mResetParams = new LLVisualParamReset();
	
	// create the observer which will watch for matching incoming inventory
	mInventoryObserver = new LLFloaterCustomizeObserver(this);
	gInventory.addObserver(mInventoryObserver);

	LLCallbackMap::map_t factory_map;
	const std::string &invalid_name = LLWearableType::getTypeName(LLWearableType::WT_INVALID);
	for(U32 type=LLWearableType::WT_SHAPE;type<LLWearableType::WT_INVALID;++type)
	{
		std::string name = LLWearableType::getTypeName((LLWearableType::EType)type);
		if(name != invalid_name)
		{
			name[0] = toupper(name[0]);
			factory_map[name] = LLCallbackMap(createWearablePanel, (void*)(new WearablePanelData(this, (LLWearableType::EType)type) ) );
		}
	}

	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_customize.xml", &factory_map);

	fetchInventory();	//May as well start requesting now.

	LLFloater::open();

	setVisible(false);
}

LLFloaterCustomize::~LLFloaterCustomize()
{
	llinfos << "Destroying LLFloaterCustomize" << llendl;
	mResetParams = NULL;
	gInventory.removeObserver(mInventoryObserver);
	delete mInventoryObserver;
}

// virtual
BOOL LLFloaterCustomize::postBuild()
{
	getChild<LLUICtrl>("Make Outfit")->setCommitCallback(boost::bind(&LLFloaterCustomize::onBtnMakeOutfit, this));
	getChild<LLUICtrl>("Save Outfit")->setCommitCallback(boost::bind(&LLAppearanceMgr::updateBaseOutfit, LLAppearanceMgr::getInstance()));
	getChild<LLUICtrl>("Ok")->setCommitCallback(boost::bind(&LLFloaterCustomize::onBtnOk, this));
	getChild<LLUICtrl>("Cancel")->setCommitCallback(boost::bind(&LLFloater::onClickClose, this));

    // reX
	getChild<LLUICtrl>("Import")->setCommitCallback(boost::bind(&LLFloaterCustomize::onBtnImport, this));
	getChild<LLUICtrl>("Export")->setCommitCallback(boost::bind(&LLFloaterCustomize::onBtnExport, this));
	
	// Tab container
	LLTabContainer* tab_container = getChild<LLTabContainer>("customize tab container");
	if(tab_container)
	{
		tab_container->setCommitCallback(boost::bind(&LLFloaterCustomize::onTabChanged, this, _2));
		tab_container->setValidateCallback(boost::bind(&LLFloaterCustomize::onTabPrecommit, this, _1, _2));
	}

	// Remove underwear panels for teens
	if (gAgent.isTeen())
	{
		if (tab_container)
		{
			LLPanel* panel = tab_container->getPanelByName("Undershirt");
			if (panel) tab_container->removeTabPanel(panel);
			panel = tab_container->getPanelByName("Underpants");
			if (panel) tab_container->removeTabPanel(panel);
		}
	}
	
	// Scrolling Panel
	initScrollingPanelList();
	
	return TRUE;
}

//static
void LLFloaterCustomize::editWearable(LLViewerWearable* wearable, bool disable_camera_switch)
{
	if(!wearable)
		return;
	LLFloaterCustomize::getInstance()->setCurrentWearableType(wearable->getType(), disable_camera_switch);
}

//static
void LLFloaterCustomize::show()
{
	if(!LLFloaterCustomize::instanceExists())
	{
		const BOOL disable_camera_switch = LLWearableType::getDisableCameraSwitch(LLWearableType::WT_SHAPE);
		LLFloaterCustomize::getInstance()->setCurrentWearableType(LLWearableType::WT_SHAPE, disable_camera_switch);
	}
	else
		LLFloaterCustomize::getInstance()->setFrontmost(true);
}

// virtual
void LLFloaterCustomize::onClose(bool app_quitting)
{
	// since this window is potentially staying open, push to back to let next window take focus
	gFloaterView->sendChildToBack(this);
	// askToSaveIfDirty will call delayedClose immediately if there's nothing to save.
	askToSaveIfDirty( boost::bind(&LLFloaterCustomize::delayedClose, this, _1, app_quitting) );
}

void LLFloaterCustomize::delayedClose(bool proceed, bool app_quitting)
{
	if(proceed)
	{
		LLVOAvatarSelf::onCustomizeEnd();
		LLFloater::onClose(app_quitting);
		if(gAgentAvatarp)gAgentAvatarp->mSpecialRenderMode = 0;
	}
}

////////////////////////////////////////////////////////////////////////////

void LLFloaterCustomize::setCurrentWearableType( LLWearableType::EType type, bool disable_camera_switch )
{
	if( mCurrentWearableType != type )
	{
		mCurrentWearableType = type; 

		if (!gAgentCamera.cameraCustomizeAvatar())
		{
			LLVOAvatarSelf::onCustomizeStart(disable_camera_switch);
		}
		else if(!gSavedSettings.getBOOL("AppearanceCameraMovement") || disable_camera_switch)	//Break out to free camera.
		{
			gAgentCamera.changeCameraToDefault();
			gAgentCamera.resetView();
		}

		S32 type_int = (S32)type;
		if(mWearablePanelList[type_int])
		{
			std::string panelname = mWearablePanelList[type_int]->getName();
			childShowTab("customize tab container", panelname);
			switchToDefaultSubpart();
		}

		updateVisiblity(disable_camera_switch);
	}
}

// reX: new function
void LLFloaterCustomize::onBtnImport()
{
	AIFilePicker* filepicker = AIFilePicker::create();
	filepicker->open(FFLOAD_XML);
	filepicker->run(boost::bind(&LLFloaterCustomize::onBtnImport_continued, filepicker));
}

void LLFloaterCustomize::onBtnImport_continued(AIFilePicker* filepicker)
{
	if (!filepicker->hasFilename())
	{
		// User canceled import.
		return;
	}

	const std::string filename = filepicker->getFilename();

	FILE* fp = LLFile::fopen(filename, "rb");

	//char text_buffer[2048];		/* Flawfinder: ignore */
	S32 c;
	S32 typ;
	S32 count;
	S32 param_id=0;
	F32 param_weight=0;
	S32 fields_read;

	for( S32 i=0; i < LLWearableType::WT_COUNT; i++ )
	{
		fields_read = fscanf( fp, "type %d\n", &typ);
		if( fields_read != 1 )
		{
			llwarns << "Bad asset type: early end of file" << llendl;
			return;
		}

		fields_read = fscanf( fp, "parameters %d\n", &count);
		if( fields_read != 1 )
		{
			llwarns << "Bad parameters : early end of file" << llendl;
			return;
		}
		for(c=0;c<count;c++)
		{
			fields_read = fscanf( fp, "%d %f\n", &param_id, &param_weight );
			if( fields_read != 2 )
			{
				llwarns << "Bad parameters list: early end of file" << llendl;
				return;
			}
			gAgentAvatarp->setVisualParamWeight( param_id, param_weight, TRUE);
			gAgentAvatarp->updateVisualParams();
		}
	}

	fclose(fp);
	return;
}

// reX: new function
void LLFloaterCustomize::onBtnExport()
{
	AIFilePicker* filepicker = AIFilePicker::create();
	filepicker->open("", FFSAVE_XML);
	filepicker->run(boost::bind(&LLFloaterCustomize::onBtnExport_continued, filepicker));
}

void LLFloaterCustomize::onBtnExport_continued(AIFilePicker* filepicker)
{
	if (!filepicker->hasFilename())
	{
		// User canceled export.
		return;
	}

	LLViewerInventoryItem* item;
	BOOL is_modifiable;

	const std::string filename = filepicker->getFilename();

	FILE* fp = LLFile::fopen(filename, "wb");

	for( S32 i=0; i < LLWearableType::WT_COUNT; i++ )
	{
		is_modifiable = FALSE;
		LLViewerWearable* old_wearable = gAgentWearables.getViewerWearable((LLWearableType::EType)i, 0);	// TODO: MULTI-WEARABLE
		if( old_wearable )
		{
			item = gInventory.getItem(old_wearable->getItemID());
			if(item)
			{
				const LLPermissions& perm = item->getPermissions();
				is_modifiable = perm.allowModifyBy(gAgent.getID(), gAgent.getGroupID());
			}
		}
		if (is_modifiable)
		{
			old_wearable->FileExportParams(fp);
		}
		if (!is_modifiable)
		{
			fprintf( fp, "type %d\n",i);
			fprintf( fp, "parameters 0\n");
		}
	}	

	for( S32 i=0; i < LLWearableType::WT_COUNT; i++ )
	{
		is_modifiable = FALSE;
		LLViewerWearable* old_wearable = gAgentWearables.getViewerWearable((LLWearableType::EType)i, 0);	// TODO: MULTI-WEARABLE
		if( old_wearable )
		{
			item = gInventory.getItem(old_wearable->getItemID());
			if(item)
			{
				const LLPermissions& perm = item->getPermissions();
				is_modifiable = perm.allowModifyBy(gAgent.getID(), gAgent.getGroupID());
			}
		}
		if (is_modifiable)
		{
			old_wearable->FileExportTextures(fp);
		}
		if (!is_modifiable)
		{
			fprintf( fp, "type %d\n",i);
			fprintf( fp, "textures 0\n");
		}
	}	

	fclose(fp);
}

void LLFloaterCustomize::onBtnOk()
{
	saveCurrentWearables();
	gFloaterView->sendChildToBack(this);
	close(false);
}

void LLFloaterCustomize::onBtnMakeOutfit()
{
	new LLMakeOutfitDialog(true); // LLMakeOutfitDialog deletes itself.
}

////////////////////////////////////////////////////////////////////////////

// static
void* LLFloaterCustomize::createWearablePanel(void* userdata)
{
	WearablePanelData* data = (WearablePanelData*)userdata;
	LLWearableType::EType type = data->mType;
	LLPanelEditWearable* &panel = data->mFloater->mWearablePanelList[type];
	if (!gAgent.isTeen() || edit_wearable_for_teens(type))
		panel = new LLPanelEditWearable( type, data->mFloater );
	else
		panel = NULL;
	delete data;
	return panel;
}

////////////////////////////////////////////////////////////////////////////

void LLFloaterCustomize::switchToDefaultSubpart()
{
	getCurrentWearablePanel()->showDefaultSubpart();
}

void LLFloaterCustomize::draw()
{
	if( isMinimized() )
	{
		LLFloater::draw();
		return;
	}

	// only do this if we are in the customize avatar mode
	// and not transitioning into or out of it
	// *TODO: This is a sort of expensive call, which only needs
	// to be called when the tabs change or an inventory item
	// arrives. Figure out some way to avoid this if possible.
	updateInventoryUI();
	
	updateAvatarHeightDisplay();

	LLScrollingPanelParam::sUpdateDelayFrames = 0;
	
	LLFloater::draw();
}

bool LLFloaterCustomize::isWearableDirty() const
{
	LLWearableType::EType cur = getCurrentWearableType();
	for(U32 i = 0; i < gAgentWearables.getWearableCount(cur); ++i)
	{
		LLViewerWearable* wearable = gAgentWearables.getViewerWearable(cur,i);
		if(wearable && wearable->isDirty())
			return TRUE;
	}
	return FALSE;
}

bool LLFloaterCustomize::onTabPrecommit( LLUICtrl* ctrl, const LLSD& param )
{
	std::string panel_name = param.asString();
	for(U32 type=LLWearableType::WT_SHAPE;type<LLWearableType::WT_INVALID;++type)
	{
		std::string type_name = LLWearableType::getTypeName((LLWearableType::EType)type);
		std::transform(panel_name.begin(), panel_name.end(), panel_name.begin(), tolower); 

		if(type_name == panel_name)
		{
			if(mCurrentWearableType != type)
			{
				askToSaveIfDirty(boost::bind(&LLFloaterCustomize::onCommitChangeTab, this, _1, (LLTabContainer*)ctrl, param.asString(), (LLWearableType::EType)type));
				return false;
			}
		}
	}
	return true;
}


void LLFloaterCustomize::onTabChanged( const LLSD& param )
{
	std::string panel_name = param.asString();
	for(U32 type=LLWearableType::WT_SHAPE;type<LLWearableType::WT_INVALID;++type)
	{
		std::string type_name = LLWearableType::getTypeName((LLWearableType::EType)type);
		std::transform(panel_name.begin(), panel_name.end(), panel_name.begin(), tolower); 

		if(type_name == panel_name)
		{
			const BOOL disable_camera_switch = LLWearableType::getDisableCameraSwitch((LLWearableType::EType)type);
			setCurrentWearableType((LLWearableType::EType)type, disable_camera_switch);
			break;
		}
	}
}

void LLFloaterCustomize::onCommitChangeTab(BOOL proceed, LLTabContainer* ctrl, std::string panel_name, LLWearableType::EType type)
{
	if (!proceed)
	{
		return;
	}

	const BOOL disable_camera_switch = LLWearableType::getDisableCameraSwitch(type);
	setCurrentWearableType(type, disable_camera_switch);
	ctrl->selectTabByName(panel_name);
}



////////////////////////////////////////////////////////////////////////////

const S32 LOWER_BTN_HEIGHT = 18 + 8;

const S32 FLOATER_CUSTOMIZE_BUTTON_WIDTH = 82;
const S32 FLOATER_CUSTOMIZE_BOTTOM_PAD = 30;
const S32 LINE_HEIGHT = 16;
const S32 HEADER_PAD = 8;
const S32 HEADER_HEIGHT = 3 * (LINE_HEIGHT + LLFLOATER_VPAD) + (2 * LLPANEL_BORDER_WIDTH) + HEADER_PAD; 

void LLFloaterCustomize::initScrollingPanelList()
{
	LLScrollContainer* scroll_container =
		getChild<LLScrollContainer>("panel_container");
	// LLScrollingPanelList's do not import correctly 
// 	mScrollingPanelList = LLUICtrlFactory::getScrollingPanelList(this, "panel_list");
	mScrollingPanelList = new LLScrollingPanelList(std::string("panel_list"), LLRect());
	if (scroll_container)
	{
		scroll_container->setScrolledView(mScrollingPanelList);
		scroll_container->addChild(mScrollingPanelList);
	}
}

void LLFloaterCustomize::wearablesChanged(LLWearableType::EType type)
{
	llassert( type < LLWearableType::WT_COUNT );
	gSavedSettings.setU32("AvatarSex", (gAgentAvatarp->getSex() == SEX_MALE) );
	
	LLPanelEditWearable* panel = mWearablePanelList[ type ];
	if( panel )
	{
		panel->wearablesChanged();
	}
}

void LLFloaterCustomize::updateVisiblity(bool force_disable_camera_switch/*=false*/)
{
	if(!getVisible())
	{
		if(force_disable_camera_switch || !gAgentCamera.cameraCustomizeAvatar() || !gAgentCamera.getCameraAnimating() || (gMorphView && gMorphView->getVisible()))
		{
			if(gAgentAvatarp)gAgentAvatarp->mSpecialRenderMode = 3;
			setVisibleAndFrontmost(TRUE);
		}
	}
}

void LLFloaterCustomize::updateScrollingPanelList()
{
	getCurrentWearablePanel()->updateScrollingPanelList();
}

void LLFloaterCustomize::askToSaveIfDirty( boost::function<void (BOOL)> cb )
{
	if(isWearableDirty())
	{
		// Ask if user wants to save, then continue to next step afterwards
		mNextStepAfterSaveCallback.connect(cb);

		// Bring up view-modal dialog: Save changes? Yes, No, Cancel
		LLNotificationsUtil::add("SaveClothingBodyChanges", LLSD(), LLSD(),
			boost::bind(&LLFloaterCustomize::onSaveDialog, this, _1, _2));
	}
	else
	{
		cb(TRUE);	//just call it immediately.
	}
}

void LLFloaterCustomize::saveCurrentWearables()
{
	LLWearableType::EType cur = getCurrentWearableType();

	for(U32 i = 0;i < gAgentWearables.getWearableCount(cur);++i)
	{
		LLViewerWearable* wearable = gAgentWearables.getViewerWearable(cur,i);
		if(wearable && wearable->isDirty())
		{
			/*
			===============================================================================================
			Copy-pasted some code from LLPanelEditWearable::saveChanges instead of just calling saveChanges, as
			we only have one 'active' panel per wearable type, not per layer. The panels just update when
			layer of focus is changed. Easier just to do it right here manually.
			===============================================================================================
			*/ 
			// Find an existing link to this wearable's inventory item, if any, and its description field.
			if(gAgentAvatarp->isUsingServerBakes())
			{
				LLInventoryItem *link_item = NULL;
				std::string description;
				LLInventoryModel::item_array_t links =
					LLAppearanceMgr::instance().findCOFItemLinks(wearable->getItemID());
				if (links.size()>0)
				{
					link_item = links.get(0).get();
					if (link_item && link_item->getIsLinkType())
					{
						description = link_item->getActualDescription();
					}
				}
				// Make another copy of this link, with the same
				// description.  This is needed to bump the COF
				// version so texture baking service knows appearance has changed.
				if (link_item)
				{
					// Create new link
					link_inventory_item( gAgent.getID(),
										 link_item->getLinkedUUID(),
										 LLAppearanceMgr::instance().getCOF(),
										 link_item->getName(),
										 description,
										 LLAssetType::AT_LINK,
										 NULL);
					// Remove old link
					gInventory.purgeObject(link_item->getUUID());
				}
			}
			gAgentWearables.saveWearable( cur, i );
		}
	}
}

bool LLFloaterCustomize::onSaveDialog(const LLSD& notification, const LLSD& response )
{
	S32 option = LLNotification::getSelectedOption(notification, response);
	if(option == 0)
	{
		saveCurrentWearables();
	}
	else
	{
		LLWearableType::EType cur = getCurrentWearableType();
		for(U32 i = 0;i < gAgentWearables.getWearableCount(cur);++i)
		{
			LLViewerWearable* wearable = gAgentWearables.getViewerWearable(cur,i);
			if(wearable && wearable->isDirty())
			{
				gAgentWearables.revertWearable( cur, i );
			}
		}
	}

	mNextStepAfterSaveCallback(option < 2);
	mNextStepAfterSaveCallback.disconnect_all_slots();	//Should this be done?

	return false;
}

// fetch observer
class LLCurrentlyWorn : public LLInventoryFetchItemsObserver
{
public:
	LLCurrentlyWorn(const uuid_vec_t& item_ids) : LLInventoryFetchItemsObserver(item_ids){}
	~LLCurrentlyWorn() {}
	virtual void done() { /* no operation necessary */}
};

void LLFloaterCustomize::fetchInventory()
{
	// Fetch currently worn items
	uuid_vec_t ids;
	LLUUID item_id;
	for(S32 type = (S32)LLWearableType::WT_SHAPE; type < (S32)LLWearableType::WT_COUNT; ++type)
	{
		for(U32 i = 0; i < gAgentWearables.getWearableCount((LLWearableType::EType)type); ++i)
		{
			item_id = gAgentWearables.getWearableItemID((LLWearableType::EType)type, i);
			if(item_id.notNull())
			{
				ids.push_back(item_id);
			}
		}
	}

	// Fire & forget. The mInventoryObserver will catch inventory
	// updates and correct the UI as necessary.
	LLCurrentlyWorn worn(ids);
	worn.startFetch();
}

void LLFloaterCustomize::updateInventoryUI()
{
	BOOL all_complete = TRUE;
	BOOL is_complete = FALSE;
	U32 perm_mask = 0x0;
	LLPanelEditWearable* panel;
	LLViewerInventoryItem* item;
	for(S32 i = 0; i < LLWearableType::WT_COUNT; ++i)
	{
		item = NULL;
		panel = mWearablePanelList[i];
		if(panel)
		{
			LLViewerWearable* wearable = panel->getWearable();
			if(wearable)
				item = gInventory.getItem(wearable->getItemID());
		}
		if(item)
		{
			is_complete = item->isComplete();
			if(!is_complete)
			{
				all_complete = FALSE;
			}
			perm_mask = item->getPermissions().getMaskOwner();
		}
		else
		{
			is_complete = false;
			perm_mask = 0x0;
		}
		if(i == mCurrentWearableType)
		{
			if(panel)
			{
				panel->setUIPermissions(perm_mask, is_complete);
			}
			//BOOL is_vis = panel && item && is_complete && (perm_mask & PERM_MODIFY);
			//childSetVisible("panel_container", is_vis);
		}
	}

	childSetEnabled("Make Outfit", all_complete);
}

