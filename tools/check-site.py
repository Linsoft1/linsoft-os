#!/usr/bin/env python3
"""Validates the Linsoft OS web page: balanced tags, unique ids, local links."""
import html.parser
import pathlib
import sys

VOID = {"area", "base", "br", "col", "embed", "hr", "img", "input", "link",
        "meta", "source", "track", "wbr"}


class Checker(html.parser.HTMLParser):
    def __init__(self):
        super().__init__()
        self.stack, self.ids, self.anchors, self.files, self.errors = [], [], [], [], []

    def handle_starttag(self, tag, attrs):
        attrs = dict(attrs)
        if "id" in attrs:
            self.ids.append(attrs["id"])
        for key in ("href", "src"):
            value = attrs.get(key) or ""
            if value.startswith("#") and len(value) > 1:
                self.anchors.append(value[1:])
            elif value and not value.startswith(("http:", "https:", "mailto:", "#", "data:")):
                self.files.append(value.split("?")[0])
        if tag == "img" and "alt" not in attrs:
            self.errors.append(f"img without alt: {attrs.get('src')}")
        if tag not in VOID:
            self.stack.append((tag, self.getpos()[0]))

    def handle_endtag(self, tag):
        if tag in VOID:
            return
        if not self.stack or self.stack[-1][0] != tag:
            self.errors.append(f"line {self.getpos()[0]}: unexpected </{tag}>")
            return
        self.stack.pop()


page = pathlib.Path(sys.argv[1] if len(sys.argv) > 1 else "docs/index.html")
checker = Checker()
checker.feed(page.read_text(encoding="utf-8"))
errors = checker.errors[:]
errors += [f"unclosed <{tag}> from line {line}" for tag, line in checker.stack]
errors += [f"duplicate id: {i}" for i in sorted({i for i in checker.ids if checker.ids.count(i) > 1})]
errors += [f"missing anchor: #{a}" for a in sorted(set(checker.anchors) - set(checker.ids))]
errors += [f"missing file: {f}" for f in sorted(set(checker.files)) if not (page.parent / f).exists()]
print("\n".join(errors) or f"{page}: OK ({len(checker.ids)} ids, {len(checker.anchors)} anchors)")
sys.exit(1 if errors else 0)