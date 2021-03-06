﻿#define TINYOBJLOADER_IMPLEMENTATION
#include "Engine/Core/common.hpp"
#include "ThirdParty/tinyobjloader/tiny_obj_loader.h"
#include "Mesher.hpp"
#include "Mesh.hpp"
//#include "Engine/Renderer/Font.hpp"
#include "ThirdParty/mikktspace/mikktspace.h"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/Primitives/ivec2.hpp"
#include "Engine/Math/Primitives/AABB2.hpp"
#include "Engine/Graphics/Font.hpp"
#include "Engine/File/FileSystem.hpp"
#include "Engine/Debug/Log.hpp"
#include "Engine/Graphics/Model/BVH.hpp"

class MikktBinding : public SMikkTSpaceContext {
public:
  MikktBinding(Mesher* mesher);
  void genTangent();
  static int mikkGetNumFace(const SMikkTSpaceContext* pContext);

  static int mikktGetNumVerticesOfFace(const SMikkTSpaceContext* pContext, const int iFace);

  static void mikktGetPosition(const SMikkTSpaceContext* pContext, float out[], const int iFace, const int iVert);

  static void mikktGetNormal(const SMikkTSpaceContext* pContext, float out[], const int iFace, const int iVert);

  static void mikktGetTexCoords(const SMikkTSpaceContext* pContext, float out[], const int iFace, const int iVert);

  static void mikktSetTSpaceBasic(const SMikkTSpaceContext* pContext,
                                  const float               fvTangent[],
                                  const float               fSign,
                                  const int                 iFace,
                                  const int                 iVert);;

  static void mikktSetTSpace(const SMikkTSpaceContext* pContext,
                             const float               fvTangent[],
                             const float               fvBiTangent[],
                             const float               fMagS,
                             const float               fMagT,
                             const tbool               bIsOrientationPreserving,
                             const int                 iFace,
                             const int                 iVert);

};

MikktBinding::MikktBinding(class Mesher* mesher) {
  m_pUserData = mesher;
  m_pInterface = new SMikkTSpaceInterface{
    MikktBinding::mikkGetNumFace,
    MikktBinding::mikktGetNumVerticesOfFace,
    MikktBinding::mikktGetPosition,
    MikktBinding::mikktGetNormal,
    MikktBinding::mikktGetTexCoords,
    MikktBinding::mikktSetTSpaceBasic,
    MikktBinding::mikktSetTSpace,
  };
}

void MikktBinding::genTangent() {
  bool result = genTangSpaceDefault(this);
  UNUSED(result);
  ENSURES(result);
}

int MikktBinding::mikkGetNumFace(const SMikkTSpaceContext* pContext) {
  Mesher& mesher = *(Mesher*)pContext->m_pUserData;
  if (mesher.mCurrentIns.prim != DRAW_TRIANGES) return 0;

  return mesher.mCurrentIns.startIndex + mesher.currentElementCount() / 3;
}

int MikktBinding::mikktGetNumVerticesOfFace(const SMikkTSpaceContext* /*pContext*/, const int /*iFace*/) {
  return 3;
}

void MikktBinding::mikktGetPosition(const SMikkTSpaceContext* pContext, float out[], const int iFace, const int iVert) {
  Mesher& mesher = *(Mesher*)pContext->m_pUserData;

  int index = mesher.mCurrentIns.startIndex + 3 * iFace + iVert;

  index = mesher.mCurrentIns.useIndices ? mesher.mIndices[index] : index;

  vec3 pos = mesher.mVertices.vertices().position[index];

  out[0] = pos.x;
  out[1] = pos.y;
  out[2] = pos.z;
}

void MikktBinding::mikktGetNormal(const SMikkTSpaceContext* pContext, float out[], const int iFace, const int iVert) {
  Mesher& mesher = *(Mesher*)pContext->m_pUserData;

  int index = mesher.mCurrentIns.startIndex + 3 * iFace + iVert;

  index = mesher.mCurrentIns.useIndices ? mesher.mIndices[index] : index;

  vec3 norm = mesher.mVertices.vertices().normal[index];

  out[0] = norm.x;
  out[1] = norm.y;
  out[2] = norm.z;
}

