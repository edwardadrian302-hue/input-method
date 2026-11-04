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
schema = '''# ChaoxiPinyin: offline full pinyin using pinned rime-ice dictionaries; no Rime user dictionary.
schema:
  schema_id: chaoxipinyin
  name: 潮汐拼音 · 全拼
  version: "0.3.0"
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
  dictionary: chaoxi
  prism: chaoxipinyin
  enable_user_dict: false
  enable_sentence: true
  enable_completion: true
  enable_word_completion: false
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
(DATA / "chaoxipinyin.schema.yaml").write_text(schema, encoding="utf-8")
(DATA / "default.yaml").write_text('''config_version: "0.3.0"
schema_list:
  - schema: chaoxipinyin
menu:
  page_size: 9
switcher:
  hotkeys: []
''', encoding="utf-8")
# Verify every pinned input before copying the offline Chinese dictionaries.
import hashlib
pin = json.loads((ROOT / "dictionary.lock.json").read_text(encoding="utf-8-sig"))
for item in pin["files"]:
    source = ROOT / "vendor/rime-ice" / item["path"]
    if hashlib.sha256(source.read_bytes()).hexdigest() != item["sha256"]:
        raise RuntimeError("Dictionary checksum mismatch: " + item["path"])
(DATA / "cn_dicts").mkdir(exist_ok=True)
for name in ("8105", "base", "ext"):
    shutil.copy2(ROOT / "vendor/rime-ice/cn_dicts" / (name + ".dict.yaml"), DATA / "cn_dicts")
(DATA / "chaoxi.dict.yaml").write_text("""# Chinese tables from iDvel/rime-ice, pinned in dictionary.lock.json.
# Wrapper modified for 潮汐拼音: no English, Lua plugins, or network services.
---
name: chaoxi
version: '0.3.0'
sort: by_weight
import_tables:
  - cn_dicts/8105
  - cn_dicts/base
  - cn_dicts/ext
...
""", encoding="utf-8")
licenses = ROOT / "licenses"
licenses.mkdir(exist_ok=True)
shutil.copy2(ROOT / "vendor/rime-ice/LICENSE", licenses / "rime-ice-GPL3.txt")
shutil.copy2(ROOT / "vendor/librime/LICENSE", licenses / "librime-BSD.txt")
path = ROOT / "vendor/librime"
lock = {"librime": {
    "url": subprocess.check_output(["git", "-C", str(path), "remote", "get-url", "origin"], text=True).strip(),
    "commit": subprocess.check_output(["git", "-C", str(path), "rev-parse", "HEAD"], text=True).strip(),
}}
lock["rime-ice"] = {"url": pin["url"], "commit": pin["commit"]}
(ROOT / "sources.lock.json").write_text(json.dumps(lock, indent=2) + "\n", encoding="utf-8")
print("Prepared full-pinyin schema, offline dictionary, licenses, and source lock.")
