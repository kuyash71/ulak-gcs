# ULAK GCS — Polisher Specification

**Document purpose:** Define the design, behavior, and script contract for the **Polisher** developer tool
built into the ULAK GCS camera panel.

---

## 1. Overview

Polisher is a developer tool integrated into the GCS camera panel. Its purpose is to allow runtime tuning
of vision pipeline parameters without restarting the application or modifying source files directly.

The operator loads a compatible Python vision script into Polisher. Any parameters declared within the
script's `@polisher_start` / `@polisher_end` block are rendered as interactive controls in the Polisher
panel. Adjusting a control updates the parameter value live on the active camera feed.

Polisher is intended for **simulation and development use** — it allows rapid iteration on OpenCV
processing parameters during vision pipeline testing.

---

## 2. Availability

Polisher is only accessible when **camera mode is active**. If the camera mode is off, the Polisher
switch in the camera panel is hidden.

Enabling the Polisher switch opens the Polisher panel alongside the camera feed.

---

## 3. Script management

### 3.1 Script storage

All Polisher scripts are stored under:

```
config/polisher/scripts/
```

This directory is the single source of truth for available scripts. Scripts outside this directory
cannot be loaded by Polisher.

### 3.2 Loading a new script

Scripts are loaded via the **script selector combobox** in the Polisher panel. The combobox lists all
`.py` files currently present in `config/polisher/scripts/`.

A dedicated **"Add script"** button opens a file picker. The selected file is copied into
`config/polisher/scripts/` under its original filename.

#### 3.2.1 Filename conflict resolution

If a file with the same name already exists in `config/polisher/scripts/`, the user is presented with
a dialog containing two options:

- **Cancel** — the copy operation is aborted. No file is modified.
- **Rename** — the user is prompted to enter a new name for the incoming file. The renamed file is then
  copied. The existing file is not modified.

There is no silent overwrite behavior.

#### 3.2.2 Filename rules

The filename must be a valid Python module name and must end with `.py`. Characters that are invalid
for Python identifiers or unsupported by the parser will be rejected with an inline error message.
The file retains its original name unless the user explicitly renames it during a conflict resolution.

### 3.3 Switching between scripts

The script selector combobox allows switching between any script present in `config/polisher/scripts/`
at any time while Polisher is open. Switching reloads the parameter block from the newly selected
script and re-renders the control panel.

Parameter values are **not persisted** between sessions or script switches. Each time a script is
loaded or switched to, all controls initialize from the default values declared in `@polisher_param`.

---

## 4. Script contract

A Polisher-compatible script must declare all tunable parameters inside a
`@polisher_start` / `@polisher_end` block. The parser reads only this block; the rest of the
file is not inspected at load time.

### 4.1 Block structure

```python
@polisher_start

@polisher_param(label="Threshold", type="slider", min=0, max=255)
threshold = 127

@polisher_param(label="Blur Kernel", type="slider", min=1, max=21, step=2)
blur_kernel = 5

@polisher_param(label="Edge Detection", type="toggle")
use_edge = True

@polisher_end
```

### 4.2 `@polisher_param` attributes

| Attribute | Required | Description |
|-----------|----------|-------------|
| `label` | Yes | Display name shown in the Polisher panel |
| `type` | Yes | Control type — see Section 4.3 |
| `min` | type=slider | Minimum value |
| `max` | type=slider | Maximum value |
| `step` | No | Step size for slider controls (default: 1) |

The variable assigned immediately below `@polisher_param` provides the **default value** and the
**variable name** used at runtime.

### 4.3 Supported control types (MVP)

| Type | Description |
|------|-------------|
| `slider` | Numeric range control. Renders as a slider with min/max/step. Value type is inferred from the default (int or float). |
| `toggle` | Boolean on/off switch. Default must be `True` or `False`. |

The type system is designed to be extensible. Additional types may be added in future iterations.

### 4.4 Scripts with no `@polisher_param` declarations

If a script is loaded and the parser finds no `@polisher_param` declarations within the
`@polisher_start` / `@polisher_end` block — or if the block itself is absent — the Polisher panel
renders an empty control area with the following notice:

```
⚠ Polisher not enabled on this script.
```

The script can still be selected and the camera feed remains active, but no parameters can be tuned.

---

## 5. Runtime behavior

- The Polisher panel renders controls derived from the parsed parameter block of the active script.
- Changing a control value updates the corresponding variable in the script's execution context
  immediately (live preview on the active feed).
- Parameter values are **not persisted**. On script load, switch, or application restart, all values
  revert to their declared defaults.
- The camera feed must remain responsive during parameter changes. Control updates must not block
  the UI thread.

---

## 6. Parser behavior

- The parser scans the file for `@polisher_start` and `@polisher_end` markers.
- Only the content between these markers is parsed.
- Each `@polisher_param(...)` decorator is read for metadata attributes.
- The variable assignment on the line immediately following the decorator provides the default value
  and variable name.
- Lines between decorators that are not variable assignments are ignored.
- Parsing is performed at script load time and on script switch. It is not repeated during runtime.

---

## 7. File structure reference

```
config/
└── polisher/
    └── scripts/
        ├── edge_detection.py
        ├── color_threshold.py
        └── ...
```

---

**End of document.**