void MikktBinding::mikktGetTexCoords(const SMikkTSpaceContext* pContext, float out[], const int iFace, const int iVert) {
  Mesher& mesher = *(Mesher*)pContext->m_pUserData;

  int index = mesher.mCurrentIns.startIndex + 3 * iFace + iVert;

  index = mesher.mCurrentIns.useIndices ? mesher.mIndices[index] : index;

  vec2 uv = mesher.mVertices.vertices().uv[index];

  out[0] = uv.x;
  out[1] = uv.y;
}

void MikktBinding::mikktSetTSpaceBasic(const SMikkTSpaceContext* pContext,
                                       const float               fvTangent[],
                                       const float               fSign,
                                       const int                 iFace,
                                       const int                 iVert) {
  Mesher& mesher = *(Mesher*)pContext->m_pUserData;

  int index = mesher.mCurrentIns.startIndex + 3 * iFace + iVert;

  index = mesher.mCurrentIns.useIndices ? mesher.mIndices[index] : index;

  vec4& tangent = mesher.mVertices.vertices().tangent[index];

  tangent.x = fvTangent[0];
  tangent.y = fvTangent[1];
  tangent.z = fvTangent[2];
  tangent.w = fSign;
}

void MikktBinding::mikktSetTSpace(const SMikkTSpaceContext* pContext,
                                  const float               fvTangent[],
                                  const float[]             /*fvBiTangent[]*/,
                                  const float               /*fMagS*/,
                                  const float               /*fMagT*/,
                                  const tbool               bIsOrientationPreserving,
                                  const int                 iFace,
                                  const int                 iVert) {
  Mesher& mesher = *(Mesher*)pContext->m_pUserData;

  int index = mesher.mCurrentIns.startIndex + 3 * iFace + iVert;

  index = mesher.mCurrentIns.useIndices ? mesher.mIndices[index] : index;

  vec4& tangent = mesher.mVertices.vertices().tangent[index];

  tangent.x = fvTangent[0];
  tangent.y = fvTangent[1];
  tangent.z = fvTangent[2];
  tangent.w = (float)bIsOrientationPreserving;
}

void Mesher::reserve(size_t size) {
  mVertices.reserve((uint)size);
  mIndices.reserve(size);
}

Mesher& Mesher::begin(eDrawPrimitive prim, bool useIndices) {
  GUARANTEE_OR_DIE(isDrawing == false, "Call begin before previous end get called.");
  mCurrentIns.prim = prim;
  mCurrentIns.useIndices = useIndices;

  if (useIndices) {
    mCurrentIns.startIndex = (uint)mIndices.size();
  } else {
    mCurrentIns.startIndex = (uint)mVertices.count();
  }

  isDrawing = true;
  return *this;
}

void Mesher::end() {
  GUARANTEE_OR_DIE(isDrawing, "Call end without calling begin before");
  uint end = currentElementCount();

  mCurrentIns.elementCount = end - mCurrentIns.startIndex;
  mIns.push_back(mCurrentIns);
  isDrawing = false;
}

void Mesher::clear() {
  mVertices.clear();
  mIndices.clear();
  mIns.clear();
  mCurrentIns = draw_instr_t();
}
Mesher& Mesher::color(const Rgba& c) {
  mStamp.color = c.normalized();
  return *this;
}

Mesher& Mesher::color(const vec4& c) {
  mStamp.color = c;
  return *this;
}

Mesher& Mesher::normal(const vec3& n) {
  mStamp.normal = n.normalized();
  return *this;
}

Mesher& Mesher::tangent(const vec3& t, float fSign) {
  EXPECTS(fSign == 1.f || fSign == -1.f);
  mStamp.tangent = vec4(t.normalized(), fSign);
  return *this;
}

Mesher& Mesher::uv(const vec2& uv) {
  mStamp.uv = uv;
  return *this;
}

