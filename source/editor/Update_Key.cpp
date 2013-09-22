#include "pch.h"
#include "../ogre/common/Def_Str.h"
#include "../ogre/common/RenderConst.h"
#include "../ogre/common/Gui_Def.h"
#include "../ogre/common/data/CData.h"
#include "../ogre/common/data/SceneXml.h"
#include "../ogre/common/data/FluidsXml.h"
#include "../ogre/common/GuiCom.h"
#include "settings.h"
#include "CApp.h"
#include "CGui.h"
#include "../road/Road.h"
#include "../vdrift/pathmanager.h"
#include "../ogre/common/MultiList2.h"
#include <OgreTerrain.h>
#include <MyGUI.h>
#include "BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h"
#include "BulletCollision/CollisionDispatch/btCollisionObject.h"
#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "../sdl4ogre/sdlinputwrapper.hpp"
#include "../sdl4ogre/sdlcursormanager.hpp"
using namespace MyGUI;
using namespace Ogre;


//---------------------------------------------------------------------------------------------------------------
//  Key Press
//---------------------------------------------------------------------------------------------------------------

bool App::keyPressed(const SDL_KeyboardEvent &arg)
{
	SDL_Scancode skey = arg.keysym.scancode;
	#define key(a)  SDL_SCANCODE_##a
	
	///  Preview camera  ---------------------
	if (edMode == ED_PrvCam)
	{
		switch (skey)
		{
			case key(ESCAPE):  // exit
			case key(F7):  togPrvCam();  break;

			case key(RETURN):  // save screen
			{	int u = pSet->allow_save ? pSet->gui.track_user : 1;
				rt[RTs-1].rndTex->writeContentsToFile(gcom->pathTrk[u] + pSet->gui.track + "/preview/view.jpg");
				gcom->listTrackChng(gcom->trkList,0);  // upd gui img
				gui->Status("Preview saved", 1,1,0);
			}	break;

			case key(F12):  // screenshot
				mWindow->writeContentsToTimestampedFile(PATHMANAGER::Screenshots() + "/", ".jpg");
				return true;
		}
		return true;  //!
	}

	//  main menu keys
	Widget* wf = MyGUI::InputManager::getInstance().getKeyFocusWidget();
	bool edFoc = wf && wf->getTypeName() == "EditBox";

	if (pSet->isMain && bGuiFocus)
	{
		switch (skey)
		{
		case key(UP):  case key(KP_8):
			pSet->inMenu = (pSet->inMenu-1+WND_ALL)%WND_ALL;
			gui->toggleGui(false);  return true;

		case key(DOWN):  case key(KP_2):
			pSet->inMenu = (pSet->inMenu+1)%WND_ALL;
			gui->toggleGui(false);  return true;

		case key(RETURN):
			pSet->isMain = false;
			gui->toggleGui(false);  return true;
		}
	}
	if (!pSet->isMain && bGuiFocus)
	{
		switch (skey)
		{
		case key(BACKSPACE):
			if (pSet->isMain)  break;
			if (bGuiFocus)
			{	if (edFoc)  break;
				pSet->isMain = true;  gui->toggleGui(false);  }
			return true;
		}
	}

	//  change gui tabs
	TabPtr tab = 0;  MyGUI::TabControl* sub = 0;  int iTab1 = 1;
	if (bGuiFocus && !pSet->isMain)
	switch (pSet->inMenu)
	{
		case WND_Edit:    tab = mWndTabsEdit;  sub = gui->vSubTabsEdit[tab->getIndexSelected()];  break;
		case WND_Help:    tab = sub = gui->vSubTabsHelp[1];  iTab1 = 0;  break;
		case WND_Options: tab = mWndTabsOpts;  sub = gui->vSubTabsOpts[tab->getIndexSelected()];  break;
	}

	//  global keys
	//------------------------------------------------------------------------------------------------------------------------------
	switch (skey)
	{
		case key(ESCAPE): //  quit
			if (pSet->escquit)
			{
				mShutDown = true;
			}	return true;

		case key(F1):
		case key(GRAVE):
			if (ctrl)  // context help (show for cur mode)
			{
				if (bMoveCam)		 gui->GuiShortcut(WND_Help, 1, 0);
				else switch (edMode)
				{	case ED_Smooth: case ED_Height: case ED_Filter:
					case ED_Deform:  gui->GuiShortcut(WND_Help, 1, 1);  break;
					case ED_Road:    gui->GuiShortcut(WND_Help, 1, 2);  break;
					case ED_Start:   gui->GuiShortcut(WND_Help, 1, 4);  break;
					case ED_Fluids:  gui->GuiShortcut(WND_Help, 1, 5);  break;
					case ED_Objects: gui->GuiShortcut(WND_Help, 1, 6);  break;
					default:		 gui->GuiShortcut(WND_Help, 1, 0);  break;
			}	}
			else	//  Gui mode, Options
				gui->toggleGui(true);
			return true;

		case key(F12): //  screenshot
			mWindow->writeContentsToTimestampedFile(PATHMANAGER::Screenshots() + "/", ".jpg");
			return true;

		//  save, reload, update
		case key(F4):  SaveTrack();	return true;
		case key(F5):  LoadTrack();	return true;
		case key(F8):  UpdateTrack();  return true;

		case key(F9):  // blendmap
			if (alt)
			{	BGui::WP wp = gui->chAutoBlendmap;  ChkEv(autoBlendmap);  }
			else	bTerUpdBlend = true;  return true;

		//  prev num tab (layers,grasses,models)
		case key(1):
   			if (alt)  {  gui->NumTabNext(-1);  return true;  }
			break;
		//  next num tab
		case key(2):
   			if (alt)  {  gui->NumTabNext(1);  return true;  }
			break;

		case key(F2):  // +-rt num
   			if (alt)
   			{	pSet->num_mini = (pSet->num_mini - 1 + RTs+2) % (RTs+2);  UpdMiniVis();  }
   			else
   			if (bGuiFocus && tab && !pSet->isMain)
   				if (shift)  // prev gui subtab
   				{
   					if (sub)  {  int num = sub->getItemCount();
   						sub->setIndexSelected( (sub->getIndexSelected() - 1 + num) % num );  }
	   			}
   				else	// prev gui tab
   				{	int num = tab->getItemCount()-1, i = tab->getIndexSelected();
					if (i==iTab1)  i = num;  else  --i;
					tab->setIndexSelected(i);  if (iTab1==1)  gui->MenuTabChg(tab,i);
	   			}
   			break;

		case key(F3):  // tabs,sub
   			if (alt)
   			{	pSet->num_mini = (pSet->num_mini + 1) % (RTs+2);  UpdMiniVis();  }
   			else
   			if (bGuiFocus && tab && !pSet->isMain)
   				if (shift)  // next gui subtab
   				{
   					if (sub)  {  int num = sub->getItemCount();
   						sub->setIndexSelected( (sub->getIndexSelected() + 1) % num );  }
	   			}
	   			else	// next gui tab
	   			{	int num = tab->getItemCount()-1, i = tab->getIndexSelected();
					if (i==num)  i = iTab1;  else  ++i;
					tab->setIndexSelected(i);  if (iTab1==1)  gui->MenuTabChg(tab,i);
				}
   			break;
   			
		case key(RETURN):  // load track
			if (bGuiFocus)
			if (mWndTabsEdit->getIndexSelected() == 1 && !pSet->isMain && pSet->inMenu == WND_Edit)
				gui->btnNewGame(0);
   			break;

		//  Wire Frame  F11
		case key(F11):
		{	mbWireFrame = !mbWireFrame;
			mCamera->setPolygonMode(mbWireFrame ? PM_WIREFRAME : PM_SOLID);
			if (ndSky)	ndSky->setVisible(!mbWireFrame);  // hide sky
			return true;
		}	break;

		//  Show Stats  ctrl-I
		case key(I):
   			if (ctrl)  {  gui->chkInputBar(gui->chInputBar);  return true;  }
			break;

		//  Top view  alt-Z
		case key(Z):
			if (alt)  {  gui->toggleTopView();  return true;  }
			break;

		//  load next track  F6
		case key(F6):
			if (pSet->check_load)
			{	gui->iLoadNext = shift ? -1 : 1;  return true;  }
			break;
	}

	//  GUI  keys in edits  ---------------------
	if (bGuiFocus && mGui && !alt && !ctrl)
	{
		MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::Enum(mInputWrapper->sdl2OISKeyCode(arg.keysym.sym)), 0);
		return true;
	}


	///  Road keys  * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * 
	if (edMode == ED_Road && road && bEdit())
	{
		if (iSnap > 0)
		switch (skey)
		{
			case key(1):  road->AddYaw(-1,angSnap,alt);  break;
			case key(2):  road->AddYaw( 1,angSnap,alt);  break;
			case key(3):  road->AddRoll(-1,angSnap,alt);  break;
			case key(4):  road->AddRoll( 1,angSnap,alt);  break;
		}
		switch (skey)
		{
			//  choose 1
			case key(SPACE):
				if (ctrl)	road->CopyNewPoint();
				else		road->ChoosePoint();  break;
				
			//  multi sel
			case key(BACKSPACE):
				if (alt)		road->SelAll();
				else if (ctrl)	road->SelClear();
				else			road->SelAddPoint();  break;
				
			//  ter on  first,last
			case key(HOME):  case key(KP_7):
				if (alt)	road->MirrorSel(shift);  else
				if (ctrl)	road->FirstPoint();
				else		road->ToggleOnTerrain();  break;
				
			//  cols
			case key(END):  case key(KP_1):
				if (ctrl)	road->LastPoint();
				else		road->ToggleColums();  break;

			//  prev,next
			case key(PAGEUP):  case key(KP_9):
				road->PrevPoint();  break;
			case key(PAGEDOWN):	case key(KP_3):
				road->NextPoint();  break;

			//  del
			case key(DELETE):  case key(KP_PERIOD):
			case key(KP_5):
				if (ctrl)	road->DelSel();
				else		road->Delete();  break;

			//  ins
			case key(INSERT):  case key(KP_0):
				if (ctrl && !shift && !alt)	{	if (road->CopySel())  gui->Status("Copy",0.6,0.8,1.0);  }
				else if (!ctrl && shift && !alt)	road->Paste();
				else if ( ctrl && shift && !alt)	road->Paste(true);
				else
				{	road->newP.pos.x = road->posHit.x;
					road->newP.pos.z = road->posHit.z;
					if (!sc->ter)
						road->newP.pos.y = road->posHit.y;
					//road->newP.aType = AT_Both;
					road->Insert(shift ? INS_Begin : ctrl ? INS_End : alt ? INS_CurPre : INS_Cur);
				}	break;					  

			case key(0):
				if (ctrl)  {   road->Set1stChk();  break;  }
			case key(EQUALS):  road->ChgMtrId(1);  break;
			case key(9):
			case key(MINUS):   road->ChgMtrId(-1);  break;

			case key(5):  road->ChgAngType(-1);  break;
			case key(6):  if (shift)  road->AngZero();  else
						road->ChgAngType(1);  break;

			case key(7):  iSnap = (iSnap-1+ciAngSnapsNum)%ciAngSnapsNum;  angSnap = crAngSnaps[iSnap];  break;
			case key(8):  iSnap = (iSnap+1)%ciAngSnapsNum;                angSnap = crAngSnaps[iSnap];  break;
			
			case key(U):  AlignTerToRoad();  break;
			
			//  looped  todo: finish set..
			case key(N):  road->isLooped = !road->isLooped;
				road->recalcTangents();  road->RebuildRoad(true);  break;
		}
	}

	//  ter brush shape
	if (edMode < ED_Road && !alt)
	switch (skey)
	{
		case key(K):	if (ctrl)  {  mBrShape[curBr] = (EBrShape)((mBrShape[curBr]-1 + BRS_ALL) % BRS_ALL);  updBrush();  }  break;
		case key(L):	if (ctrl)  {  mBrShape[curBr] = (EBrShape)((mBrShape[curBr]+1) % BRS_ALL);            updBrush();  }  break;
		case key(N): case key(COMMA):	mBrOct[curBr] = std::max(1, mBrOct[curBr]-1);  updBrush();  break;
		case key(M): case key(PERIOD):	mBrOct[curBr] = std::min(7, mBrOct[curBr]+1);  updBrush();  break;
	}

	//  ter brush presets  ----
	if (edMode < ED_Road && alt && skey >= key(1) && skey <= key(0) && !bMoveCam)
	{
		// TODO
		int id = skey - key(1);
		if (shift)  id += 10;
		SetBrushPreset(id);
	}

	
	//  Fluids  ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ 
	if (edMode == ED_Fluids)
	{	int fls = sc->fluids.size();
		switch (skey)
		{
			//  ins
			case key(INSERT):  case key(KP_0):
			if (road && road->bHitTer)
			{
				FluidBox fb;	fb.name = "water blue";
				fb.pos = road->posHit;	fb.rot = Ogre::Vector3(0.f, 0.f, 0.f);
				fb.size = Ogre::Vector3(50.f, 20.f, 50.f);	fb.tile = Vector2(0.01f, 0.01f);
				sc->fluids.push_back(fb);
				sc->UpdateFluidsId();
				iFlCur = sc->fluids.size()-1;
				bRecreateFluids = true;
			}	break;
		}
		if (fls > 0)
		switch (skey)
		{
			//  first, last
			case key(HOME):  case key(KP_7):
				iFlCur = 0;  UpdFluidBox();  break;
			case key(END):  case key(KP_1):
				if (fls > 0)  iFlCur = fls-1;  UpdFluidBox();  break;

			//  prev,next
			case key(PAGEUP):  case key(KP_9):
				if (fls > 0) {  iFlCur = (iFlCur-1+fls)%fls;  }  UpdFluidBox();  break;
			case key(PAGEDOWN):	case key(KP_3):
				if (fls > 0) {  iFlCur = (iFlCur+1)%fls;	  }  UpdFluidBox();  break;

			//  del
			case key(DELETE):  case key(KP_PERIOD):
			case key(KP_5):
				if (fls == 1)	sc->fluids.clear();
				else			sc->fluids.erase(sc->fluids.begin() + iFlCur);
				iFlCur = std::max(0, std::min(iFlCur, (int)sc->fluids.size()-1));
				bRecreateFluids = true;
				break;

			//  prev,next type
			case key(9):  case key(MINUS):
			{	FluidBox& fb = sc->fluids[iFlCur];
				fb.id = (fb.id-1 + data->fluids->fls.size()) % data->fluids->fls.size();
				fb.name = data->fluids->fls[fb.id].name;
				bRecreateFluids = true;  }	break;
			case key(0):  case key(EQUALS):
			{	FluidBox& fb = sc->fluids[iFlCur];
				fb.id = (fb.id+1) % data->fluids->fls.size();
				fb.name = data->fluids->fls[fb.id].name;
				bRecreateFluids = true;  }	break;
		}
	}

	//  Objects  | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
	if (edMode == ED_Objects)
	{	int objs = sc->objects.size(), objAll = gui->vObjNames.size();
		switch (skey)
		{
			case key(SPACE):
				gui->iObjCur = -1;  PickObject();  UpdObjPick();  break;
				
			//  prev,next type
			case key(9):  case key(MINUS):   gui->SetObjNewType((gui->iObjTNew-1 + objAll) % objAll);  break;
			case key(0):  case key(EQUALS):  gui->SetObjNewType((gui->iObjTNew+1) % objAll);  break;
				
			//  ins
			case key(INSERT):	case key(KP_0):
			if (road && road->bHitTer)
			{
				gui->AddNewObj();
				//iObjCur = sc->objects.size()-1;  // auto select inserted-
				UpdObjPick();
			}	break;
			
			//  sel
			case key(BACKSPACE):
				if (ctrl)  gui->vObjSel.clear();  // unsel all
				else
				if (gui->iObjCur > -1)
					if (gui->vObjSel.find(gui->iObjCur) == gui->vObjSel.end())
						gui->vObjSel.insert(gui->iObjCur);  // add to sel
					else
						gui->vObjSel.erase(gui->iObjCur);  // unselect
				break;
		}
		::Object* o = gui->iObjCur == -1 ? &gui->objNew :
					((gui->iObjCur >= 0 && objs > 0 && gui->iObjCur < objs) ? &sc->objects[gui->iObjCur] : 0);
		switch (skey)
		{
			//  first, last
			case key(HOME):  case key(KP_7):
				gui->iObjCur = 0;  UpdObjPick();  break;
			case key(END):  case key(KP_1):
				if (objs > 0)  gui->iObjCur = objs-1;  UpdObjPick();  break;

			//  prev,next
			case key(PAGEUP):  case key(KP_9):
				if (objs > 0) {  gui->iObjCur = (gui->iObjCur-1+objs)%objs;  }  UpdObjPick();  break;
			case key(PAGEDOWN):	case key(KP_3):
				if (objs > 0) {  gui->iObjCur = (gui->iObjCur+1)%objs;	  }  UpdObjPick();  break;

			//  del
			case key(DELETE):  case key(KP_PERIOD):
			case key(KP_5):
				if (gui->iObjCur >= 0 && objs > 0)
				{	::Object& o = sc->objects[gui->iObjCur];
					mSceneMgr->destroyEntity(o.ent);
					mSceneMgr->destroySceneNode(o.nd);
					
					if (objs == 1)	sc->objects.clear();
					else			sc->objects.erase(sc->objects.begin() + gui->iObjCur);
					gui->iObjCur = std::min(gui->iObjCur, (int)sc->objects.size()-1);
					UpdObjPick();
				}	break;

			//  move,rot,scale
			case key(1):
				if (!shift)  gui->objEd = EO_Move;
				else if (o)
				{
					if (gui->iObjCur == -1)  // reset h
					{
						o->pos[2] = 0.f;  o->SetFromBlt();  UpdObjPick();
					}
					else if (road)  // move to ter
					{
						const Ogre::Vector3& v = road->posHit;
						o->pos[0] = v.x;  o->pos[1] =-v.z;  o->pos[2] = v.y + gui->objNew.pos[2];
						o->SetFromBlt();  UpdObjPick();
					}
				}	break;

			case key(2):
				if (!shift)  gui->objEd = EO_Rotate;
				else if (o)  // reset rot
				{
					o->rot = QUATERNION<float>(0,1,0,0);
					o->SetFromBlt();  UpdObjPick();
				}	break;

			case key(3):
				if (!shift)  gui->objEd = EO_Scale;
				else if (o)  // reset scale
				{
					o->scale = Ogre::Vector3::UNIT_SCALE * (ctrl ? 0.5f : 1.f);
					o->nd->setScale(o->scale);  UpdObjPick();
				}	break;
		}
	}

	//  Rivers  ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
	if (edMode == ED_Rivers)
	{
		// todo
	}

	///  Common Keys  ************************************************************************************************************
	if (alt)
	switch (skey)
	{
		case key(Q):  gui->GuiShortcut(WND_Edit, 1);  return true;  // Q Track
		case key(S):  gui->GuiShortcut(WND_Edit, 2);  return true;  // S Sun

		case key(H):  gui->GuiShortcut(WND_Edit, 3);  return true;  // H Heightmap
		 case key(D): gui->GuiShortcut(WND_Edit, 3,0);  return true;  //  D -Brushes

		case key(T):  gui->GuiShortcut(WND_Edit, 4);  return true;  // T Layers (Terrain)
		 case key(B): gui->GuiShortcut(WND_Edit, 4,0);  return true;  //  B -Blendmap
		 case key(P): gui->GuiShortcut(WND_Edit, 4,1);  return true;  //  P -Particles
		 case key(U): gui->GuiShortcut(WND_Edit, 4,2);  return true;  //  U -Surfaces

		case key(V):  gui->GuiShortcut(WND_Edit, 5);  return true;  // V Vegetation
		 case key(M): gui->GuiShortcut(WND_Edit, 5,2);  return true;  //  M -Models

		case key(R):  gui->GuiShortcut(WND_Edit, 6);  return true;  // R Road
		case key(X):  gui->GuiShortcut(WND_Edit, 7);  return true;  // X Objects
		case key(O):  gui->GuiShortcut(WND_Edit, 8);  return true;  // O Tools

		case key(C):  gui->GuiShortcut(WND_Options, 1,0);	return true;  // C Screen
		case key(G):  gui->GuiShortcut(WND_Options, 1,1);	return true;  // G -Graphics
		//case key(N):  gui->GuiShortcut(WND_Options, 1,1);	return true;  // N --Vegetation !
		case key(K):  gui->GuiShortcut(WND_Options, 1,2);  return true;  // K -Tweak
		case key(E):  gui->GuiShortcut(WND_Options, 2);    return true;  // E Settings
		
		case key(I):  gui->GuiShortcut(WND_Help, 1);  return true;  // I Input/help
		case key(J):  gui->GuiShortcut(WND_Edit, 9);  return true;  // J Warnings
	}
	else
	switch (skey)
	{
		case key(TAB):	//  Camera / Edit mode
		if (!bGuiFocus && !alt)  {
			bMoveCam = !bMoveCam;  UpdVisGui();  UpdFluidBox();  UpdObjPick();
		}	break;

		//  fog
		case key(G):  {
			pSet->bFog = !pSet->bFog;  gui->chkFog->setStateSelected(pSet->bFog);  UpdFog();  }  break;
		//  trees
		case key(V):  bTrGrUpd = true;  break;
		//  weather
		case key(I):  {
			pSet->bWeather = !pSet->bWeather;  gui->chkWeather->setStateSelected(pSet->bWeather);  }  break;

		//  terrain
		case key(D):  if (bEdit()){  SetEdMode(ED_Deform);  curBr = 0;  updBrush();  UpdEditWnds();  }	break;
		case key(S):  if (bEdit()){  SetEdMode(ED_Smooth);  curBr = 1;  updBrush();  UpdEditWnds();  }	break;
		case key(E):  if (bEdit()){  SetEdMode(ED_Height);  curBr = 2;  updBrush();  UpdEditWnds();  }	break;
		case key(F):  if (bEdit()){  SetEdMode(ED_Filter);  curBr = 3;  updBrush();  UpdEditWnds();  }
			else  //  focus on find edit  (global)
			if (ctrl && gcom->edTrkFind /*&& bGuiFocus &&
				!pSet->isMain && pSet->inMenu == WND_Edit && mWndTabsEdit->getIndexSelected() == 1*/)
			{
				gui->GuiShortcut(WND_Edit, 1);  // Track tab
				MyGUI::InputManager::getInstance().resetKeyFocusWidget();
				MyGUI::InputManager::getInstance().setKeyFocusWidget(gcom->edTrkFind);
				return true;
			}	break;

		//  road
		case key(R):  if (bEdit()){  SetEdMode(ED_Road);	UpdEditWnds();  }	break;
		case key(B):  if (road)  {  road->UpdPointsH();  road->RebuildRoad(true);  }  break;
		case key(T):  if (mWndRoadStats)  mWndRoadStats->setVisible(!mWndRoadStats->getVisible());  break;
		case key(M):  if (edMode == ED_Road && road)  road->ToggleMerge();  break;

		//  start pos
		case key(Q):  if (bEdit()){  SetEdMode(ED_Start);  UpdEditWnds();  }   break;
		case key(SPACE):
			if (edMode == ED_Start && road)  road->iDir *= -1;  break;
		//  prv cam
		case key(F7):  togPrvCam();  break;

		//  fluids
		case key(W):  if (bEdit()){  SetEdMode(ED_Fluids);  UpdEditWnds();  }   break;
		case key(F10):  SaveWaterDepth();   break;

		//  objects
		case key(C):  if (edMode == ED_Objects)  {  gui->objSim = !gui->objSim;  ToggleObjSim();  }  break;
		case key(X):  if (bEdit()){  SetEdMode(ED_Objects);  UpdEditWnds();  }   break;
		
		//  rivers
		///case key(A):	if (bEdit()){  SetEdMode(ED_Rivers);  UpdEditWnds();  }	break;
	}

	return true;
}
