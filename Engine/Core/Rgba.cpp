#pragma once

#include "Engine/Core/Rgba.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Math/MathUtils.hpp"

const Rgba Rgba::white = Rgba(255, 255, 255);
const Rgba Rgba::black = Rgba(0,   0,   0  );
const Rgba Rgba::red   = Rgba(255, 0  , 0  );
const Rgba Rgba::yellow= Rgba(255, 255, 0  );
const Rgba Rgba::cyan  = Rgba(0  , 255, 255);

const Rgba Rgba::transparent = Rgba(255, 255, 255, 0);

Rgba::Rgba(unsigned char redByte, unsigned char greenByte, unsigned char blueByte, unsigned char alphaByte)
	: r(redByte)
	, g(greenByte)
	, b(blueByte)
	, a(alphaByte) {}

void Rgba::setByBytes(unsigned char redByte, unsigned char greenByte, unsigned char blueByte, unsigned char alphaByte) {
	r = redByte;
	g = greenByte;
	b = blueByte;
	a = alphaByte;
}

void Rgba::setByFloats(float normalizedRed, float normalizedGreen, float normalizedBlue, float normalizedAlpha) {
	r = unsigned char(normalizedRed * 255);
	g = unsigned char(normalizedGreen * 255);
	b = unsigned char(normalizedBlue * 255);
	a = unsigned char(normalizedAlpha * 255);
}

void Rgba::getAsFloats(float & out_normalizedRed, float & out_normalizedGreen, float & out_normalizedBlue, float & out_normalizedAlpha) const {
	out_normalizedRed = 1.f / 255.f * (float)r;
	out_normalizedGreen = 1.f / 255.f * (float)g;
	out_normalizedBlue = 1.f / 255.f * (float)b;
	out_normalizedAlpha = 1.f / 255.f * (float)a;
}

void Rgba::scaleColor(float rgbScale) {
	float _r = r * rgbScale;
	float _g = g * rgbScale;
	float _b = b * rgbScale;

	r = unsigned char(clampf(_r, 0, 255));
	g = unsigned char(clampf(_g, 0, 255));
	b = unsigned char(clampf(_b, 0, 255));
}

void Rgba::scaleOpacity(float alphaScale) {
	float _a = a * alphaScale;
	a = unsigned char(clampf(_a, 0, 255));
}

void Rgba::fromRgbString(const char* data) {
  auto rgba = split(data, " ,");

  GUARANTEE_RECOVERABLE(rgba.size() == 3 || rgba.size() == 4, "try to cast illegal string to Rgba");

  if(rgba.size() == 3) {
    setByBytes(
      parse<unsigned char>(rgba[0]), 
      parse<unsigned char>(rgba[1]), 
      parse<unsigned char>(rgba[2]));
  } else if(rgba.size() == 4) {
    setByBytes(
      parse<unsigned char>(rgba[0]), 
      parse<unsigned char>(rgba[1]), 
      parse<unsigned char>(rgba[2]), 
      parse<unsigned char>(rgba[3]));
  }
}

void Rgba::fromHexString(const char* data) {
  // FFAABBFF
  // 01234567
  r = 16 * castHex(data[0]) + castHex(data[1]);
  g = 16 * castHex(data[2]) + castHex(data[3]);
  b = 16 * castHex(data[4]) + castHex(data[5]);

  switch(strlen(data)) {
    case 6:
      a = 255;
    break;
    case 8:
      a = 16 * castHex(data[6]) + castHex(data[7]);
    break;
    default:
      ERROR_AND_DIE("illegal hex string for Rgba");
    break;
  }
}

void Rgba::fromString(const char* data) {
  if(data[0] == '#') {
    fromHexString(data+1);
    return;
  } else {
    fromRgbString(data);
    return;
  }
}

std::string Rgba::toString(bool withAlpha) {
  if(withAlpha) {
    return Stringf("%u,%u,%u,%u", r, g, b, a);
  } else {
    return Stringf("%u,%u,%u", r, g, b);
  }
}

float HueToRGB(float v1, float v2, float vH) {
  if (vH < 0)
    vH += 1;

  if (vH > 1)
    vH -= 1;

  if ((6 * vH) < 1)
    return (v1 + (v2 - v1) * 6 * vH);

  if ((2 * vH) < 1)
    return v2;

  if ((3 * vH) < 2)
    return (v1 + (v2 - v1) * ((2.0f / 3) - vH) * 6);

  return v1;
}

Rgba hsl(float h, float s, float l) {
  unsigned char r = 0;
  unsigned char g = 0;
  unsigned char b = 0;

  if (s == 0) {
    r = g = b = (unsigned char)(l * 255);
  } else {
    float v1, v2;
    float hue = (float)h / 360;

    v2 = (l < 0.5) ? (l * (1 + s)) : ((l + s) - (l * s));
    v1 = 2 * l - v2;

    r = (unsigned char)(255 * HueToRGB(v1, v2, hue + (1.0f / 3)));
    g = (unsigned char)(255 * HueToRGB(v1, v2, hue));
    b = (unsigned char)(255 * HueToRGB(v1, v2, hue - (1.0f / 3)));
  }

  return Rgba(r, g, b);
}

