﻿#pragma once
#include "Engine/Core/common.hpp"
#include "Engine/Math/Primitives/vec3.hpp"
#include "Engine/Math/Primitives/mat44.hpp"

class mat44;
struct transform_t {
  vec3 position;
  Euler euler;
  vec3 scale;

  transform_t();

  /* provide an SRT matrix
   * The rotation is ZXY rotation
   */
  mat44 localToWorld(eRotationOrder rotationOrder) const;
  mat44 worldToLocal(eRotationOrder rotationOrder) const;
  void set(const mat44& transform, eRotationOrder rotationOrder);

  inline void translate(const vec3& offset) { position += offset; };
  inline void rotate(float x, float y, float z) { euler += vec3(x, y, z); };
  inline void rotate(const Euler& e) { euler += e; };

  static const transform_t IDENTITY;
};

class Transform {
public:

  mat44 localToWorld() const;
  mat44 worldToLocal() const;

  // mutator
  void localRotate(const Euler& euler);
  void localTranslate(const vec3& offset);
  void setlocalTransform(const mat44& transform);
  void setWorldTransform(const mat44& transform);
  void setRotationOrder(eRotationOrder order);
  // accessor
  vec3 forward() const;
  vec3 up() const;
  vec3 right() const;
  
  inline const vec3& localPosition() const { return mLocalTransform.position; }
  inline vec3& localPosition() { return mLocalTransform.position; };

  inline const Euler& localRotation() const { return mLocalTransform.euler; };
  inline Euler& localRotation() { return mLocalTransform.euler; };

  inline const vec3& localScale() const { return mLocalTransform.scale; };
  inline vec3& localScale() { return mLocalTransform.scale; };

  inline const vec3 position() const { return (worldMat() * vec4(mLocalTransform.position, 1.f)).xyz(); };
  inline const Euler rotation() const { return localToWorld().euler(mRotationOrder); };
  inline const vec3 scale() const {
    mat44 model = localToWorld();  
    return { model.x().xyz().magnitude(), 
             model.y().xyz().magnitude(), 
             model.z().xyz().magnitude() };
  };

  vec3 transform(const vec3& pointOrDisp, bool isDisp = false) const;

  const Transform*& parent() { return mParent; }

  static mat44 lookAt(const vec3& position, const vec3& target);

private:
  mat44 worldMat() const;
  mat44 localMat() const;
  transform_t mLocalTransform;
  const Transform* mParent = nullptr;
  eRotationOrder mRotationOrder = ROTATION_ZXY;
};
