"""
Common types and defines

History:
--------
- Created by Holderried, Roman, 22.08.2022
"""

class Version:
    def __init__(self, versionMajor = 0, versionMinor = 0, revision = 0):
        self.versionMajor = versionMajor
        self.versionMinor = versionMinor
        self.revision = revision