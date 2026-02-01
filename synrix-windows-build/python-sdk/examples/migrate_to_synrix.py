#!/usr/bin/env python3
"""
SYNRIX Migration Helper
Automatically updates your LangChain code to use SYNRIX instead of Qdrant.
"""

import sys
import re
import shutil
from pathlib import Path


def migrate_file(file_path: str, dry_run: bool = False) -> bool:
    """Migrate a Python file from Qdrant to SYNRIX"""
    file_path = Path(file_path)
    
    if not file_path.exists():
        print(f"❌ File not found: {file_path}")
        return False
    
    # Read original file
    with open(file_path, 'r') as f:
        content = f.read()
    
    original_content = content
    changes_made = []
    
    # Pattern 1: url='http://localhost:6333' or url="http://localhost:6333"
    pattern1 = r"url\s*=\s*['\"]http://localhost:6333['\"]"
    if re.search(pattern1, content):
        content = re.sub(pattern1, "url='http://localhost:6334'", content)
        changes_made.append("Changed Qdrant URL port 6333 → 6334")
    
    # Pattern 2: host='localhost', port=6333
    pattern2 = r"port\s*=\s*6333"
    if re.search(pattern2, content):
        content = re.sub(pattern2, "port=6334", content)
        changes_made.append("Changed Qdrant port 6333 → 6334")
    
    # Pattern 3: QdrantClient(url='http://localhost:6333')
    pattern3 = r"QdrantClient\([^)]*url\s*=\s*['\"]http://localhost:6333['\"]"
    if re.search(pattern3, content):
        content = re.sub(
            r"url\s*=\s*['\"]http://localhost:6333['\"]",
            "url='http://localhost:6334'",
            content
        )
        changes_made.append("Changed QdrantClient URL port 6333 → 6334")
    
    if not changes_made:
        print(f"ℹ️  No Qdrant URLs found in {file_path}")
        return False
    
    if dry_run:
        print(f"\n📝 Would migrate: {file_path}")
        for change in changes_made:
            print(f"   • {change}")
        return True
    
    # Backup original
    backup_path = file_path.with_suffix(file_path.suffix + '.backup')
    shutil.copy2(file_path, backup_path)
    print(f"💾 Backup created: {backup_path}")
    
    # Write migrated file
    with open(file_path, 'w') as f:
        f.write(content)
    
    print(f"✅ Migrated: {file_path}")
    for change in changes_made:
        print(f"   • {change}")
    
    return True


def main():
    if len(sys.argv) < 2:
        print("SYNRIX Migration Helper")
        print("=" * 50)
        print()
        print("Usage:")
        print("  python3 migrate_to_synrix.py <file.py> [--dry-run]")
        print()
        print("Examples:")
        print("  python3 migrate_to_synrix.py my_app.py")
        print("  python3 migrate_to_synrix.py my_app.py --dry-run")
        print()
        print("This will:")
        print("  • Change Qdrant URLs from port 6333 → 6334")
        print("  • Create a backup of your original file")
        print("  • Update your code to use SYNRIX")
        sys.exit(1)
    
    file_path = sys.argv[1]
    dry_run = '--dry-run' in sys.argv
    
    if dry_run:
        print("🔍 DRY RUN MODE - No files will be modified\n")
    
    success = migrate_file(file_path, dry_run=dry_run)
    
    if success and not dry_run:
        print()
        print("🎯 Next steps:")
        print("  1. Start SYNRIX server:")
        print("     cd NebulOS-Scaffolding/integrations/qdrant_mimic")
        print("     ./synrix-server-evaluation --port 6334")
        print()
        print("  2. Run your app - it will now use SYNRIX!")
        print()
        print("  3. If something goes wrong, restore from backup:")
        print(f"     cp {file_path}.backup {file_path}")


if __name__ == "__main__":
    main()
