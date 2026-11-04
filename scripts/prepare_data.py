"""Prepare the minimal, fully offline distribution from pinned source checkouts."""
from pathlib import Path
import shutil
import subprocess
import json

ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data"
DATA.mkdir(exist_ok=True)
# Full pinyin; a small explicit set of initial abbreviations, no fuzzy spelling.
algebra = """    - abbrev/^([a-z]).+$/$1/
    - abbrev/^([zcs]h).+$/$1/
    - derive/^([nl])ue$/$1ve/
    - derive/^([jqxy])u/$1v/"""
schema = '''# JianPin: offline full pinyin using pinyin_simp; no Rime user dictionary.
schema:
  schema_id: jianpin
  name: 简拼 · 全拼
  version: "0.2.0"
switches:
  - name: ascii_mode
    reset: 0
engine:
  processors: [key_binder, speller, punctuator, selector, navigator, express_editor]
  segmentors: [abc_segmentor, punct_segmentor, fallback_segmentor]
  translators: [punct_translator, script_translator]
  filters: [uniquifier]
menu:
  page_size: 9
speller:
  alphabet: "zyxwvutsrqponmlkjihgfedcba"
  initials: zyxwvutsrqponmlkjihgfedcba
  delimiter: " '"
  algebra:
''' + algebra + '''
translator:
  dictionary: pinyin_simp
  prism: jianpin
  enable_user_dict: false
  enable_sentence: true
  enable_completion: true
  contextual_suggestions: false
key_binder:
  bindings:
    - { when: has_menu, accept: minus, send: Page_Up }
    - { when: has_menu, accept: equal, send: Page_Down }
punctuator:
  half_shape:
    ",": { commit: "，" }
    ".": { commit: "。" }
    "!": { commit: "！" }
    "?": { commit: "？" }
    ":": { commit: "：" }
    ";": { commit: "；" }
    "(": "（"
    ")": "）"
    "[": "【"
    "]": "】"
    '"': { pair: ["“", "”"] }
    "'": { pair: ["‘", "’"] }
'''
(DATA / "jianpin.schema.yaml").write_text(schema, encoding="utf-8")
(DATA / "default.yaml").write_text('''config_version: "0.2.0"
schema_list:
  - schema: jianpin
menu:
  page_size: 9
switcher:
  hotkeys: []
''', encoding="utf-8")
shutil.copy2(ROOT / "vendor/pinyin-simp/pinyin_simp.dict.yaml", DATA)
licenses = ROOT / "licenses"
licenses.mkdir(exist_ok=True)
for repo, name in [("librime", "librime-BSD.txt"), ("double-pinyin", "double-pinyin-GPL3.txt"), ("pinyin-simp", "pinyin-simp-Apache2.txt")]:
    shutil.copy2(ROOT / "vendor" / repo / "LICENSE", licenses / name)
lock = {}
for repo in ["librime", "double-pinyin", "pinyin-simp"]:
    path = ROOT / "vendor" / repo
    lock[repo] = {
        "url": subprocess.check_output(["git", "-C", str(path), "remote", "get-url", "origin"], text=True).strip(),
        "commit": subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"], text=True).strip(),
    }
(ROOT / "sources.lock.json").write_text(json.dumps(lock, indent=2) + "\n", encoding="utf-8")
print("Prepared full-pinyin schema, offline dictionary, licenses, and source lock.")