Mesher& Mesher::uv(float u, float v) {
  mStamp.uv.x = u;
  mStamp.uv.y = v;
  return *this;
}

uint Mesher::vertex3f(const vec3& pos) {
  mStamp.position = pos;
  mVertices.push(mStamp);
  return mVertices.count() - 1u;
}

uint Mesher::vertex3f(span<const vec3> verts) {
  mVertices.reserve(mVertices.count() + (uint)verts.size());
  for (const vec3& vert : verts) {
    vertex3f(vert);
  }
  return mVertices.count() - (uint)verts.size();
}

uint Mesher::vertex3f(float x, float y, float z) {
  mStamp.position.x = x;
  mStamp.position.y = y;
  mStamp.position.z = z;

  mVertices.push(mStamp);
  return mVertices.count() - 1u;
}

uint Mesher::vertex2f(const vec2& pos) {
  return vertex3f(pos.x, pos.y, 0);
}

Mesher& Mesher::genNormal() {
  uint end = currentElementCount();

  vec3* normals = mVertices.vertices().normal;

  if (mCurrentIns.prim == DRAW_TRIANGES) {
    if (mCurrentIns.useIndices) {
      for (auto i = mCurrentIns.startIndex; i + 2 < end; i += 3) {
        vec3 normal = normalOf(mIndices[i], mIndices[i + 1], mIndices[i + 2]);
        normals[mIndices[i]] += normal;
        normals[mIndices[i + 1]] += normal;
        normals[mIndices[i + 2]] += normal;
        //        ENSURES(normals[i].magnitudeSquared() != 0);
      }
    } else {
      for (auto i = mCurrentIns.startIndex; i + 2 < end; i += 3) {
        vec3 normal = normalOf(i, i + 1, i + 2);
        normals[i] += normal;
        normals[i + 1] += normal;
        normals[i + 2] += normal;
      }
    }

    for (auto i = mCurrentIns.startIndex; i < mVertices.count(); i++) {
      if (normals[i].magnitude2() != 0) {
        normals[i].normalize();
      }
    }
  }

  return *this;
}

Mesher& Mesher::line(const vec3& from, const vec3& to) {
  uint start = vertex3f(from);
  vertex3f(to);

  mIndices.push_back(start);
  mIndices.push_back(start + 1);

  return *this;
}

Mesher& Mesher::line2(const vec2& from, const vec2& to, float z) {
  return line({ from, z }, { to, z });
}

Mesher& Mesher::sphere(const vec3& center, float size, uint levelX, uint levelY) {
  //  mIns.prim = DRAW_POINTS;
  //  mIns.useIndices = false;
  float dTheta = 360.f / (float)levelX;
  float dPhi = 180.f / (float)levelY;

  uint start = mVertices.count();

  for (uint j = 0; j <= levelY; j++) {
    for (uint i = 0; i <= levelX; i++) {
      float phi = dPhi * (float)j - 90.f, theta = dTheta * (float)i;
      vec3 pos = fromSpherical(size, theta, phi) + center;
      uv(theta / 360.f, (phi + 90.f) / 180.f);
      normal(pos - center);
      tangent({ -sinDegrees(theta), 0, cosDegrees(theta) }, 1);
      vertex3f(pos);
    }
  }

  for (uint j = 0; j < levelY; j++) {
    for (uint i = 0; i < levelX; i++) {
      uint current = start + j * (levelX + 1) + i;
      //      triangle(current, current + 1, current + levelX + 1);
      //      triangle(current, current + levelX + 1, current + levelX);
      quad(current, current + 1, current + levelX + 1 + 1, current + levelX + 1);
    }
  }

  return *this;
}

Mesher& Mesher::triangle() {
  EXPECTS(mCurrentIns.useIndices);
  uint last = mVertices.count() - 1;

  if (last < 2) {
    ERROR_RECOVERABLE("cannot construct triangle with less than 4 vertices");
  } else {
    triangle(last - 2, last - 1, last);
  }

  return *this;
}

