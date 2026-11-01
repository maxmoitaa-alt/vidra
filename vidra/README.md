# Vidra 1.0.1

Executor Luau para Roblox em Linux. Material 3 Expressive e chrome em Liquid Glass.

```sh
./build.sh              # compila, testa e instala em /usr
./build.sh --no-install
ctest --test-dir build --output-on-failure
```

Depende de clang, cmake, ninja, python3, libgl-dev, libx11-dev, libxext-dev,
libxfixes-dev, libxi-dev, libfreetype-dev, libharfbuzz-dev. Vulkan e opcional:
sem ele o app usa OpenGL 3.3. Para forcar: `VIDRA_RENDERER=gl`.

Atalhos: T alterna tema, 1..5 abrem configuracoes, console, explorador,
aparencia e sobre.
