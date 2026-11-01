layout(location = 0) in vec2 vUV;
layout(location = 1) in vec2 vPos;
layout(location = 2) flat in vec2 vHalf;
layout(location = 3) flat in vec4 vColor;
layout(location = 4) flat in vec4 vUvRect;
layout(location = 0) out vec4 oColor;

layout(std140) uniform QuadU { vec4 box; vec4 color; vec4 uvRect; vec4 p0; vec4 p1; } Q;
layout(std140) uniform Globals {
  vec2 screen; float time; float hover;
  vec4 tint; vec4 surface; vec4 light;
  vec4 neighbors[8]; vec4 gp; vec4 gp2; vec4 gp3;
} G;

uniform sampler2D uBackdrop;
uniform sampler2D uAtlas;
uniform int uMat;

float sdfAll(vec2 p) {
  float d = sdCard(p, vHalf, Q.p0.x, Q.p0.y, Q.p0.z, Q.p0.w);
  if (uMat == 1) {
    for (int i = 0; i < 8; ++i) {
      if (G.neighbors[i].w < 0.5) continue;
      vec2 local = p - (G.neighbors[i].xy - Q.box.xy);
      d = smin(d, sdSquircle(local, vec2(G.neighbors[i].z), Q.p0.y), G.gp.w);
    }
  }
  return d;
}

void main() {
  if (uMat == 2) {
    float d = sdRoundBox(vPos, vHalf, Q.p0.x);
    float k = max(Q.p1.x * 0.5, 1.0);
    float a = exp(-max(d, 0.0) * max(d, 0.0) / (2.0 * k * k)) * vColor.a;
    if (a < 0.002) discard;
    oColor = vec4(vColor.rgb * a, a);
    return;
  }
  if (uMat == 3) {
    vec2 local = vUvRect.xy + vUvRect.zw * vec2(vUV.x, 1.0 - vUV.y);
    vec4 t = texture(uAtlas, local);
    if (t.a < 0.02) discard;
    oColor = vec4(vColor.rgb, vColor.a) * t;
    return;
  }
  float d = sdfAll(vPos);
  float aa = max(fwidth(d), 1e-4);
  float cov = 1.0 - smoothstep(-aa, aa, d);
  if (cov <= 0.001) discard;
  if (uMat == 4 || uMat == 0) {
    oColor = vec4(vColor.rgb, vColor.a * cov);
    return;
  }
  float bevel = G.gp.x;
  float t = clamp(-d / bevel, 0.0, 1.0);
  float strength = pow(1.0 - t, 2.0) * (Q.p1.y / 24.0);
  float dx = sdfAll(vPos + vec2(1.0, 0.0)) - sdfAll(vPos - vec2(1.0, 0.0));
  float dy = sdfAll(vPos + vec2(0.0, 1.0)) - sdfAll(vPos - vec2(0.0, 1.0));
  vec2 nrm = normalize(vec2(dx, dy) + vec2(1e-6));
  vec2 off = nrm * strength * G.gp.y / G.screen;
  float disp = G.gp.z * strength / G.screen.x;
  vec3 refr = vec3(texture(uBackdrop, vUV + off + nrm * disp).r,
                   texture(uBackdrop, vUV + off).g,
                   texture(uBackdrop, vUV + off - nrm * disp).b);
  vec3 hsv = rgb2hsv(refr);
  hsv.y = clamp(hsv.y * (1.0 + G.gp3.w), 0.0, 1.0);
  refr = clamp(hsv2rgb(hsv), 0.0, 1.0);
  float bodyA = clamp(max(G.gp3.x, 0.22) + (1.0 - t) * 0.10, 0.0, 0.92);
  vec3 body = mix(refr, G.tint.rgb, G.gp3.y);
  float bl = dot(refr, vec3(0.2126, 0.7152, 0.0722));
  body = mix(body, G.surface.rgb, smoothstep(G.gp2.z, G.gp2.z + 0.2, bl) * G.gp2.w);
  float rim = smoothstep(1.0 - G.gp2.x / max(bevel, 1.0), 1.0, 1.0 - t);
  float ang = dot(nrm, normalize(G.light.xy));
  float rimA = rim * G.gp2.y * smoothstep(-0.2, 0.9, ang);
  float phase = fract(G.time / 1.2);
  float axis = dot(vUV, vec2(cos(radians(35.0)), sin(radians(35.0))));
  float sweep = smoothstep(0.0, 0.35, phase * 1.7 - axis) *
                (1.0 - smoothstep(0.35, 0.7, phase * 1.7 - axis));
  vec3 rgb = body + vec3(rimA) + vec3(sweep * G.hover * 0.22);
  oColor = vec4(rgb * cov, clamp(bodyA + rimA * 0.5, 0.0, 1.0) * cov);
}