Mesher& Mesher::triangle(uint a, uint b, uint c) {
  ASSERT_OR_DIE(mCurrentIns.useIndices, "use indices!");
  switch (mCurrentIns.prim) {
    case DRAW_POINTS:
    case DRAW_TRIANGES:
    {
      mIndices.push_back(a);
      mIndices.push_back(b);
      mIndices.push_back(c);
    } break;
    case DRAW_LINES:
    {
      mIndices.push_back(a);
      mIndices.push_back(b);
      mIndices.push_back(b);
      mIndices.push_back(c);
      mIndices.push_back(c);
      mIndices.push_back(a);
    } break;
    default:
      ERROR_AND_DIE("unsupported primitive");
  }

  return *this;
}

Mesher& Mesher::quad() {
  EXPECTS(mCurrentIns.useIndices);
  uint last = mVertices.count() - 1;

  if (last < 3) {
    ERROR_RECOVERABLE("cannot construct quad with less than 4 vertices");
  } else {
    quad(last - 3, last - 2, last - 1, last);
  }

  return *this;
}

Mesher& Mesher::quad(uint a, uint b, uint c, uint d) {
  ASSERT_OR_DIE(mCurrentIns.useIndices, "use indices!");
  switch (mCurrentIns.prim) {
    case DRAW_POINTS:
    {
      mIndices.push_back(a);
      mIndices.push_back(b);
      mIndices.push_back(c);
      mIndices.push_back(d);
    } break;
    case DRAW_LINES:
    {
      mIndices.push_back(a);
      mIndices.push_back(b);
      mIndices.push_back(b);
      mIndices.push_back(c);
      mIndices.push_back(c);
      mIndices.push_back(d);
      mIndices.push_back(d);
      mIndices.push_back(a);
    } break;
    case DRAW_TRIANGES:
    {
      triangle(a, b, c);
      triangle(a, c, d);
    } break;
    default:
      ERROR_AND_DIE("unsupported primitive");
  }

  return *this;
}

Mesher& Mesher::quad(const vec3& a, const vec3& b, const vec3& c, const vec3& d) {
  tangent(b - a, 1);
  if(mWindOrder == WIND_COUNTER_CLOCKWISE) {
    normal((d - a).cross(b - a));
  } else {
    normal((b - a).cross(d - a));
  }
  if(mCurrentIns.useIndices) {
    uint start =
      uv({ 0,0 })
      .vertex3f(a);

    uv({ 1,0 })
      .vertex3f(b);
    uv({ 1,1 })
      .vertex3f(c);
    uv({ 0,1 })
      .vertex3f(d);

    quad(start + 0, start + 1, start + 2, start + 3);
  } else {
    uv({ 0,0 })
      .vertex3f(a);
    uv({ 1,0 })
      .vertex3f(b);
    uv({ 1,1 })
      .vertex3f(c);

    uv({ 0,0 })
      .vertex3f(a);
    uv({ 1,1 })
      .vertex3f(c);
    uv({ 0,1 })
      .vertex3f(d);
  }

  return *this;
}

Mesher& Mesher::quad(const vec3& a, const vec3& b, const vec3& c, const vec3& d,
  const vec2& uva, const vec2& uvb, const vec2& uvc, const vec2& uvd) {
  tangent(b - a, 1);
  if(mWindOrder == WIND_COUNTER_CLOCKWISE) {
    normal((d - a).cross(b - a));
  } else {
    normal((b - a).cross(d - a));
  }
  if(mCurrentIns.useIndices) {
    uint start =
      uv(uva)
      .vertex3f(a);

    uv(uvb)
      .vertex3f(b);
    uv(uvc)
      .vertex3f(c);
    uv(uvd)
      .vertex3f(d);

    quad(start + 0, start + 1, start + 2, start + 3);
  } else {
    uv(uva)
      .vertex3f(a);
    uv(uvb)
      .vertex3f(b);
    uv(uvc)
      .vertex3f(c);

    uv(uva)
      .vertex3f(a);
    uv(uvc)
      .vertex3f(c);
    uv(uvd)
      .vertex3f(d);
  }

  return *this;
}

