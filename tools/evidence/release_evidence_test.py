import copy
import pathlib
import sys
from tempfile import TemporaryDirectory
import unittest
from unittest.mock import patch

sys.path.insert(0, str(pathlib.Path(__file__).resolve().parent))

from tools.evidence.evidence_contract import aggregate_records, expand_workloads, load_contract, parse_scale_record, validate_contract
from tools.evidence.release_evidence import is_inside, probe_gpu


class ReleaseEvidenceTests(unittest.TestCase):
    def test_repository_contract_is_valid(self) -> None:
        root = pathlib.Path(__file__).resolve().parents[2]
        contract = load_contract(root)

        self.assertEqual(validate_contract(contract), [])

    def test_workload_expansion_covers_matrix(self) -> None:
        root = pathlib.Path(__file__).resolve().parents[2]
        expanded = expand_workloads(load_contract(root))

        self.assertEqual(len(expanded), 33)
        self.assertEqual(
            sum(item["kind"] == "migration" for item in expanded),
            2,
        )
        self.assertEqual(
            sum(item["kind"] == "overlap" and item["overlap"] == "serialized" for item in expanded),
            2,
        )

    def test_scale_record_parser_converts_measurements(self) -> None:
        line = (
            "BLITZAR SCALE schema=1 rank=1 ranks=2 particles=64 warmup_steps=1 "
            "timed_steps=3 seed=424242 solver=direct overlap=overlapped status=0 backend=0 "
            "elapsed_ns=42 peak_rss_bytes=99 local_before=32 local_after=31 "
            "migration_observed=1 migration_sent_remote=4 migration_received_remote=3 "
            "overlap_has_overlap=1 local_packets=31 ghost_packets=33 send_bytes=496 "
            "receive_bytes=528 oracle_checked=1 oracle_pass=1 oracle_max_error=1.5e-8"
        )

        record = parse_scale_record(line)

        self.assertIsNotNone(record)
        self.assertEqual(record["rank"], 1)
        self.assertEqual(record["elapsed_ns"], 42)
        self.assertTrue(record["migration_observed"])
        self.assertEqual(record["oracle_max_error"], 1.5e-8)

    def test_aggregate_sums_rank_measurements(self) -> None:
        workload = {
            "id": "migration",
            "kind": "migration",
            "particles": 64,
            "ranks": 2,
            "solver": "direct",
            "overlap": "overlapped",
            "migration": True,
        }
        records = [
            {
                "rank": 0,
                "status": 0,
                "backend": 0,
                "elapsed_ns": 20,
                "peak_rss_bytes": 100,
                "local_before": 32,
                "local_after": 31,
                "local_packets": 31,
                "ghost_packets": 33,
                "send_bytes": 496,
                "receive_bytes": 528,
                "migration_observed": True,
                "migration_sent_remote": 4,
                "migration_received_remote": 3,
                "overlap_has_overlap": True,
                "oracle_checked": False,
            },
            {
                "rank": 1,
                "status": 0,
                "backend": 0,
                "elapsed_ns": 24,
                "peak_rss_bytes": 120,
                "local_before": 32,
                "local_after": 33,
                "local_packets": 33,
                "ghost_packets": 31,
                "send_bytes": 528,
                "receive_bytes": 496,
                "migration_observed": True,
                "migration_sent_remote": 3,
                "migration_received_remote": 4,
                "overlap_has_overlap": True,
                "oracle_checked": False,
            },
        ]

        aggregate = aggregate_records(records, workload)

        self.assertTrue(aggregate["ranks_complete"])
        self.assertEqual(aggregate["elapsed_ns"], 24)
        self.assertEqual(aggregate["send_bytes"], 1024)
        self.assertEqual(aggregate["local_after"], 64)
        self.assertTrue(aggregate["migration_observed"])

    def test_contract_rejects_malformed_migration_shape(self) -> None:
        root = pathlib.Path(__file__).resolve().parents[2]
        contract = load_contract(root)
        invalid = copy.deepcopy(contract)
        invalid["workloads"][2]["ranks"] = None

        errors = validate_contract(invalid)

        self.assertTrue(any("ranks" in error for error in errors))

    def test_evidence_path_must_be_external(self) -> None:
        root = pathlib.Path("C:/repo").resolve()

        self.assertTrue(is_inside(root / "plan" / "run", root))
        self.assertFalse(is_inside(root.parent / "evidence", root))

    def test_gpu_probe_uses_accelerator_qualification_target(self) -> None:
        with TemporaryDirectory() as directory:
            root = pathlib.Path(directory)
            build = root / "build"
            output = root / "evidence"
            build.mkdir()
            (build / "CMakeCache.txt").write_text(
                "BLITZAR_HIP_ENABLED:INTERNAL=ON\n", encoding="utf-8"
            )
            (build / "blitzar_accelerator_test").touch()
            process = {
                "returncode": 0,
                "stdout": "BLITZAR GPU qualification passed: compiled=1 device=1\n",
                "stderr": "",
                "cwd": str(root),
                "timed_out": False,
                "missing": False,
            }

            with patch("tools.evidence.release_evidence.shutil.which", return_value=None), patch(
                "tools.evidence.release_evidence.run_process", return_value=process
            ):
                result = probe_gpu(root, build, output)

        self.assertEqual(result["compile"]["state"], "passed")
        self.assertEqual(result["device_execution"]["state"], "passed")
        self.assertEqual(result["fallback"]["state"], "not-observed")


if __name__ == "__main__":
    unittest.main()
