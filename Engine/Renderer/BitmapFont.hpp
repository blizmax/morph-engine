﻿#pragma once
#include <string>
#include "Engine/Core/EngineCommon.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"

class aabb2;
class BitmapFont {
  friend class Renderer;

public:
  const std::string m_fontName;
  
  aabb2 getUVsForGlyph(int glyphUnicode) const; // pass ‘A’ or 65 for A, etc.
  float getStringWidth(const std::string& asciiText, float cellHeight, float aspectScale = 1.f) const;
  float getCharWidth(float cellHeight, char c = 'A', float aspectScale = 1);
  float getGlyphAspect(int glyphUnicode) const { UNUSED(glyphUnicode);  return m_baseAspect; } // will change later
  int maxCharacterInWidth(float spaceWidth, float cellHeight, float aspectScale = 1.f) const;
  inline const Texture& getTexture() const { return m_spriteSheet.getTexture(); }

  static void setDefaultFont(BitmapFont& font);
  static BitmapFont* getDefaultFont();
private:
  explicit BitmapFont(const std::string& fontName, const SpriteSheet& glyphSheet,
                      float baseAspect = 1.f); // private, can only called by Renderer (friend class)
  ~BitmapFont();
private:
  const SpriteSheet&	m_spriteSheet; // used internally; assumed to be a 16x16 glyph sheet
  float	m_baseAspect = 1.0f; // used as the base aspect ratio for all glyphs

};
