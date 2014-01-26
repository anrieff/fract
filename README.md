fract
=====

This is a real-time raytracer, written in C++ (or, possibly 50:50 inline assembly), heavily optimized to run fast even
on old hardware like the Core 2 Duo processors. It boasts:

- Fully-fedged raytracing, reflections, refractions and the like.
- Texture mip-mapping and mipmap level selection based on an approach, similar to ray differentials
- Heavily-optimized multi-layer rendering code, with MMX, MMX2 and SSE optimizations.
- Adaptive raytracing, and fast soft-shadows calculations
- Some physics (ball collisions and bounces)
- Benchmark-grade options, creation of result files. The benchmark used to be a good general-purpose PC benchmark
- A developer mode, where one can look-around the scene. Also a quake-like console interface for scripting.
- Fully portable to Windows, Linux, OS X and others (FreeBSD and Solaris; have been ported to ARM and Sparc).

For more info on how to compile it, consult INSTALL.
