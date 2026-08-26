import tempfile
import unittest
from pathlib import Path

from tools.gates.public_api_gate import scan_internal_headers, scan_public_header


class PublicApiGateTests(unittest.TestCase):
    def setUp(self) -> None:
        self.temp_dir = tempfile.TemporaryDirectory()
        self.root = Path(self.temp_dir.name)
        (self.root / "include" / "blitzar").mkdir(parents=True)
        (self.root / "src" / "sdk").mkdir(parents=True)

    def tearDown(self) -> None:
        self.temp_dir.cleanup()

    def test_accepts_registered_c_abi_pointers_and_fixed_types(self) -> None:
        path = self.root / "include" / "blitzar" / "blitzar.h"
        path.write_text(
            "#include <stdint.h>\n"
            "typedef struct blitzar_context blitzar_context;\n"
            "typedef struct blitzar_simulation blitzar_simulation;\n"
            "blitzar_status Run(blitzar_simulation** simulation, const double* values);\n",
            encoding="utf-8",
        )
        specification = {"allowed_includes": ["stdint.h"], "raw_pointer_policy": "c_abi"}
        self.assertEqual(scan_public_header(self.root, path, specification, ["MPI_", "hip", "cuda", "src/"]), [])

    def test_rejects_forbidden_cpp_include_and_raw_pointer(self) -> None:
        path = self.root / "include" / "blitzar" / "blitzar.hpp"
        path.write_text(
            '#include "src/Internal.hpp"\n#include <mpi.h>\nclass Wrapper { int* value; };\n',
            encoding="utf-8",
        )
        specification = {"allowed_includes": [], "raw_pointer_policy": "forbidden"}
        violations = scan_public_header(self.root, path, specification, ["MPI_", "hip", "cuda", "src/"])
        messages = {item.message for item in violations}
        self.assertIn("raw pointer is forbidden in the C++ public facade", messages)
        self.assertTrue(any("include is not registered" in message for message in messages))

    def test_rejects_unregistered_internal_cpp_facade_include(self) -> None:
        path = self.root / "src" / "sdk" / "State.hpp"
        path.write_text('#include <blitzar/blitzar.hpp>\n', encoding="utf-8")
        violations = scan_internal_headers(self.root, {
            "internal_cpp_facade_exceptions": [],
        })
        self.assertEqual(len(violations), 1)
        self.assertIn("public C++ facade", violations[0].message)


if __name__ == "__main__":
    unittest.main()