Mesher& Mesher::quad(const vec3& a, const vec3& b, const vec3& c, const vec3& d, const aabb2& uvs) {
  return quad(a, b, c, d, 
              uvs.mins,
              {uvs.maxs.x, uvs.mins.y},
              uvs.maxs,
              {uvs.mins.x, uvs.maxs.y}
              );
}

Mesher& Mesher::quad(const vec3& center, const vec3& xDir, const vec3& yDir, const vec2& size) {
  vec3 halfX = xDir * size.x * .5f, halfY = yDir * size.y * .5f;
  return quad(center - halfX - halfY,
              center + halfX - halfY,
              center + halfX + halfY,
              center - halfX + halfY);
}

Mesher& Mesher::quad(const vec3& center, const vec3& xDir, const vec3& yDir, const vec2& size,
                     const vec2& uva, const vec2& uvb, const vec2& uvc, const vec2& uvd) {
  vec3 halfX = xDir * size.x * .5f, halfY = yDir * size.y * .5f;
  return quad(center - halfX - halfY,
              center + halfX - halfY,
              center + halfX + halfY,
              center - halfX + halfY,
              uva, uvb, uvc, uvd);
}

Mesher& Mesher::quad2(const aabb2& bound, float z) {
  return quad(vec3{ bound.mins, z },
              vec3{ bound.maxs.x, bound.mins.y, z },
              vec3{ bound.maxs, z },
              vec3{ bound.mins.x, bound.maxs.y, z });
}

Mesher& Mesher::cube(const vec3& center, const vec3& dimension) {
  vec3 bottomCenter = center - vec3::up * dimension.y * .5f;
  float dx = dimension.x * .5f, dy = dimension.y * .5f, dz = dimension.z * .5f;

  std::array<vec3, 8> vertices = {
    bottomCenter + vec3{ -dx, 2.f * dy, -dz },
    bottomCenter + vec3{ dx, 2.f * dy, -dz },
    bottomCenter + vec3{ dx, 2.f * dy,  dz },
    bottomCenter + vec3{ -dx, 2.f * dy,  dz },

    bottomCenter + vec3{ -dx, 0, -dz },
    bottomCenter + vec3{ dx, 0, -dz },
    bottomCenter + vec3{ dx, 0,  dz },
    bottomCenter + vec3{ -dx, 0,  dz }
  };

  quad(vertices[0], vertices[1], vertices[2], vertices[3]);
  quad(vertices[4], vertices[7], vertices[6], vertices[5]);
  quad(vertices[4], vertices[5], vertices[1], vertices[0]);
  quad(vertices[5], vertices[6], vertices[2], vertices[1]);
  quad(vertices[6], vertices[7], vertices[3], vertices[2]);
  quad(vertices[7], vertices[4], vertices[0], vertices[3]);

  return *this;
}

Mesher& Mesher::cube(const vec3& origin, const vec3& dimension, 
                     const vec3& right, const vec3& up, const vec3& forward) {
  vec3 r = right * dimension.x;
  vec3 u = up * dimension.y;
  vec3 f = forward * dimension.z;

  std::array<vec3, 8> vertices = {
    origin + u,
    origin + u + r,
    origin + u + r + f, 
    origin + u + f,

    origin,
    origin + r,
    origin + r + f,
    origin + f,
  };

  quad(vertices[0], vertices[1], vertices[2], vertices[3]);
  quad(vertices[4], vertices[7], vertices[6], vertices[5]);
  quad(vertices[4], vertices[5], vertices[1], vertices[0]);
  quad(vertices[5], vertices[6], vertices[2], vertices[1]);
  quad(vertices[6], vertices[7], vertices[3], vertices[2]);
  quad(vertices[7], vertices[4], vertices[0], vertices[3]);

  return *this;

}

