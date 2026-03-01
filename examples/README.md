# Example Outputs

These files show real output from Synrix tools. Run them yourself to verify.

| File | Command | What It Shows |
|------|---------|---------------|
| [crash_recovery_output.txt](crash_recovery_output.txt) | `./tools/crash_recovery_demo.sh` | Crash + recovery, ZERO DATA LOSS |
| [latency_diagnostic_output.txt](latency_diagnostic_output.txt) | `./tools/run_query_latency_diagnostic.sh` | Min/max/avg latency (ns) |
| [wal_test_output.txt](wal_test_output.txt) | Build + run `tools/wal_test.c` | WAL checkpoint + recovery |
| [learning_iteration_output.txt](learning_iteration_output.txt) | `./scripts/jetson_self_optimize.sh --pmu` | PMU + manifold learning |

## Run Yourself

Download the [latest release](https://github.com/RYJOX-Technologies/Synrix-Memory-Engine/releases), extract, then:

```bash
./tools/crash_recovery_demo.sh
./tools/run_query_latency_diagnostic.sh
```
