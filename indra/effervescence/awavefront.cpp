/**
 * @file awavefront.cpp
 * @brief A system which allows saving in-world objects to Wavefront .OBJ files for offline texturizing/shading.
 * @author Apelsin
 *
 * $LicenseInfo:firstyear=2011&license=LGPLV3$
 * Copyright (C) 2011-2013 Apelsin
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 3 of the License, or (at your option) any later version.
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

#include "awavefront.h"

// library includes
#include "aifilepicker.h"

// newview includes
#include "llavatarappearancedefines.h"
#include "llface.h"
#include "llvoavatar.h"

typedef std::vector<LLAvatarJoint*> avatar_joint_list_t;

// menu includes
#include "llevent.h"
#include "llmemberlistener.h"
#include "llview.h"
#include "llselectmgr.h"

LLVOAvatar* find_avatar_from_object(LLViewerObject* object);

typedef LLMemberListener<LLView> view_listener_t;

namespace
{
	const std::string OBJ(".obj");
	void save_wavefront_continued(WavefrontSaver* wfsaver, AIFilePicker* filepicker)
	{
		if (filepicker->hasFilename())
		{
			const std::string selected_filename = filepicker->getFilename();
			if (LLFILE* fp = LLFile::fopen(selected_filename, "wb"))
			{
				wfsaver->saveFile(fp);
				llinfos << "OBJ file saved to " << selected_filename << llendl;
				fclose(fp);
			}
			else llerrs << "can't open: " << selected_filename << llendl;
		}
		else llwarns << "No file; bailing" << llendl;

		delete wfsaver;
	}
}

Wavefront::Wavefront(vert_t v, tri_t t)
:	name("")
,	vertices(v)
,	triangles(t)
{
}

Wavefront::Wavefront(const LLVolumeFace* face, const LLXform* transform, const LLXform* transform_normals)
:	name("")
{
	class v4adapt
	{
		private:
			LLStrider<LLVector4a> mV4aStrider;
		public:
			v4adapt(LLVector4a* vp){ mV4aStrider = vp; }
			inline LLVector3 operator[] (const unsigned int i)
			{
				return LLVector3((F32*)&mV4aStrider[i]);
			}
	};
	v4adapt verts(face->mPositions);
	for (S32 i = 0; i < face->mNumVertices; ++i)
	{
		LLVector3 v = verts[i];
		vertices.push_back(std::pair<LLVector3, LLVector2>(v, face->mTexCoords[i]));
	}

	if (transform) Transform(vertices, transform);

	v4adapt norms(face->mNormals);
	for (S32 i = 0; i < face->mNumVertices; ++i)
		normals.push_back(norms[i]);

	if (transform_normals) Transform(normals, transform_normals);

	for (S32 i = 0; i < face->mNumIndices/3; ++i)
	{
		triangles.push_back(tri(face->mIndices[i*3+0], face->mIndices[i*3+1], face->mIndices[i*3+2]));
	}
}

Wavefront::Wavefront(LLFace* face, LLPolyMesh* mesh, const LLXform* transform, const LLXform* transform_normals)
:	name("")
{
	LLVertexBuffer* vb = face->getVertexBuffer();
	if (!vb) return;

	LLStrider<LLVector3> getVerts;
	LLStrider<LLVector3> getNorms;
	LLStrider<LLVector2> getCoord;
	LLStrider<U16> getIndices;
	face->getGeometry(getVerts, getNorms, getCoord, getIndices);

	const U16 start = face->getGeomStart();
	const U32 end = start + (mesh ? mesh->getNumVertices() : vb->getNumVerts()) - 1; //vertices
	for (int i = start; i <= end; ++i)
		vertices.push_back(std::make_pair(getVerts[i], getCoord[i]));

	if (transform) Transform(vertices, transform);

	for (int i = start; i <= end; ++i)
		normals.push_back(getNorms[i]);

	if (transform_normals) Transform(normals, transform_normals);

	const U32 pcount = mesh ? mesh->getNumFaces() : (vb->getNumIndices()/3); //indices
	const U16 offset = face->getIndicesStart(); //indices
	for (int i = 0; i < pcount; ++i)
	{
		triangles.push_back(tri(getIndices[i * 3  + offset] + start, getIndices[i * 3 + 1 + offset] + start, getIndices[i * 3 + 2 + offset] + start));
	}
}

void Wavefront::Transform(vert_t& v, const LLXform* x) //recursive
{
	LLMatrix4 m;
	x->getLocalMat4(m);
	for (vert_t::iterator iterv = v.begin(); iterv != v.end(); ++iterv)
	{
		iterv->first = iterv->first * m;
	}

	if (const LLXform* xp = x->getParent()) Transform(v, xp);
}

void Wavefront::Transform(vec3_t& v, const LLXform* x) //recursive
{
	LLMatrix4 m;
	x->getLocalMat4(m);
	for (vec3_t::iterator iterv = v.begin(); iterv != v.end(); ++iterv)
	{
		*iterv = *iterv * m;
	}

	if (const LLXform* xp = x->getParent()) Transform(v, xp);
}

WavefrontSaver::WavefrontSaver()
{}

void WavefrontSaver::Add(const Wavefront& obj)
{
	obj_v.push_back(obj);
}

void WavefrontSaver::Add(const LLVolume* vol, const LLXform* transform, const LLXform* transform_normals)
{
	const int faces = vol->getNumVolumeFaces();
	for(int i = 0; i < faces; ++i) //each face will be treated as a separate Wavefront object
	{
		Add(Wavefront(&vol->getVolumeFace(i), transform, transform_normals));
	}
}
void WavefrontSaver::Add(const LLViewerObject* some_vo)
{
	LLXform v_form;
	v_form.setScale(some_vo->getScale());
	v_form.setPosition(some_vo->getRenderPosition());
	v_form.setRotation(some_vo->getRenderRotation());

	LLXform normfix;
	normfix.setRotation(v_form.getRotation()); //Should work...
	Add(some_vo->getVolume(), &v_form, &normfix);
}

namespace
{
	class LLSaveSelectedObjects : public view_listener_t
	{
		bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
		{
			if (LLObjectSelectionHandle selection = LLSelectMgr::getInstance()->getSelection())
			{
				WavefrontSaver* wfsaver = new WavefrontSaver; //deleted in callback
				wfsaver->offset = -selection->getFirstRootObject()->getRenderPosition();
				for (LLObjectSelection::iterator iter = selection->begin(); iter != selection->end(); ++iter)
				{
					LLSelectNode* node = *iter;
					wfsaver->Add(node->getObject());
				}

				AIFilePicker* filepicker = AIFilePicker::create();
				filepicker->open(selection->getFirstNode()->mName.c_str()+OBJ);
				filepicker->run(boost::bind(&save_wavefront_continued, wfsaver, filepicker));
			}
			return true;
		}
	};
}

void WavefrontSaver::Add(const LLVOAvatar* av_vo) //adds attachments, too!
{
	offset = -av_vo->getRenderPosition();
	avatar_joint_list_t vjv = av_vo->getMeshLOD();
	for (avatar_joint_list_t::const_iterator itervj = vjv.begin(); itervj != vjv.end(); ++itervj)
	{
		const LLViewerJoint* vj = dynamic_cast<LLViewerJoint*>(*itervj);
		if (!vj || vj->mMeshParts.empty()) continue;

		LLViewerJointMesh* vjm = dynamic_cast<LLViewerJointMesh*>(vj->mMeshParts[0]); //highest LOD
		if (!vjm) continue;

		vjm->updateJointGeometry();
		LLFace* face = vjm->getFace();
		if (!face) continue;

		//Beware: this is a hack because LLFace has multiple LODs
		//'pm' supplies the right number of vertices and triangles!
		LLPolyMesh* pm = vjm->getMesh();
		if (!pm) continue;
		LLXform normfix;
		normfix.setRotation(pm->getRotation());

		//Special case for balls...I mean eyeballs!
		static const std::string eyeLname = LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::getInstance()->getMeshEntry(LLAvatarAppearanceDefines::MESH_ID_EYEBALL_LEFT)->mName;
		static const std::string eyeRname = LLAvatarAppearanceDefines::LLAvatarAppearanceDictionary::getInstance()->getMeshEntry(LLAvatarAppearanceDefines::MESH_ID_EYEBALL_RIGHT)->mName;
		const std::string name = vj->getName();
		if (name == eyeLname || name == eyeRname)
		{
			LLXform lol;
			lol.setPosition(-offset);
			Add(Wavefront(face, pm, &lol, &normfix));
		}
		else
			Add(Wavefront(face, pm, NULL, &normfix));
	}

	for (LLVOAvatar::attachment_map_t::const_iterator iter = av_vo->mAttachmentPoints.begin(); iter != av_vo->mAttachmentPoints.end(); ++iter)
	{
		LLViewerJointAttachment* ja = iter->second;
		if (!ja) continue;

		for (LLViewerJointAttachment::attachedobjs_vec_t::iterator itero = ja->mAttachedObjects.begin(); itero != ja->mAttachedObjects.end(); ++itero)
		{
			LLViewerObject* o = *itero;
			if (!o) continue;

			LLDynamicArray<LLViewerObject*> prims = LLDynamicArray<LLViewerObject*>();
			o->addThisAndAllChildren(prims);
			for (LLDynamicArray<LLViewerObject*>::iterator iterc = prims.begin(); iterc != prims.end(); ++iterc)
			{
				const LLViewerObject* c = *iterc;
				if (!c) continue;
				const LLVolume* vol = c->getVolume();
				if (!vol) continue;

				LLXform v_form;
				v_form.setScale(c->getScale());
				v_form.setPosition(c->getRenderPosition());
				v_form.setRotation(c->getRenderRotation());

				LLXform normfix;
				normfix.setRotation(v_form.getRotation());

				if (c->isHUDAttachment())
				{
					v_form.addPosition(-offset);
					//Normals of HUD elements are funky
					//TO-DO: fix 'em!
				}
				Add(vol, &v_form, &normfix);
			}
		}
	}
}

namespace
{
	class LLSaveSelectedAvatar : public view_listener_t
	{
		bool handleEvent(LLPointer<LLOldEvents::LLEvent> event, const LLSD& userdata)
		{
			if (const LLVOAvatar* avatar = find_avatar_from_object(LLSelectMgr::getInstance()->getSelection()->getPrimaryObject()))
			{
				WavefrontSaver* wfsaver = new WavefrontSaver; //deleted in callback
				wfsaver->Add(avatar);

				AIFilePicker* filepicker = AIFilePicker::create();
				filepicker->open(avatar->getFullname()+OBJ);
				filepicker->run(boost::bind(save_wavefront_continued, wfsaver, filepicker));
			}
			return true;
		}
	};
}

namespace
{
	void write_or_bust(LLFILE* fp, const std::string outstring)
	{
		const size_t size = outstring.length();
		if (fwrite(outstring.c_str(), 1, size, fp) != size)
			llwarns << "Short write" << llendl;
	}
}

bool WavefrontSaver::saveFile(LLFILE* fp)
{
	if (!fp) return false;

	int num = 0;
	int index = 0;
	for (std::vector<Wavefront>::iterator w_iter = obj_v.begin(); w_iter != obj_v.end(); ++w_iter)
	{
		int count = 0;

		std::string name = (*w_iter).name;
		if (name.empty()) name = llformat("%d", num++);

		vert_t vertices = (*w_iter).vertices;
		vec3_t normals = (*w_iter).normals;
		tri_t triangles = (*w_iter).triangles;

		//Write Object
		write_or_bust(fp, "o " + name + "\n");

		//Write vertices; swap axes if necessary
		static const LLCachedControl<bool> swapYZ("OBJExportSwapYZ", false);
		const double xm = swapYZ ? -1.0 : 1.0;
		const int y = swapYZ ? 2 : 1;
		const int z = swapYZ ? 1 : 2;
		for (vert_t::iterator v_iter = vertices.begin(); v_iter != vertices.end(); ++v_iter)
		{
			++count;
			const LLVector3 v = v_iter->first + offset;
			write_or_bust(fp, llformat("v %f %f %f\n",v[0] * xm, v[y], v[z]));
		}

		for (vec3_t::iterator n_iter = normals.begin(); n_iter != normals.end(); ++n_iter)
		{
			const LLVector3 n = *n_iter;
			write_or_bust(fp, llformat("vn %f %f %f\n",n[0] * xm, n[y], n[z]));
		}

		for (vert_t::iterator v_iter = vertices.begin(); v_iter != vertices.end(); ++v_iter)
		{
			write_or_bust(fp, llformat("vt %f %f\n", v_iter->second[0], v_iter->second[1]));
		}

		//Write triangles
		for (tri_t::iterator t_iter = triangles.begin(); t_iter != triangles.end(); ++t_iter)
		{
			const int f1 = t_iter->v0 + index + 1;
			const int f2 = t_iter->v1 + index + 1;
			const int f3 = t_iter->v2 + index + 1;
			write_or_bust(fp, llformat("f %d/%d/%d %d/%d/%d %d/%d/%d\n",
										  f1,f1,f1,f2,f2,f2,f3,f3,f3));
		}
		index += count;
	}

	return true;
}

void addMenu(view_listener_t* menu, const std::string& name);
void add_wave_listeners() //Called in llviewermenu with other addMenu calls, function linked against
{
	addMenu(new LLSaveSelectedObjects(), "Object.SaveAsOBJ");
	addMenu(new LLSaveSelectedAvatar(), "Avatar.SaveAsOBJ");
}