Mesher& Mesher::cone(const vec3& origin, const vec3& direction, float length, float angle, uint slide, bool bottomFace) {

  mat44 trans = mat44::lookAt(origin, origin + direction.normalized() * length);

  uint top = vertex3f((trans * vec4(vec3::zero, 1)).xyz());

  uint bottom = vertex3f((trans * vec4(0, 0, length, 1)).xyz());

  float radius = tanDegree(angle) * length;

  float dAngle = 360.f / (float)slide;

  vertex3f((trans * vec4(radius, 0, length, 1)).xyz());

  for (float i = dAngle; i <= 360.f; i += dAngle) {
    uint current = vertex3f((trans * vec4(cosDegrees(i) * radius, sinDegrees(i) * radius, length, 1)).xyz());

    triangle(top, current - 1, current);
    if (bottomFace) triangle(current - 1, bottom, current);
  }

  return *this;
}

Mesher& Mesher::text(const span<const std::string_view> asciiTexts, float size, const Font* font,
                     const vec3& position, const vec3& right, const vec3& up) {

  vec3 cursor = position;
  vec3 lineStart = cursor;

  for (auto& asciiText : asciiTexts) {
    text(asciiText, size, font, cursor, right, up);
    cursor = lineStart - font->lineHeight(size) * up;
  }

  return *this;
}

Mesher& Mesher::text(const std::string_view asciiText,
                     float size,
                     const Font* font,
                     const vec3& position,
                     const vec3& right,
                     const vec3& up) {
  if (asciiText.empty()) return *this;
  vec3 cursor = position;
  vec3 lineStart = cursor;

  uint i = 0;
  {
    aabb2 bounds = font->bounds(asciiText[i], size);
    vec3 bottomLeft = cursor + bounds.mins.x * right + bounds.mins.y * up;
    vec3 bottomRight = cursor + bounds.maxs.x * right + bounds.mins.y * up;
    vec3 topRight = cursor + bounds.maxs.x * right + bounds.maxs.y * up;
    vec3 topLeft = cursor + bounds.mins.x * right + bounds.maxs.y * up;
    auto uvs = font->uv(asciiText[0]).vertices();
    uint start =
      uv(uvs[0])
      .vertex3f(bottomLeft);
    uv(uvs[1])
      .vertex3f(bottomRight);
    uv(uvs[2])
      .vertex3f(topRight);
    uv(uvs[3])
      .vertex3f(topLeft);
    quad(start, start + 1, start + 2, start + 3);
  }
  //    end();

  cursor += font->advance('\0', asciiText[i], size) * right;
  while (++i < asciiText.size()) {
    aabb2 bounds = font->bounds(asciiText[i], size);
    vec3 bottomLeft = cursor + bounds.mins.x * right + bounds.mins.y * up;
    vec3 bottomRight = cursor + bounds.maxs.x * right + bounds.mins.y * up;
    vec3 topRight = cursor + bounds.maxs.x * right + bounds.maxs.y * up;
    vec3 topLeft = cursor + bounds.mins.x * right + bounds.maxs.y * up;
    auto uvs = font->uv(asciiText[i]).vertices();

    uint start =
      uv(uvs[0])
      .vertex3f(bottomLeft);
    uv(uvs[1])
      .vertex3f(bottomRight);
    uv(uvs[2])
      .vertex3f(topRight);
    uv(uvs[3])
      .vertex3f(topLeft);
    quad(start, start + 1, start + 2, start + 3);

    cursor += font->advance(asciiText[i - 1], asciiText[i], size) * right;
  }

  return *this;
}

Mesher& Mesher::text(const span<const std::string> asciiTexts,
                     float size,
                     const Font* font,
                     const vec3& position,
                     const vec3& right,
                     const vec3& up) {

  vec3 cursor = position;
  vec3 lineStart = cursor;

  for (auto& asciiText : asciiTexts) {
    text(asciiText, size, font, cursor, right, up);
    cursor = lineStart - font->lineHeight(size) * up;
  }

  return *this;
}


void Mesher::mikkt() {
  EXPECTS(!mCurrentIns.useIndices);
  MikktBinding binding(this);
  binding.genTangent();
}


vec3 Mesher::normalOf(uint a, uint b, uint c) {
  vec3* verts = mVertices.vertices().position;

  vec3 ab = verts[b] - verts[a];
  vec3 ac = verts[c] - verts[a];
  if(mWindOrder == WIND_COUNTER_CLOCKWISE) {
    return ac.cross(ab);;
  } else {
    return ab.cross(ac);
  }
  return ac.cross(ab);
}

