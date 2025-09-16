# OTTSR – Study Reminder App

[![Build Status](https://img.shields.io/github/actions/workflow/status/USERNAME/REPO/ottsr.yml?branch=main)](https://github.com/USERNAME/REPO/actions)
[![License: MIT](https://img.shields.io/badge/License-MIT-green)](LICENSE)
[![C](https://img.shields.io/badge/Language-C-blue)](https://en.wikipedia.org/wiki/C_%28programming_language%29)

OTTSR (**O**ptimized **T**ime-**T**racking for **S**tudy **R**outines) is a lightweight Windows desktop app for managing study sessions. Early development version – currently just a white box due to errors.

---

## Features

* Pomodoro-style study/break cycles
* Config file support (`ottsr.conf`)
* Native Win32 app – small EXE, no dependencies

> ⚠️ Early dev: UI incomplete, may crash.

---

## Build from Source

Requires **MSVC** on Windows.

```sh
cl /O2 /GL /Gy /arch:AVX2 /fp:fast /MD ottsr.c /Fe:ottsr.exe user32.lib kernel32.lib shell32.lib winmm.lib opengl32.lib glu32.lib gdi32.lib comctl32.lib psapi.lib /link /LTCG
```

GitHub Actions builds and uploads a new release automatically.

---

## License

MIT License
