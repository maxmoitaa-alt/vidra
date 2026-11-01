#!/usr/bin/env python3
import json, pathlib, sys

def fl(x): return f"{float(x):.4f}f"
def arr(xs): return "{" + ",".join(fl(x) for x in xs) + "}"
def const(n, v):
    if isinstance(v, str):  return f'inline constexpr std::string_view {n} = "{v}";'
    if isinstance(v, bool): return f"inline constexpr bool {n} = {'true' if v else 'false'};"
    if isinstance(v, int):  return f"inline constexpr int {n} = {v};"
    return f"inline constexpr float {n} = {fl(v)};"
def ident(k): return k[0].upper() + k[1:]

def main():
    t = json.loads(pathlib.Path("tokens/tokens.json").read_text())
    out = pathlib.Path("include/vidra/tokens.hpp")
    out.parent.mkdir(parents=True, exist_ok=True)
    L = ["// GERADO — fonte: tokens/tokens.json", "#pragma once",
         "#include <array>", "#include <cstdint>", "#include <string_view>",
         "namespace vidra::tok {", const("kVersion", t["version"])]
    c = t["color"]
    for k in ("primary", "secondary", "tertiary", "neutral", "neutralVariant"):
        L.append(const("kChroma" + ident(k), c["chroma"][k]))
    L += [const("kHueOffsetTertiary", c["hueOffsetTertiary"]),
          const("kSurfaceTintMixLow", c["surfaceTintMix"][0]),
          const("kSurfaceTintMixHigh", c["surfaceTintMix"][1]),
          const("kContrastMin", c["contrastMin"]),
          const("kCrossfadeMs", c["crossfadeMs"]),
          "inline constexpr std::array<std::string_view,%d> kSeeds = {%s};"
          % (len(c["seeds"]), ",".join(f'"{s}"' for s in c["seeds"]))]
    s = t["shape"]
    L += ["inline constexpr std::array<float,10> kShapeScale = %s;" % arr(s["scale"]),
          const("kShapeFull", 1e4), const("kSquircleExponent", s["exponent"]),
          const("kNotchDepth", s["notchDepth"]), const("kNotchWidth", s["notchWidth"])]
    m = t["motion"]
    for n in ("springIn", "springOut"):
        for f in ("stiffness", "damping", "mass"):
            L.append(const(f"k{n[0].upper()+n[1:]}{f.capitalize()}", m[n][f]))
    for k, v in m["curves"].items():
        L.append("inline constexpr std::array<float,4> kCurve%s = %s;" % (ident(k), arr(v)))
    for k, v in m["durations"].items(): L.append(const("kDur" + ident(k), v))
    L.append(const("kStagger", m["stagger"]))
    for name, v in t["type"].items():
        i = "".join(p.capitalize() for p in name.split("-"))
        L += [const(f"kType{i}Size", v["size"]), const(f"kType{i}Line", v["line"]),
              const(f"kType{i}Tracking", v["tracking"]), const(f"kType{i}Weight", v["weight"])]
    for k, v in t["state"].items():     L.append(const("kState" + ident(k), v))
    for k, v in t["elevation"].items(): L.append(const("kElev" + ident(k), v))
    for k, v in t["glass"].items():     L.append(const("kGlass" + ident(k), v))
    for k, v in t["layout"].items():    L.append(const("kLayout" + ident(k), v))
    L += ["} // namespace vidra::tok", ""]
    out.write_text("\n".join(L))
    print(f"tokens.hpp: {len(L)} linhas")

if __name__ == "__main__":
    sys.exit(main())
