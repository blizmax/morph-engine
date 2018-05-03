﻿#include "Shader.hpp"
#include "Engine/File/FileSystem.hpp"
#include "Engine/Persistence/yaml.hpp"
#include "Engine/Debug/ErrorWarningAssert.hpp"
#include "Engine/Renderer/Shader/ShaderPass.hpp"

Shader::~Shader() {
  for(ShaderPass* pass: mPasses) {
    SAFE_DELETE(pass);
  }   
}

/* implemented version:
x name: shader/example
program:
  path:
  define: defines;seperated;by;semicolons

layer: 0
sort: 1
cull: back|front|none
fill: solid|wire
frontface: ccw|cw
blend: false
blend:
  color:
    op: add|sub|rev_sub|min|max
    src: one|zero|src_color|inv_src_color|src_alpha|inv_src_alpha|dest_color|inv_dest_color|dest_alpha|inv_dest_alpha|constant|inv_constant
    dest: zero
depth:
  write: true|false
  test: less|never|equal|lequal|greater|gequal|not|always
 */
owner<Shader*> Shader::fromYaml(const fs::path& file) {
  auto vfs = FileSystem::Get();

  std::optional<Blob> f = vfs.asBuffer(file);

  if(!f) {
    return nullptr;
  }

  yaml::Node node = yaml::Load(f->as<const char*>());

  Shader* shader = new Shader();
  
  shader = node.as<Shader*>();

  return shader;
}

template<>
ResDef<Shader> Resource<Shader>::load(const std::string& file) {
  Shader* shader = Shader::fromYaml(file);

  return { shader->name, shader };
}

bool YAML::convert<Shader*>::decode(const Node& node, Shader*& shader) {
  shader = new Shader();
  shader->name = node["name"] ? node["name"].as<std::string>() : "";

  auto passes = node["pass"];
  EXPECTS(passes.IsSequence() && passes.size() >= 1);

  for (auto& pass : passes) {
    ShaderPass* p = new ShaderPass(pass.as<ShaderPass>());
    shader->mPasses.push_back(p);
  }

  return true;
}

