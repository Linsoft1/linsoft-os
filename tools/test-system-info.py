#!/usr/bin/python3
"""Focused release-check tests; run with the Debian system Python and PyGObject."""

import io
import json
import pathlib
import runpy
import unittest
import urllib.error
from unittest.mock import patch

SOURCE = pathlib.Path(__file__).resolve().parent.parent / (
    "linsoft/debian-live/config/includes.chroot/usr/bin/linsoft-system-info"
)
APP = runpy.run_path(str(SOURCE), run_name="linsoft_system_info")


class ReleaseTests(unittest.TestCase):
    def test_numeric_comparison(self):
        compare = APP["compare_versions"]
        for latest, installed, expected in (
            ("v1.5", "1.5", 0), ("v1.5.0", "1.5", 0),
            ("v1.10", "1.9", 1), ("v2.0", "1.5", 1),
            ("v1.4", "1.5", -1),
        ):
            with self.subTest(latest=latest, installed=installed):
                self.assertEqual(compare(latest, installed), expected)

    def test_invalid_versions(self):
        for value in (None, 15, "", "latest", "v1.5-rc1", "../1.5", "1.5/evil"):
            with self.subTest(value=value), self.assertRaises(ValueError):
                APP["version_parts"](value)

    def test_latest_release(self):
        with patch("urllib.request.urlopen", return_value=io.BytesIO(
            json.dumps({"tag_name": "v1.5", "draft": False, "prerelease": False}).encode()
        )) as request:
            self.assertEqual(APP["fetch_latest"](), "v1.5")
            self.assertEqual(request.call_args.kwargs["timeout"], 20)
            self.assertEqual(request.call_args.args[0].full_url, APP["LATEST_API"])

    def test_invalid_responses(self):
        for content in (
            b"not-json", b"[]", b"{}", b'{"tag_name": "../evil"}',
            b'{"tag_name": "v2.0", "draft": true}',
            b'{"tag_name": "v2.0", "prerelease": true}',
            b"x" * 1048577,
        ):
            with self.subTest(content=content[:60]):
                with patch("urllib.request.urlopen", return_value=io.BytesIO(content)):
                    with self.assertRaises(ValueError):
                        APP["fetch_latest"]()

    def test_network_failure(self):
        with patch("urllib.request.urlopen", side_effect=urllib.error.URLError("offline")):
            with self.assertRaises(urllib.error.URLError):
                APP["fetch_latest"]()


if __name__ == "__main__":
    unittest.main()
