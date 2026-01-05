# MobaBase

**MobaBase** is an aspiring **3D game engine** written in **C++**, with a **Vulkan-based rendering pipeline**.
It started as a purpose-built engine for **Multiplayer Online Battle Arenas**, but has since evolved toward supporting more general game environments.

This repository primarily exists as a **personal project and technical playground**.

> ⚠️ **Status:** Work in progress. The engine is far from feature-complete and under active development.

---

## Goals & Design Direction

* **Cross-platform focus**

  Targeting **Windows / Linux**, using **Vulkan** with **GLFW** (Windows-only at the moment due to some direct Win32 calls).

* **Pure C++ gameplay code**

  No embedded scripting languages or heavy runtime abstractions. Systems are written with explicit ownership and lifetime control.

* **Multithreading from the ground up**

  Core systems are designed with concurrency in mind.

* **Easy access patterns**

  Philosophy to keep easier object access patterns using value type references over DOD archtecture for the best of two worlds.

* **Flexible rendering architecture**

  Supporting multiple lighting models and shader-driven material reflection.

---

## Current State

Implemented or partially implemented systems include:

* **Rendering**

  * Vulkan renderer
  * Shader-based material creation, matching, and reflection
  * Forward+ lighting pipeline

* **Scene & Transforms**

  * Efficient transform hierarchy
  * Asynchronous BVH construction

* **Platform**

  * Windows support (Linux planned once Win32 dependencies are rerouted through GLFW)

---

## Non-Goals (for now)

* Editor tooling
* High-level gameplay abstractions
* Beginner-oriented APIs
* Engine-as-a-product stability

This project prioritizes **engine internals and architectural clarity** over usability or completeness... for now =)

---

## Why this repository exists

MobaBase is primarily:

* A long-running **personal engine project**
* A place to explore **rendering, ECS-style data access, and multithreaded systems**

---

## Notes

* Expect unfinished systems and refactors.
* Expect some platform-specific code paths.
* Expect evolution — APIs are not frozen.

---

*More documentation will be added as we go.*
