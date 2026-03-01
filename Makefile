# Makefile for NebulOS-Scaffolding / Synrix
# GitHub-ready: make build, make test-core

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -O2 -I. -I./src/storage/lattice
LDFLAGS = -lm -lpthread

# Lattice sources (for tools)
LATTICE_SRC = src/storage/lattice/persistent_lattice.c \
              src/storage/lattice/wal.c \
              src/storage/lattice/isolation.c \
              src/storage/lattice/seqlock.c \
              src/storage/lattice/dynamic_prefix_index.c \
              src/storage/lattice/exact_name_index.c \
              src/storage/lattice/lattice_constraints.c \
              src/storage/lattice/license_utils.c

# Core system components
CORE_SRC = src/storage/lattice/persistent_lattice.c
PHYSICS_SRC = physics/fundamental_physics.c
CHEMISTRY_SRC = chemistry/src/chemistry_domain.c
BIOLOGY_SRC = biology/fundamental_biology.c

# Validation test suite
TESTS = clean_slate_challenge random_input_challenge protein_folding_challenge \
        improved_biological_plausibility improved_lattice_persistence

# Default target (GitHub Quick Start: build tools only; physics/chemistry/biology not in repo)
all: build

# Core system
core: build-dir $(CORE_SRC)
	@echo "🔧 Compiling core system..."
	$(CC) $(CFLAGS) -c $(CORE_SRC) -o build/persistent_lattice.o
	@echo "✅ Core system compiled"

# Physics engine
physics: $(PHYSICS_SRC)
	@echo "🔬 Compiling physics engine..."
	$(CC) $(CFLAGS) -c $(PHYSICS_SRC) -o build/fundamental_physics.o
	@echo "✅ Physics engine compiled"

# Chemistry engine
chemistry: $(CHEMISTRY_SRC)
	@echo "🧪 Compiling chemistry engine..."
	$(CC) $(CFLAGS) -c $(CHEMISTRY_SRC) -o build/chemistry_domain.o
	@echo "✅ Chemistry engine compiled"

# Biology engine
biology: $(BIOLOGY_SRC)
	@echo "🧬 Compiling biology engine..."
	$(CC) $(CFLAGS) -c $(BIOLOGY_SRC) -o build/fundamental_biology.o
	@echo "✅ Biology engine compiled"

# Validation test suite
tests: $(TESTS)
	@echo "🧪 All validation tests compiled"

# Individual test compilation
clean_slate_challenge: clean_slate_challenge.c
	@echo "🧪 Compiling Clean Slate Challenge..."
	$(CC) $(CFLAGS) -o clean_slate_challenge clean_slate_challenge.c
	@echo "✅ Clean Slate Challenge compiled"

random_input_challenge: random_input_challenge.c
	@echo "🎲 Compiling Random Input Challenge..."
	$(CC) $(CFLAGS) -o random_input_challenge random_input_challenge.c $(LDFLAGS)
	@echo "✅ Random Input Challenge compiled"

protein_folding_challenge: protein_folding_challenge.c
	@echo "🧬 Compiling Protein Folding Challenge..."
	$(CC) $(CFLAGS) -o protein_folding_challenge protein_folding_challenge.c $(LDFLAGS)
	@echo "✅ Protein Folding Challenge compiled"

improved_biological_plausibility: improved_biological_plausibility.c
	@echo "🔬 Compiling Improved Biological Plausibility..."
	$(CC) $(CFLAGS) -o improved_biological_plausibility improved_biological_plausibility.c $(LDFLAGS)
	@echo "✅ Improved Biological Plausibility compiled"

improved_lattice_persistence: improved_lattice_persistence.c
	@echo "💾 Compiling Improved Lattice Persistence..."
	$(CC) $(CFLAGS) -o improved_lattice_persistence improved_lattice_persistence.c
	@echo "✅ Improved Lattice Persistence compiled"

# Create build directory
build-dir:
	@mkdir -p build
	@echo "📁 Build directory created"

