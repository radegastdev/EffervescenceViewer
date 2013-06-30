/**
 * @file awavefront.h
 * @brief A system which allows saving in-world objects to Wavefront .OBJ files for offline texturizing/shading.
 * @author Apelsin
 *
 * $LicenseInfo:firstyear=2011&license=WTFPLV2$
 *
 */

#ifndef AWAVEFRONT
#define AWAVEFRONT

#include <vector>
#include "v3math.h"
#include "v2math.h"
#include "llface.h"
#include "llvolume.h"

using namespace std;

class LLFace;
class LLPolyMesh;
class LLViewerObject;
class LLVOAvatar;

typedef std::vector<std::pair<LLVector3, LLVector2> > vert_t;
typedef std::vector<LLVector3> vec3_t;

struct tri
{
	tri(int a, int b, int c) : v0(a), v1(b), v2(c) {}
	int v0;
	int v1;
	int v2;
};
typedef std::vector<tri> tri_t;

class Wavefront
{
public:
	vert_t vertices;
	vec3_t normals; //null unless otherwise specified!
	tri_t triangles; //because almost all surfaces in SL are triangles
	std::string name;
	Wavefront(vert_t v, tri_t t);
	Wavefront(const LLVolumeFace* face, const LLXform* transform = NULL, const LLXform* transform_normals = NULL);
	Wavefront(LLFace* face, LLPolyMesh* mesh = NULL, const LLXform* transform = NULL, const LLXform* transform_normals = NULL);
	static void Transform(vert_t& v, const LLXform* x); //helper function
	static void Transform(vec3_t& v, const LLXform* x); //helper function
};

class WavefrontSaver
{
public:
	std::vector<Wavefront> obj_v;
	LLVector3 offset;
	WavefrontSaver();
	void Add(const Wavefront& obj);
	void Add(const LLVolume* vol, const LLXform* transform = NULL, const LLXform* transform_normals = NULL);
	void Add(const LLViewerObject* some_vo);
	void Add(const LLVOAvatar* av_vo);
	bool saveFile(LLFILE* fp);
};

#endif // AWAVEFRONT

