﻿#include "Engine/Renderer/SpriteAnim.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Debug/ErrorWarningAssert.hpp"
#include "Engine/Math/MathUtils.hpp"
#include <algorithm>
SpriteAnimDefinition::SpriteAnimDefinition(const SpriteSheet& spriteSheet, const Xml& node)
  : m_spriteSheet(spriteSheet) {
  GUARANTEE_OR_DIE(node.name() == "SpriteAnim", "xml node tag is not correct");
  float fps = 0.f;
  fps = node.attribute("fps", fps);
//  GUARANTEE_RECOVERABLE(fps != 0, Stringf("missing fps infomation for %s", node["name"].c_str()));
  if (fps == 0) fps = 1.f;
  m_durationSeconds = 1.f / fps;
  m_frameIndexes = node.attribute("spriteIndexes", m_frameIndexes);
  m_name = node.attribute("name", m_name);
}

SpriteAnimDefinition::SpriteAnimDefinition(const SpriteSheet& spriteSheet, float durationSeconds,
  SpriteAnimMode playbackMode, std::vector<int> frames)
  : m_frameIndexes(frames)
  , m_durationSeconds(durationSeconds)
  , m_spriteSheet(spriteSheet)
  , m_playMode(playbackMode) {
  
}

SpriteAnim::SpriteAnim(const SpriteSheet& spriteSheet, 
                                 float durationSeconds, 
                                 SpriteAnimMode playbackMode,
                                 int startSpriteIndex, 
                                 int endSpriteIndex)
  : m_fromDefinition(false) {
  std::vector<int> frames;
  for(int i = startSpriteIndex; i <= endSpriteIndex; i ++) {
    frames.push_back(i);
  }
  m_definition = new const SpriteAnimDefinition(spriteSheet, durationSeconds, playbackMode, frames);
  
}

SpriteAnim::SpriteAnim(const SpriteAnimDefinition& definition)
  : m_definition(&definition)
  , m_fromDefinition(true) {
}

SpriteAnim::~SpriteAnim() {
  if(!m_fromDefinition) {
    delete m_definition;
  }
  m_definition = nullptr;
}

void SpriteAnim::update(float deltaSeconds) {
  m_elapsedSeconds += deltaSeconds;
  if (m_elapsedSeconds > m_definition->m_durationSeconds) {
    switch (m_definition->m_playMode) {
      case SPRITE_ANIM_MODE_PLAY_TO_END:
        m_isFinished = true;
        break;
      case SPRITE_ANIM_MODE_LOOPING: 
        m_elapsedSeconds -= m_definition->m_durationSeconds;
      break;
      case NUM_SPRITE_ANIM_MODES: 
      break;
    }
  }
}
aabb2 SpriteAnim::getCurrentTexCoords() const {
  float secPerUnit = m_definition->m_durationSeconds / float(m_definition->m_frameIndexes.size());
  int curremtFrameIdx = clamp(int(floor(m_elapsedSeconds / secPerUnit)), 0, (int)m_definition->m_frameIndexes.size() - 1);
  
  return m_definition->m_spriteSheet.getTexCoordsByIndex(m_definition->m_frameIndexes[curremtFrameIdx]);
}
const Texture& SpriteAnim::getTexture() const {
  return m_definition->m_spriteSheet.getTexture();
}
void SpriteAnim::pause() {
  m_isPlaying = false;
}
void SpriteAnim::resume() {
  m_isPlaying = true;
}
void SpriteAnim::reset() {
  m_isPlaying = true;
  m_isFinished = false;
  m_elapsedSeconds = 0;
}
void SpriteAnim::setSecondsElapsed(float secondsElapsed) {
  m_elapsedSeconds = secondsElapsed;
}
void SpriteAnim::setFractionElapsed(float fractionElapsed) {
  m_elapsedSeconds = m_definition->m_durationSeconds * fractionElapsed;
}
