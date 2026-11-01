layout(location = 0) out vec2 vUV;
layout(location = 1) out vec2 vPos;
layout(location = 2) flat out vec2 vHalf;
layout(location = 3) flat out vec4 vColor;
layout(location = 4) flat out vec4 vUvRect;

layout(std140) uniform QuadU { vec4 box; vec4 color; vec4 uvRect; vec4 p0; vec4 p1; } Q;
layout(std140) uniform Globals {
  vec2 screen; float time; float hover;
  vec4 tint; vec4 surface; vec4 light;
  vec4 neighbors[8]; vec4 gp; vec4 gp2; vec4 gp3;
} G;

void main() {
  vec2 corner = vec2(float((gl_VertexIndex << 1) & 2), float(gl_VertexIndex & 2));
  vHalf = Q.box.zw;
  vec2 world = Q.box.xy + corner * (vHalf + vec2(24.0));
  vUV = world / G.screen;
  vPos = corner * (vHalf + vec2(24.0));
  vColor = Q.color;
  vUvRect = Q.uvRect;
  vec2 ndc = world / G.screen * 2.0 - 1.0;
  gl_Position = vec4(ndc.x, -ndc.y, 0.0, 1.0);
}
