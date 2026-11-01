#include "vidra/gfx/renderer.hpp"
#include "vidra/core/log.hpp"
#include "vidra/tokens.hpp"
#ifndef GL_GLEXT_PROTOTYPES
#define GL_GLEXT_PROTOTYPES 1
#endif
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <cstdio>
#include <cstring>
#include <vector>

namespace vidra::gfx {
namespace {

GLuint compile(GLenum t, const char* src) {
  const GLuint s = glCreateShader(t);
  glShaderSource(s, 1, &src, nullptr);
  glCompileShader(s);
  GLint ok = 0;
  glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char b[2048];
    glGetShaderInfoLog(s, sizeof(b), nullptr, b);
    VIDRA_LOGE("shader: %s", b);
  }
  return s;
}
std::string slurp(const char* p) {
  FILE* f = std::fopen(p, "rb");
  if (!f) return {};
  std::string o;
  char b[4096];
  size_t n;
  while ((n = std::fread(b, 1, sizeof(b), f)) > 0) o.append(b, n);
  std::fclose(f);
  return o;
}
struct Globals {
  float screen[2]; float time; float hover;
  float tint[4]; float surface[4]; float light[4];
  float neighbors[8][4]; float gp[4]; float gp2[4]; float gp3[4];
};
struct QuadU { float box[4]; float color[4]; float uv[4]; float p0[4]; float p1[4]; };

class GLRenderer final : public Renderer {
 public:
  bool init(void* display, uint64_t window, int w, int h, std::string* err) override {
    dpy_ = static_cast<Display*>(display);
    win_ = Window(window);
    if (!dpy_ || !win_) { if (err) *err = "sem display"; return false; }
    int att[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
                 GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8, None};
    XVisualInfo* vi = glXChooseVisual(dpy_, DefaultScreen(dpy_), att);
    if (!vi) { if (err) *err = "sem visual GLX"; return false; }
    ctx_ = glXCreateContext(dpy_, vi, nullptr, True);
    XFree(vi);
    if (!ctx_) { if (err) *err = "sem contexto"; return false; }
    glXMakeCurrent(dpy_, win_, ctx_);

    const std::string common = slurp("shaders/common.glsl");
    const std::string vs = "#version 330 core\n" + common + slurp("shaders/quad.vert");
    const std::string fs = "#version 330 core\n" + common + slurp("shaders/scene.frag");
    const std::string bvs = "#version 330 core\n" + slurp("shaders/blur.vert");
    const std::string bfs = "#version 330 core\n" + slurp("shaders/blur.frag");
    prog_ = glCreateProgram();
    glAttachShader(prog_, compile(GL_VERTEX_SHADER, vs.c_str()));
    glAttachShader(prog_, compile(GL_FRAGMENT_SHADER, fs.c_str()));
    glLinkProgram(prog_);
    blurProg_ = glCreateProgram();
    glAttachShader(blurProg_, compile(GL_VERTEX_SHADER, bvs.c_str()));
    glAttachShader(blurProg_, compile(GL_FRAGMENT_SHADER, bfs.c_str()));
    glLinkProgram(blurProg_);
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glGenTextures(1, &atlas_);
    glBindTexture(GL_TEXTURE_2D, atlas_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1024, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glGenBuffers(1, &gUbo_);
    glBindBuffer(GL_UNIFORM_BUFFER, gUbo_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(Globals), nullptr, GL_DYNAMIC_DRAW);
    glGenBuffers(1, &qUbo_);
    glBindBuffer(GL_UNIFORM_BUFFER, qUbo_);
    glBufferData(GL_UNIFORM_BUFFER, sizeof(QuadU), nullptr, GL_DYNAMIC_DRAW);

    resize(w, h);
    ok_ = prog_ != 0 && fb_ != 0;
    if (!ok_ && err) *err = "pipeline incompleto";
    return ok_;
  }

  void shutdown() override {
    if (atlas_) glDeleteTextures(1, &atlas_);
    if (backdrop_) glDeleteTextures(1, &backdrop_);
    if (fb_) glDeleteFramebuffers(1, &fb_);
    if (prog_) glDeleteProgram(prog_);
    if (blurProg_) glDeleteProgram(blurProg_);
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (gUbo_) glDeleteBuffers(1, &gUbo_);
    if (qUbo_) glDeleteBuffers(1, &qUbo_);
    if (ctx_) { glXMakeCurrent(dpy_, None, nullptr); glXDestroyContext(dpy_, ctx_); ctx_ = nullptr; }
  }

  void resize(int w, int h) override {
    if (w <= 0 || h <= 0) return;
    w_ = w; h_ = h;
    if (!fb_) glGenFramebuffers(1, &fb_);
    if (backdrop_) glDeleteTextures(1, &backdrop_);
    glGenTextures(1, &backdrop_);
    glBindTexture(GL_TEXTURE_2D, backdrop_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glBindFramebuffer(GL_FRAMEBUFFER, fb_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, backdrop_, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    tonal_.clear();
    glass_.clear();
  }

  void beginFrame(float t) override {
    tonal_.clear();
    glass_.clear();
    time_ = t;
    glViewport(0, 0, w_, h_);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glEnable(GL_BLEND);
    glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
  }

  void push(const Quad& q) override { (q.mat == Material::Glass ? glass_ : tonal_).push_back(q); }

  void endFrame() override {
    draw(tonal_, false);
    if (!glass_.empty()) {
      glBindTexture(GL_TEXTURE_2D, backdrop_);
      glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, w_, h_);
      glBindFramebuffer(GL_FRAMEBUFFER, fb_);
      blur();
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
      draw(glass_, true);
    }
    glXSwapBuffers(dpy_, win_);
  }

  BackendKind kind() const override { return BackendKind::OpenGL; }
  bool ok() const override { return ok_; }
  int width() const override { return w_; }
  int height() const override { return h_; }
  void setPalette(const float tint[3], const float surf[3], float hover) override {
    std::memcpy(tint_, tint, sizeof(tint_));
    std::memcpy(surf_, surf, sizeof(surf_));
    hover_ = hover;
  }
  void uploadAtlas(int x, int y, int w, int h, const uint8_t* rgba) override {
    glBindTexture(GL_TEXTURE_2D, atlas_);
    glTexSubImage2D(GL_TEXTURE_2D, 0, x, y, w, h, GL_RGBA, GL_UNSIGNED_BYTE, rgba);
  }

 private:
  void blur() {
    glUseProgram(blurProg_);
    glUniform1i(glGetUniformLocation(blurProg_, "uSrc"), 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backdrop_);
    const GLint d = glGetUniformLocation(blurProg_, "B.dir");
    glUniform1f(glGetUniformLocation(blurProg_, "B.sigma"), 9.0f);
    glUniform2f(d, 1.0f / w_, 0.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glUniform2f(d, 0.0f, 1.0f / h_);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  }

  void draw(const std::vector<Quad>& batch, bool) {
    if (batch.empty()) return;
    glUseProgram(prog_);
    glBindVertexArray(vao_);
    glUniform1i(glGetUniformLocation(prog_, "uBackdrop"), 0);
    glUniform1i(glGetUniformLocation(prog_, "uAtlas"), 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, backdrop_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, atlas_);
    const GLuint gb = glGetUniformBlockIndex(prog_, "Globals");
    if (gb != GL_INVALID_INDEX) glUniformBlockBinding(prog_, gb, 0);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, gUbo_);
    const GLuint qb = glGetUniformBlockIndex(prog_, "QuadU");
    if (qb != GL_INVALID_INDEX) glUniformBlockBinding(prog_, qb, 1);
    glBindBufferBase(GL_UNIFORM_BUFFER, 1, qUbo_);

    Globals g{};
    g.screen[0] = float(w_); g.screen[1] = float(h_); g.time = time_; g.hover = hover_;
    std::memcpy(g.tint, tint_, sizeof(tint_));
    std::memcpy(g.surface, surf_, sizeof(surf_));
    g.light[0] = 0.82f; g.light[1] = -0.57f;
    g.gp[0] = tok::kGlassBevelPx; g.gp[1] = tok::kGlassMaxRefractPx;
    g.gp[2] = tok::kGlassDispersionPx; g.gp[3] = tok::kGlassMetaballBlendPx;
    g.gp2[0] = tok::kGlassRimPx; g.gp2[1] = tok::kGlassRimAlpha;
    g.gp2[2] = tok::kGlassScrimLumaThreshold; g.gp2[3] = tok::kGlassScrimAlpha;
    g.gp3[0] = tok::kGlassBodyAlphaMin; g.gp3[1] = tok::kGlassTintMix;
    g.gp3[2] = 24.0f; g.gp3[3] = tok::kGlassVibrancySat;
    glBindBuffer(GL_UNIFORM_BUFFER, gUbo_);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(g), &g);

    for (const Quad& q : batch) {
      QuadU u{};
      u.box[0] = q.box.x; u.box[1] = q.box.y; u.box[2] = q.box.w * 0.5f; u.box[3] = q.box.h * 0.5f;
      u.color[0] = q.color.r / 255.0f; u.color[1] = q.color.g / 255.0f;
      u.color[2] = q.color.b / 255.0f; u.color[3] = q.alpha;
      u.uv[0] = q.uv.x; u.uv[1] = q.uv.y; u.uv[2] = q.uv.w; u.uv[3] = q.uv.h;
      u.p0[0] = q.radius; u.p0[1] = q.exponent; u.p0[2] = q.notchDepth; u.p0[3] = tok::kNotchWidth;
      u.p1[0] = q.blur; u.p1[1] = q.thickness;
      glBindBuffer(GL_UNIFORM_BUFFER, qUbo_);
      glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(u), &u);
      glUniform1i(glGetUniformLocation(prog_, "uMat"), int(q.mat));
      glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
  }

  Display* dpy_ = nullptr;
  Window win_ = 0;
  GLXContext ctx_ = nullptr;
  GLuint prog_ = 0, blurProg_ = 0, vao_ = 0, atlas_ = 0, backdrop_ = 0, fb_ = 0, gUbo_ = 0, qUbo_ = 0;
  int w_ = 0, h_ = 0;
  float time_ = 0, hover_ = 0;
  float tint_[3]{0.42f, 0.31f, 0.64f}, surf_[3]{0.02f, 0.02f, 0.03f};
  bool ok_ = false;
  std::vector<Quad> tonal_, glass_;
};

} // namespace

Renderer* createGLRenderer() { return new GLRenderer(); }

} // namespace vidra::gfx