# Build tools (crash_test, query_latency_diagnostic) - GitHub Quick Start
build: build-dir
	@echo "🔧 Building crash_test..."
	@$(CC) $(CFLAGS) tools/crash_test.c $(LATTICE_SRC) -o tools/crash_test $(LDFLAGS) 2>/dev/null || \
		$(CC) $(CFLAGS) tools/crash_test.c $(LATTICE_SRC) -o tools/crash_test $(LDFLAGS)
	@echo "🔧 Building query_latency_diagnostic..."
	@$(CC) $(CFLAGS) tools/query_latency_diagnostic.c $(LATTICE_SRC) -o tools/query_latency_diagnostic $(LDFLAGS) 2>/dev/null || \
		$(CC) $(CFLAGS) tools/query_latency_diagnostic.c $(LATTICE_SRC) -o tools/query_latency_diagnostic $(LDFLAGS)
	@echo "✅ Tools built: tools/crash_test, tools/query_latency_diagnostic"

# Clean build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -f $(TESTS)
	rm -f build/*.o
	rm -f *.bin
	rm -f tools/crash_test tools/query_latency_diagnostic
	@echo "✅ Clean complete"

# Run core tests (crash recovery + WAL) - same as test-core for GitHub Quick Start
# (Validation suite requires physics/chemistry/biology - use 'make test-validation' if present)
test: test-core

# Run all tests across codebase (unified test runner)
test-all:
	@echo "🧪 Running unified test suite..."
	@bash tests/run_all_tests.sh

# Run core tests: crash recovery demo + WAL (GitHub Quick Start)
test-core: build
	@echo "🧪 Running core tests (crash recovery + WAL)..."
	@echo ""
	@echo "=== Crash Recovery Demo ==="
	@./tools/crash_recovery_demo.sh 2>&1 | tail -20
	@echo ""
	@echo "✅ test-core complete"

test-integration:
	@bash tests/run_all_tests.sh integration

test-domain:
	@bash tests/run_all_tests.sh domain

test-pattern:
	@bash tests/run_all_tests.sh pattern

test-e2e:
	@bash tests/run_all_tests.sh e2e

# Quick validation (5 minutes)
quick-test: clean_slate_challenge random_input_challenge protein_folding_challenge
	@echo "⚡ QUICK VALIDATION (5 minutes)"
	@echo "==============================="
	@echo ""
	@echo "🧪 Clean Slate Challenge..."
	./clean_slate_challenge
	@echo ""
	@echo "🎲 Random Input Challenge..."
	./random_input_challenge
	@echo ""
	@echo "🧬 Protein Folding Challenge..."
	./protein_folding_challenge
	@echo ""
	@echo "✅ Quick validation complete!"

# Help
help:
	@echo "🎯 NebulOS-Scaffolding Build System"
	@echo "===================================="
	@echo ""
	@echo "Available targets:"
	@echo "  all             - Compile everything"
	@echo "  core            - Compile core system"
	@echo "  physics         - Compile physics engine"
	@echo "  chemistry       - Compile chemistry engine"
	@echo "  biology         - Compile biology engine"
	@echo "  tests           - Compile validation tests"
	@echo "  test            - Run original validation tests"
	@echo "  test-all        - Run unified test suite (all tests)"
	@echo "  test-core       - Run core system tests only"
	@echo "  test-integration - Run integration tests only"
	@echo "  test-domain     - Run domain tests only"
	@echo "  test-pattern    - Run pattern evolution tests only"
	@echo "  test-e2e        - Run end-to-end tests only"
	@echo "  quick-test      - Run quick validation (5 min)"
	@echo "  clean           - Clean build artifacts"
	@echo "  help            - Show this help"
	@echo ""
	@echo "Quick start:"
	@echo "  make all     # Compile everything"
	@echo "  make test    # Run validation tests"
	@echo "  make clean   # Clean up"

# Phony targets
.PHONY: all core physics chemistry biology tests test test-all test-core test-integration test-domain test-pattern test-e2e quick-test clean help