uint Mesher::currentElementCount() const {
  return  mCurrentIns.useIndices ? (uint)mIndices.size() : mVertices.count();
}


void Mesher::surfacePatch(const delegate<vec3(const vec2&)>& parametric, float eps) {
  return surfacePatch({ 0.f, 1.f }, { 0.f, 1.f }, 10u, 10u, parametric, eps);
}

void Mesher::surfacePatch(const FloatRange& u, const FloatRange& v, uint levelX, uint levelY, const std::function<vec3(const vec2&)>& parametric, float eps) {
  return surfacePatch([parametric](const vec2& pos, auto...) {
    return parametric(pos);
  }, u, v, levelX, levelY, eps);
}

void Mesher::surfacePatch(const std::function<vec3(const vec2&, const ivec2&)>& parametric,
                          const FloatRange& u,
                          const FloatRange& v,
                          uint levelX,
                          uint levelY, float eps) {
  uint start = mVertices.count();

  float stepX = u.length() / float(levelX);
  float stepY = v.length() / float(levelY);

  for (int j = 0; j <= (int)levelY; j++) {
    for (int i = 0; i <= (int)levelX; i++) {
      float x = u.min + stepX * (float)i;
      float y = v.min + stepY * (float)j;
      uv(x, y);
      vec3 tan = (parametric({ x + eps, y }, { i + 1, j }) - parametric({ x - eps, y }, { i - 1, j })) / (2 * eps);
      vec3 bitan = (parametric({ x, y + eps }, { i, j + 1 }) - parametric({ x, y - eps }, { i, j - 1 })) / (2 * eps);
      if (tan.magnitude() >= eps) {
        tangent(tan);
        normal(bitan.cross(tan));
      }
      vertex3f(parametric({ x,y }, { i,j }));
    }
  }

  for (uint j = 0; j < levelY; j++) {
    for (uint i = 0; i < levelX; i++) {
      uint current = start + j * (levelX + 1) + i;
      quad(current, current + 1, current + levelX + 1 + 1, current + levelX + 1);
    }
  }
}

//
//Mesher& Mesher::sphere(const vec3& center, float size, uint levelX, uint levelY) {
//  //  mIns.prim = DRAW_POINTS;
//  //  mIns.useIndices = false;
//  float dTheta = 360.f / (float)levelX;
//  float dPhi = 180.f / (float)levelY;
//
//  uint start = mVertices.count();
//
//  for (uint j = 0; j <= levelY; j++) {
//    for (uint i = 0; i <= levelX; i++) {
//      float phi = dPhi * (float)j - 90.f, theta = dTheta * (float)i;
//      vec3 pos = fromSpherical(size, theta, phi) + center;
//      uv(theta / 360.f, (phi + 90.f) / 180.f);
//      normal(pos - center);
//      tangent({ -sinDegrees(theta), 0, cosDegrees(theta) }, 1);
//      vertex3f(pos);
//    }
//  }
//
//  for (uint j = 0; j < levelY; j++) {
//    for (uint i = 0; i < levelX; i++) {
//      uint current = start + j * (levelX + 1) + i;
//      //      triangle(current, current + 1, current + levelX + 1);
//      //      triangle(current, current + levelX + 1, current + levelX);
//      quad(current, current + 1, current + levelX + 1 + 1, current + levelX + 1);
//    }
//  }
//
//  return *this;
//}


