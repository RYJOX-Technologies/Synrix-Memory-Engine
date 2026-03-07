"""
SYNRIX CLI Commands

Command-line interface for SYNRIX operations.
"""

import os
import sys
import argparse
import subprocess
from pathlib import Path
from .engine import install_engine, find_engine, get_engine_path, check_engine_running
from .exceptions import SynrixError
from . import raw_backend

# ASCII-only output so CLI works on Windows cp1252 and other non-UTF-8 consoles
def _ok(s=""):
    return "[OK] " + s if s else "[OK]"
def _fail(s=""):
    return "[X] " + s if s else "[X]"
def _warn(s=""):
    return "[!] " + s if s else "[!]"


def install_engine_command(args):
    """Handle 'synrix install-engine' command."""
    try:
        engine_path = install_engine(force=args.force)
        print(f"\n{_ok()} Engine installed successfully!")
        print(f"   Location: {engine_path}")
        print(f"\n   To start the engine, run: synrix run")
        return 0
    except SynrixError as e:
        print(f"{_fail()} Error: {e}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"{_fail()} Unexpected error: {e}", file=sys.stderr)
        return 1


def run_command(args):
    """Handle 'synrix run' - start the engine server (foreground)."""
    engine_path = find_engine()
    if not engine_path:
        print(f"{_fail()} Engine not found. Run: synrix install-engine", file=sys.stderr)
        return 1
    if check_engine_running(args.port):
        print(f"{_ok()} Engine already running on port {args.port}")
        return 0
    print(f"Starting SYNRIX engine on port {args.port}... (Ctrl+C to stop)")
    try:
        subprocess.run(
            [str(engine_path), "--port", str(args.port)],
            check=True,
        )
    except KeyboardInterrupt:
        pass
    except FileNotFoundError:
        print(f"{_fail()} Engine binary not found: {engine_path}", file=sys.stderr)
        return 1
    except subprocess.CalledProcessError as e:
        print(f"{_fail()} Engine exited with code {e.returncode}", file=sys.stderr)
        return 1
    return 0


def status_command(args):
    """Handle 'synrix status' command."""
    engine_path = find_engine()
    
    if engine_path:
        print(f"{_ok()} SYNRIX engine found: {engine_path}")
        
        # Check if running
        if check_engine_running(args.port):
            print(f"{_ok()} Engine is running on port {args.port}")
        else:
            print(f"{_warn()} Engine is not running on port {args.port}")
            print(f"   Start it with: {engine_path} --port {args.port}")
    else:
        print(f"{_fail()} SYNRIX engine not found")
        print("   Install it with: synrix install-engine")
        return 1
    
    return 0


def license_check_command(args):
    """Handle 'synrix license check [KEY]' - validate cloud license via API."""
    api_url = os.environ.get("SYNRIX_LICENSE_API_URL", "").strip()
    if not api_url:
        print(f"{_fail()} Set SYNRIX_LICENSE_API_URL (e.g. https://xxx.supabase.co/functions/v1/check-license)", file=sys.stderr)
        return 1
    key = args.key or raw_backend._resolve_license_key()
    if not key:
        print(f"{_fail()} No license key. Pass KEY or set SYNRIX_LICENSE_KEY / add to ~/.synrix/license.json", file=sys.stderr)
        return 1
    if not key.startswith("synrix_cloud_"):
        print(f"{_warn()} Key does not look like a cloud key (synrix_cloud_xxx). API check may still work.", file=sys.stderr)
    hwid = raw_backend.get_hardware_id_standalone()
    if not hwid:
        print(f"{_fail()} Could not get hardware ID (engine lib not found or lattice_get_hardware_id missing).", file=sys.stderr)
        return 1
    claims = raw_backend._fetch_cloud_license_claims(key, hwid, api_url)
    if claims is None:
        print(f"{_fail()} License invalid or API error (check key, HWID, and API URL).")
        return 1
    print(f"{_ok()} License valid")
    tier_bytes = bytes(claims.tier)
    print(f"   Tier: {tier_bytes.split(b'\0')[0].decode('utf-8', errors='replace')}")
    print(f"   Node limit: {claims.node_limit or 'unlimited'}")
    return 0


def license_sync_command(args):
    """Handle 'synrix license sync' - verify cloud license; backend will use it on next init."""
    # Reuse check logic; sync is "verify and remind about env"
    r = license_check_command(argparse.Namespace(key=getattr(args, "key", None)))
    if r == 0:
        print("   Set SYNRIX_LICENSE_KEY and SYNRIX_LICENSE_API_URL; backend applies cloud license on init.")
    return r


def main():
    """Main CLI entry point."""
    parser = argparse.ArgumentParser(
        prog="synrix",
        description="SYNRIX - Local-first semantic memory system for AI applications"
    )
    
    subparsers = parser.add_subparsers(dest="command", help="Available commands")
    
    # install-engine command
    install_parser = subparsers.add_parser(
        "install-engine",
        help="Download and install SYNRIX engine binary"
    )
    install_parser.add_argument(
        "--force",
        action="store_true",
        help="Force re-download even if engine exists"
    )
    install_parser.set_defaults(func=install_engine_command)

    # run command (start engine server)
    run_parser = subparsers.add_parser(
        "run",
        help="Start the SYNRIX engine server (foreground; use in one terminal, then use SDK in another)"
    )
    run_parser.add_argument(
        "--port",
        type=int,
        default=6334,
        help="Port for the engine (default: 6334)"
    )
    run_parser.set_defaults(func=run_command)
    
    # status command
    status_parser = subparsers.add_parser(
        "status",
        help="Check SYNRIX engine status"
    )
    status_parser.add_argument(
        "--port",
        type=int,
        default=6334,
        help="Port to check (default: 6334)"
    )
    status_parser.set_defaults(func=status_command)
    
    # license check
    license_parser = subparsers.add_parser("license", help="Cloud license (Supabase check-license)")
    license_sub = license_parser.add_subparsers(dest="license_subcommand")
    check_parser = license_sub.add_parser("check", help="Check cloud license (requires SYNRIX_LICENSE_API_URL)")
    check_parser.add_argument("key", nargs="?", default=None, help="License key (default: from env/file)")
    check_parser.set_defaults(func=license_check_command)
    sync_parser = license_sub.add_parser("sync", help="Verify cloud license; backend uses it on next init")
    sync_parser.add_argument("key", nargs="?", default=None, help="License key (default: from env/file)")
    sync_parser.set_defaults(func=license_sync_command)
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    # license check / license sync
    if args.command == "license":
        if not getattr(args, "license_subcommand", None):
            license_parser.print_help()
            return 1
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
