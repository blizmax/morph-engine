#include "Engine/Renderer/Renderable/Renderable.hpp"
#include "Engine/Core/Resource.hpp"
#include "Engine/Graphics/Program/Material.hpp"
#include "Engine/Math/Transform.hpp"

Renderable::Renderable(S<const Material> mat, const Mesh& me, const Transform& trans)
  : mResMaterial(std::move(mat))
  , mMaterial(nullptr)
  , mMesh(&me)
  , mTransform(&trans) {
    
}

Renderable::Renderable(Material& mat, const Mesh& me, const Transform& trans)
  : mResMaterial(nullptr)
  , mMaterial(&mat)
  , mMesh(&me)
  , mTransform(&trans) {}

Renderable::~Renderable() {
  SAFE_DELETE(mMaterial);
}

void Renderable::material(S<const Material> mat) {
  delete mMaterial;
  mResMaterial = mat;
}

void Renderable::material(Material& mat) {
  delete mMaterial;
  mMaterial = &mat;
}

// Material* Renderable::material() {
//   if(!mMaterial) {
//     mMaterial = Resource<Material>::clone(mResMaterial);
//   }
//
//   return mMaterial;
// }

const Material* Renderable::material() const {
  return mMaterial == nullptr ? mResMaterial.get() : mMaterial;
}

// void Renderable::pushRenderTask(std::vector<RenderTask>& tasks, const RenderScene& scene, Camera& cam) const {
//   uint lightIndices[NUM_MAX_LIGHTS];
//   uint lightCount = 0;
//
//   if (useLight()) {
//     scene.lightContributorsAt(mTransform->position(),
//                               lightIndices,
//                               &lightCount);
//   }
//   for (int i = 0; i < material()->shader()->passes().size(); i++) {
//     tasks.emplace_back();
//
//     RenderTask& rt = tasks.back();
//
//     rt.camera = &cam;
//     rt.mesh = mesh();
//     rt.transform = mTransform;
//
//     const Material* mat = material();
//     rt.material = mat;
//     rt.passIndex = i;
//     rt.queue = mat->shader()->pass(i).sort;
//     rt.layer = mat->shader()->pass(i).layer;
//
//     memcpy(rt.lightIndices, lightIndices, NUM_MAX_LIGHTS*sizeof(uint));
//     rt.lightCount = lightCount;
//   }
// }
