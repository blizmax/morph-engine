#include "Engine/Core/common.hpp"
#include "Engine/Math/Transform.hpp"
#include "Engine/Renderer/type.h"

class Rgba;

class Light {
public:
  Transform transform;
  light_info_t info;

  void asDirectionalLight(float intensity = 1.f, const vec3& attenuation = vec3(0, 0, 1), 
                          const Rgba& color = Rgba::white);

  void asPointLight(float intensity = 1.f, const vec3& attenuation = vec3(0, 0, 1), 
                    const Rgba& color = Rgba::white);

  void asSpotLight(float innerAngle, float outerAngle,
                   float intensity = 1.f, const vec3& attenuation = vec3(0, 0, 1),
                   const Rgba& color = Rgba::white);

protected:
};