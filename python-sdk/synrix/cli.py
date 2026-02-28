"""
SYNRIX CLI Commands

Command-line interface for SYNRIX operations.
"""

import sys
import argparse
from pathlib import Path
from .engine import install_engine, find_engine, get_engine_path, check_engine_running
from .exceptions import SynrixError


def install_engine_command(args):
    """Handle 'synrix install-engine' command."""
    try:
        engine_path = install_engine(force=args.force)
        print(f"\n✅ Engine installed successfully!")
        print(f"   Location: {engine_path}")
        print(f"\n   To start the engine, run:")
        print(f"   {engine_path} --port 6334")
        print(f"\n   Or use synrix.init() in Python to auto-start it.")
        return 0
    except SynrixError as e:
        print(f"❌ Error: {e}", file=sys.stderr)
        return 1
    except Exception as e:
        print(f"❌ Unexpected error: {e}", file=sys.stderr)
        return 1


def status_command(args):
    """Handle 'synrix status' command."""
    engine_path = find_engine()
    
    if engine_path:
        print(f"✅ SYNRIX engine found: {engine_path}")
        
        # Check if running
        if check_engine_running(args.port):
            print(f"✅ Engine is running on port {args.port}")
        else:
            print(f"⚠️  Engine is not running on port {args.port}")
            print(f"   Start it with: {engine_path} --port {args.port}")
    else:
        print("❌ SYNRIX engine not found")
        print("   Install it with: synrix install-engine")
        return 1
    
    return 0


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
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())