// blender exporter: z-fwd, y-up
void Mesher::obj(fs::path objFile) {
  tinyobj::attrib_t attrib;
  std::vector<tinyobj::shape_t> shapes;
  std::vector<tinyobj::material_t> materials;

  std::string err;

  auto file = FileSystem::Get().locate(objFile);

  if(!file) {
    Log::errorf("Fail to load obj file, cannot find %s", objFile.generic_string().c_str());
    Log::errorf("==> traslated as: %s", file->generic_string().c_str());
  }
  bool ret = tinyobj::LoadObj(
    &attrib, &shapes, &materials, &err, file->generic_string().c_str(), nullptr, true);

  if(!err.empty()) {
    Log::warnf("Obj Loading Warning: %s", err.c_str());
  }

  if(!ret) {
    Log::errorf("fail to load object from file: %s", objFile.c_str());
  }

  for(size_t s = 0; s<shapes.size(); s++) {
    size_t indexOffset = 0;

    for(size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++) {
      int fv = shapes[s].mesh.num_face_vertices[f];

      EXPECTS(fv % 3 == 0);
      for(size_t v = 0; v < fv; v+=3) {
        vec3 v1, v2, v3;
        vec3 n1, n2, n3;
        vec2 t1, t2, t3;

        {
          tinyobj::index_t idx = shapes[s].mesh.indices[indexOffset + v];

          v1.x = -attrib.vertices[3 * idx.vertex_index + 0];
          v1.y = attrib.vertices[3 * idx.vertex_index + 1];
          v1.z = attrib.vertices[3 * idx.vertex_index + 2];

          if(!attrib.texcoords.empty()) {
            t1.x = attrib.texcoords[2 * idx.texcoord_index + 0];
            t1.y = attrib.texcoords[2 * idx.texcoord_index + 1];
          }
          if(!attrib.normals.empty()) {
             n1.x =  -attrib.normals[3 * idx.normal_index + 0];
             n1.y = attrib.normals[3 * idx.normal_index + 1];
             n1.z = attrib.normals[3 * idx.normal_index + 2];
          }
        }

        {
          tinyobj::index_t idx = shapes[s].mesh.indices[indexOffset + v + 1];

          v2.x = -attrib.vertices[3 * idx.vertex_index + 0];
          v2.y = attrib.vertices[3 * idx.vertex_index + 1];
          v2.z = attrib.vertices[3 * idx.vertex_index + 2];

          if (!attrib.texcoords.empty()) {
            t2.x = attrib.texcoords[2 * idx.texcoord_index + 0];
            t2.y = attrib.texcoords[2 * idx.texcoord_index + 1];
          }
          if(!attrib.normals.empty()) {
             n2.x =  -attrib.normals[3 * idx.normal_index + 0];
             n2.y = attrib.normals[3 * idx.normal_index + 1];
             n2.z = attrib.normals[3 * idx.normal_index + 2];
          }
        }

        {
          tinyobj::index_t idx = shapes[s].mesh.indices[indexOffset + v + 2];

          v3.x = -attrib.vertices[3 * idx.vertex_index + 0];
          v3.y = attrib.vertices[3 * idx.vertex_index + 1];
          v3.z = attrib.vertices[3 * idx.vertex_index + 2];

          if (!attrib.texcoords.empty()) {
            t3.x = attrib.texcoords[2 * idx.texcoord_index + 0];
            t3.y = attrib.texcoords[2 * idx.texcoord_index + 1];
          }
          if(!attrib.normals.empty()) {
             n3.x =  -attrib.normals[3 * idx.normal_index + 0];
             n3.y = attrib.normals[3 * idx.normal_index + 1];
             n3.z = attrib.normals[3 * idx.normal_index + 2];
          }
        }

        if(attrib.normals.empty()) {
          uv(t1);
          vertex3f(v1);

          uv(t2);
          vertex3f(v2);

          uv(t3);
          vertex3f(v3);
        } else {
          normal(n1);
          uv(t1);
          vertex3f(v1);

          normal(n2);
          uv(t2);
          vertex3f(v2);

          normal(n3);
          uv(t3);
          vertex3f(v3);
        }
      }

      indexOffset += fv;
    }

    if(attrib.normals.empty()) {
      genNormal();
    }


    mikkt();

  }
}

owner<BVH*> Mesher::createBVH(uint maxDepth) {
  BVH* bvh = new BVH(
    { mVertices.vertices().position, mVertices.count() },
    { mVertices.vertices().color, mVertices.count() }, maxDepth);

  return bvh;
}
